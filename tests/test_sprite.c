/* nge_sprite.c: attribute records, the game's table, uploads, clip window and status. */
#include "test.h"

#include <arch/zxn.h>

#include "sprite/nge_sprite.h"

#define ATTR_BYTES 5

/* Number of logged writes to a port, from the start of the log */
static int port_writes(uint16_t port)
{
    int i;
    int n = 0;

    for (i = 0; i < test_port_log_count; i++) {
        n += test_port_log_port[i] == port;
    }
    return n;
}

/* First hardware sprite of the registered table */
static uint8_t table_first;

/*
 * Run nge_sprite_update() and check its writes: the table's first sprite
 * selected, then only attribute bytes. Returns how many sprites were sent;
 * their bytes start at test_port_log_val[1].
 */
static int upload(void)
{
    test_port_reset();
    nge_sprite_update();
    CHECK(test_port_log_count >= 1);
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == table_first);
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == test_port_log_count - 1);
    CHECK((test_port_log_count - 1) % ATTR_BYTES == 0);
    return (test_port_log_count - 1) / ATTR_BYTES;
}

/* Attribute byte n (0-4) of sprite id in the last upload */
static uint8_t attr(int id, int n)
{
    return test_port_log_val[1 + (id - table_first) * ATTR_BYTES + n];
}

static uint8_t status_value;

static uint8_t read_status(uint16_t port)
{
    CHECK(port == __IO_SPRITE_FLAGS);
    return status_value;
}

int main(void)
{
    static uint8_t patterns[3 * NGE_SPRITE_PATTERN_SIZE];
    static const nge_rect_t clip = { .x1 = 16, .x2 = 143, .y1 = 32, .y2 = 223 };
    static nge_sprite_attr_t table[8];
    static nge_sprite_attr_t hud[16];
    nge_sprite_attr_t a[3];
    int i;
    int ok;

    test_init();

    /* ---- init: all 128 hidden, transparency, layer bits ---- */
    test_regs[REG_SPRITE_LAYER_SYSTEM] = RSLS_LAYER_PRIORITY_SLU | 0x40;  /* keep other bits */
    nge_sprite_init();
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 0);
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == NGE_SPRITE_MAX * ATTR_BYTES);
    ok = 1;
    for (i = 0; i < NGE_SPRITE_MAX; i++) {
        /* Hidden, and 5-byte attributes so every sprite takes the same bytes */
        ok = ok && test_port_log_val[1 + i * ATTR_BYTES + 3] == 0x40;
    }
    CHECK(ok);
    CHECK(test_regs[REG_SPRITE_TRANSPARENCY_INDEX] == NGE_SPRITE_TRANSPARENT);
    CHECK(test_regs[REG_SPRITE_LAYER_SYSTEM] ==
          (RSLS_LAYER_PRIORITY_SLU | 0x40 | RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER));

    /* ---- attribute records ---- */
    CHECK(sizeof(nge_sprite_attr_t) == ATTR_BYTES);
    nge_sprite_attr_build(&a[0], 100, 50, 5, NGE_SPRITE_MIRROR_X | NGE_SPRITE_PALETTE(2), 0);
    CHECK(a[0].x == 100 && a[0].y == 50 && a[0].attr2 == (0x20 | 0x08));
    CHECK(a[0].attr3 == (0x80 | 0x40 | 5) && a[0].attr4 == 0);
    /* 9-bit coordinates, scale, 4-bit odd pattern (N6) */
    nge_sprite_attr_build(&a[1], 0x13F, 0x1F0, 13, 0,
                          NGE_SPRITE_SCALE_X2 | NGE_SPRITE_SCALE_Y4 | NGE_SPRITE_PATTERN4);
    CHECK(a[1].x == 0x3F && a[1].y == 0xF0 && a[1].attr2 == 0x01);
    CHECK(a[1].attr3 == (0xC0 | 6));
    CHECK(a[1].attr4 == (0x80 | 0x40 | 0x08 | 0x04 | 0x01));
    nge_sprite_attr_build(&a[2], 0, 0, 12, 0, NGE_SPRITE_PATTERN4);   /* even: no N6 */
    CHECK(a[2].attr3 == (0xC0 | 6) && a[2].attr4 == 0x80);
    NGE_SPRITE_ATTR_HIDE(&a[2]);
    CHECK(a[2].attr3 == (0x40 | 6) && a[2].attr4 == 0x80);
    TEST_EXPECT_HALT(nge_sprite_attr_build(&a[2], 0, 0, NGE_SPRITE_PATTERNS, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_attr_build(&a[2], 0, 0, NGE_SPRITE_PATTERNS4, 0, NGE_SPRITE_PATTERN4));
    CHECK(test_halt_count == 1);

    /* Upload: any first sprite, the records' bytes in order */
    test_port_reset();
    nge_sprite_upload(5, a, 2);
    CHECK(test_port_log_count == 1 + 2 * ATTR_BYTES);
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 5);
    CHECK(memcmp(&test_port_log_val[1], a, 2 * ATTR_BYTES) == 0);
    TEST_EXPECT_HALT(nge_sprite_upload(127, a, 2));
    CHECK(test_halt_count == 1);

    /* ---- no table: the setters assert, update sends nothing ---- */
    TEST_EXPECT_HALT(nge_sprite_move(0, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_set(0, 0, 0, 0, 0));
    CHECK(test_halt_count == 1);
    test_port_reset();
    nge_sprite_update();
    CHECK(test_port_log_count == 0);

    /* ---- a table for sprites 0-7 ---- */
    table[3].attr3 = 0xFF;
    nge_sprite_table_init(table, 0, 8);
    CHECK(table[3].attr3 == 0x40);                           /* all hidden */
    table_first = 0;
    CHECK(upload() == 0);                                    /* nothing used yet */

    /* ---- set: position, pattern, flags ---- */
    nge_sprite_set(0, 100, 50, 5, 0);
    CHECK(upload() == 1);
    CHECK(attr(0, 0) == 100 && attr(0, 1) == 50);
    CHECK(attr(0, 2) == 0);
    CHECK(attr(0, 3) == (0x80 | 0x40 | 5));
    CHECK(attr(0, 4) == 0);

    /* 9-bit coordinates: bit 8 of X in attr2, of Y in attr4 */
    nge_sprite_set(2, 0x13F, 0x1F0, 63,
                   NGE_SPRITE_MIRROR_X | NGE_SPRITE_ROTATE | NGE_SPRITE_PALETTE(3));
    CHECK(upload() == 3);   /* sprites 0-2: the unused sprite 1 is sent hidden */
    CHECK(attr(1, 3) == 0x40);
    CHECK(attr(2, 0) == 0x3F && attr(2, 1) == 0xF0);
    CHECK(attr(2, 2) == (0x30 | 0x08 | 0x02 | 0x01));
    CHECK(attr(2, 3) == (0xC0 | 63));
    CHECK(attr(2, 4) == 0x01);

    /* Moving keeps the flags; X bit 8 clears */
    nge_sprite_move(2, 20, 0x100);
    upload();
    CHECK(attr(2, 0) == 20 && attr(2, 1) == 0);
    CHECK(attr(2, 2) == (0x30 | 0x08 | 0x02));
    CHECK(attr(2, 4) == 0x01);

    /* New flags keep the position; scale keeps Y bit 8 */
    nge_sprite_move(2, 0x101, 0x100);
    nge_sprite_set_flags(2, NGE_SPRITE_MIRROR_Y);
    nge_sprite_set_scale(2, NGE_SPRITE_SCALE_X2 | NGE_SPRITE_SCALE_Y8);
    upload();
    CHECK(attr(2, 2) == (0x04 | 0x01));
    CHECK(attr(2, 4) == (0x08 | 0x06 | 0x01));
    nge_sprite_set_scale(2, NGE_SPRITE_SCALE_1);
    upload();
    CHECK(attr(2, 4) == 0x01);

    /* Changing pattern keeps visibility */
    nge_sprite_set_pattern(0, 7);
    upload();
    CHECK(attr(0, 3) == (0xC0 | 7));

    /* ---- 4-bit patterns: H set, N6 = lowest bit of the pattern ---- */
    nge_sprite_set4(3, 8, 8, 13, NGE_SPRITE_PALETTE(15));
    upload();
    CHECK(attr(3, 2) == 0xF0);
    CHECK(attr(3, 3) == (0xC0 | 6));
    CHECK(attr(3, 4) == (0x80 | 0x40));
    nge_sprite_set_pattern4(3, 12);
    upload();
    CHECK(attr(3, 3) == (0xC0 | 6) && attr(3, 4) == 0x80);
    /* Back to an 8-bit pattern clears H and N6 */
    nge_sprite_set_pattern(3, 1);
    upload();
    CHECK(attr(3, 3) == (0xC0 | 1) && attr(3, 4) == 0);

    /* ---- visibility ---- */
    nge_sprite_show(0, 0);
    upload();
    CHECK(attr(0, 3) == (0x40 | 7));
    nge_sprite_show(0, 1);
    upload();
    CHECK(attr(0, 3) == (0xC0 | 7));
    test_port_reset();
    nge_sprite_hide_all();
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == NGE_SPRITE_MAX * ATTR_BYTES);   /* hardware, now */
    CHECK(upload() == 4);
    ok = 1;
    for (i = 0; i < 4; i++) {
        ok = ok && (attr(i, 3) & 0x80) == 0;
    }
    CHECK(ok);

    /* ---- a table for the top sprites, e.g. a HUD ---- */
    nge_sprite_table_init(hud, 112, 16);
    table_first = 112;
    nge_sprite_set(113, 10, 20, 1, 0);
    CHECK(upload() == 2);                                    /* 112 (hidden) and 113 */
    CHECK(attr(112, 3) == 0x40);
    CHECK(attr(113, 0) == 10 && attr(113, 1) == 20 && attr(113, 3) == (0xC0 | 1));
    nge_sprite_set(127, 0, 0, 0, 0);
    CHECK(upload() == 16);
    TEST_EXPECT_HALT(nge_sprite_move(111, 0, 0));            /* below the table */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_show(0, 1));
    CHECK(test_halt_count == 1);
    nge_sprite_set_scale(113, NGE_SPRITE_SCALE_X2);
    nge_sprite_set(113, 10, 20, 2, 0);                       /* set keeps the scale */
    upload();
    CHECK(attr(113, 4) == 0x08 && attr(113, 3) == (0xC0 | 2));
    nge_sprite_set4(113, 10, 20, 3, 0);
    upload();
    CHECK(attr(113, 4) == (0x80 | 0x40 | 0x08) && attr(113, 3) == (0xC0 | 1));

    /* ---- pattern uploads ---- */
    for (i = 0; i < (int)sizeof(patterns); i++) {
        patterns[i] = (uint8_t)(i * 7);
    }
    test_port_reset();
    test_reg_log_count = 0;
    nge_sprite_load_patterns(NGE_SPRITE_DATA_COMMON, 10, patterns, 3);
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 10);
    CHECK(port_writes(__IO_SPRITE_PATTERN) == 3 * NGE_SPRITE_PATTERN_SIZE);
    CHECK(memcmp(&test_port_log_val[1], patterns, sizeof(patterns)) == 0);
    CHECK(test_reg_log_count == 0);                  /* common data: no mapping */

    /* Data in a page: mapped into slot 7 for the upload, then mapped back */
    test_regs[REG_MMU7] = 1;
    test_port_reset();
    test_reg_log_count = 0;
    nge_sprite_load_patterns(41, 0, patterns, 1);
    CHECK(test_reg_log_count == 2);
    CHECK(test_reg_log_reg[0] == REG_MMU7 && test_reg_log_val[0] == 41);
    CHECK(test_reg_log_reg[1] == REG_MMU7 && test_reg_log_val[1] == 1);
    CHECK(port_writes(__IO_SPRITE_PATTERN) == NGE_SPRITE_PATTERN_SIZE);

    /* Odd 4-bit pattern: second half of 256-byte slot 2 */
    test_port_reset();
    nge_sprite_load_patterns4(NGE_SPRITE_DATA_COMMON, 5, patterns, 2);
    CHECK(test_port_log_val[0] == (0x80 | 2));
    CHECK(port_writes(__IO_SPRITE_PATTERN) == 2 * NGE_SPRITE_PATTERN4_SIZE);
    test_port_reset();
    nge_sprite_load_patterns4(NGE_SPRITE_DATA_COMMON, 4, patterns, 1);
    CHECK(test_port_log_val[0] == 2);
    test_reg_log_count = 0;
    nge_sprite_load_patterns4(43, 4, patterns, 1);
    CHECK(test_reg_log_count == 2 && test_reg_log_val[0] == 43 && test_reg_log_val[1] == 1);

    /* ---- sprite palette ---- */
    {
        static const nge_palette_entry_9_t pal[] = { {0x11, 1}, {0x22, 0} };

        test_reg_log_count = 0;
        nge_sprite_load_palette(41, 16, pal, 2);
        CHECK(test_reg_log_reg[0] == REG_MMU7 && test_reg_log_val[0] == 41);
        CHECK(test_reg_log_reg[1] == 0x43 && (test_reg_log_val[1] & 0x70) == RPC_SELECT_SPRITES_PALETTE_0);
        CHECK(test_reg_log_reg[2] == 0x40 && test_reg_log_val[2] == 16);
        CHECK(test_reg_log_val[3] == 0x11 && test_reg_log_val[6] == 0);
        CHECK(test_reg_log_reg[7] == REG_MMU7 && test_reg_log_val[7] == 1);
        CHECK(test_reg_log_count == 8);
    }

    /* ---- clip window ---- */
    test_reg_log_count = 0;
    nge_sprite_set_clip(&clip);
    CHECK(test_reg_log_reg[0] == REG_CLIP_WINDOW_CONTROL && test_reg_log_val[0] == RCWC_RESET_SPRITE_CLIP_INDEX);
    CHECK(test_reg_log_reg[1] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[1] == 16);
    CHECK(test_reg_log_reg[2] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[2] == 143);
    CHECK(test_reg_log_reg[3] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[3] == 32);
    CHECK(test_reg_log_reg[4] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[4] == 223);
    CHECK(test_regs[REG_SPRITE_LAYER_SYSTEM] & 0x20);
    nge_sprite_clear_clip();
    CHECK(test_regs[REG_SPRITE_LAYER_SYSTEM] ==
          (RSLS_LAYER_PRIORITY_SLU | 0x40 | RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER));

    /* ---- status ---- */
    test_port_read_hook = read_status;
    status_value = 0xFD;   /* undefined bits are masked off */
    CHECK(nge_sprite_status() == NGE_SPRITE_STATUS_COLLISION);
    status_value = 0x02;
    CHECK(nge_sprite_status() == NGE_SPRITE_STATUS_LINE_FULL);
    test_port_read_hook = NULL;

    /* ---- out of range ---- */
    TEST_EXPECT_HALT(nge_sprite_set(NGE_SPRITE_MAX, 0, 0, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_move(200, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_set(0, 0, 0, NGE_SPRITE_PATTERNS, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_set4(0, 0, 0, NGE_SPRITE_PATTERNS4, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_load_patterns(NGE_SPRITE_DATA_COMMON, 63, patterns, 2));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_load_patterns4(NGE_SPRITE_DATA_COMMON, 127, patterns, 2));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_load_patterns(NGE_SPRITE_DATA_COMMON, 0, NULL, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_load_patterns(40, 0, patterns, 1));   /* even page */
    CHECK(test_halt_count == 1);

    return test_summary("sprite");
}
