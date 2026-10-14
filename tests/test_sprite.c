/* nge_sprite.c: records, uploads, pattern and palette loading, clip window and status. */
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

/* The five bytes of record a */
static int record_is(const nge_sprite_attr_t *a, uint8_t x, uint8_t y, uint8_t a2, uint8_t a3, uint8_t a4)
{
    return a->x == x && a->y == y && a->attr2 == a2 && a->attr3 == a3 && a->attr4 == a4;
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
    nge_sprite_attr_t a[4];
    nge_sprite_attr_t keep;
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

    /* ---- records: clear ---- */
    CHECK(sizeof(nge_sprite_attr_t) == ATTR_BYTES);
    memset(a, 0xFF, sizeof(a));
    nge_sprite_clear(a, 3);
    CHECK(record_is(&a[0], 0, 0, 0, 0x40, 0) && record_is(&a[2], 0, 0, 0, 0x40, 0));   /* hidden */
    CHECK(a[3].attr3 == 0xFF);                               /* only count records */

    /* ---- set: position, pattern, flags, mode ---- */
    nge_sprite_set(&a[0], 100, 50, 5, NGE_SPRITE_MIRROR_X | NGE_SPRITE_PALETTE(2), 0);
    CHECK(record_is(&a[0], 100, 50, 0x20 | 0x08, 0x80 | 0x40 | 5, 0));
    /* 9-bit coordinates: bit 8 of X in attr2, of Y in attr4; scale */
    nge_sprite_set(&a[1], 0x13F, 0x1F0, 63, NGE_SPRITE_ROTATE, NGE_SPRITE_SCALE_X2 | NGE_SPRITE_SCALE_Y8);
    CHECK(record_is(&a[1], 0x3F, 0xF0, 0x02 | 0x01, 0xC0 | 63, 0x08 | 0x06 | 0x01));
    /* 4-bit patterns: H set, N6 = lowest bit of the pattern */
    nge_sprite_set(&a[2], 8, 8, 13, NGE_SPRITE_PALETTE(15), NGE_SPRITE_PATTERN4);
    CHECK(record_is(&a[2], 8, 8, 0xF0, 0xC0 | 6, 0x80 | 0x40));
    nge_sprite_set(&a[2], 8, 8, 12, 0, NGE_SPRITE_PATTERN4);
    CHECK(record_is(&a[2], 8, 8, 0, 0xC0 | 6, 0x80));
    /* Set replaces everything: back to 8-bit, normal size */
    nge_sprite_set(&a[1], 0, 0, 1, 0, 0);
    CHECK(record_is(&a[1], 0, 0, 0, 0xC0 | 1, 0));

    /* ---- changing one property keeps the others ---- */
    nge_sprite_set(&a[0], 0x13F, 0x100, 5, NGE_SPRITE_MIRROR_X | NGE_SPRITE_PALETTE(3), NGE_SPRITE_SCALE_X4);
    nge_sprite_move(&a[0], 20, 0x101);                       /* X bit 8 clears, flags kept */
    CHECK(record_is(&a[0], 20, 1, 0x30 | 0x08, 0xC0 | 5, 0x10 | 0x01));
    nge_sprite_set_flags(&a[0], NGE_SPRITE_MIRROR_Y);        /* position kept */
    nge_sprite_move(&a[0], 0x101, 0x101);
    nge_sprite_set_flags(&a[0], NGE_SPRITE_MIRROR_Y);
    CHECK(a[0].attr2 == (0x04 | 0x01));
    nge_sprite_set_scale(&a[0], NGE_SPRITE_SCALE_Y2);        /* Y bit 8 kept */
    CHECK(a[0].attr4 == (0x02 | 0x01));
    nge_sprite_set_pattern(&a[0], 7);                        /* visibility kept */
    CHECK(a[0].attr3 == (0xC0 | 7));
    nge_sprite_show(&a[0], 0);
    CHECK(a[0].attr3 == (0x40 | 7));
    nge_sprite_set_pattern(&a[0], 8);                        /* still hidden */
    CHECK(a[0].attr3 == (0x40 | 8));
    nge_sprite_show(&a[0], 1);
    CHECK(a[0].attr3 == (0xC0 | 8));
    keep = a[0];
    nge_sprite_show(&a[0], 1);
    CHECK(memcmp(&keep, &a[0], ATTR_BYTES) == 0);

    /* The pattern is read as the record's type: 4-bit if set with NGE_SPRITE_PATTERN4 */
    nge_sprite_set(&a[2], 8, 8, 12, 0, NGE_SPRITE_PATTERN4 | NGE_SPRITE_SCALE_X2);
    nge_sprite_set_pattern(&a[2], 127);
    CHECK(a[2].attr3 == (0xC0 | 63) && a[2].attr4 == (0x80 | 0x40 | 0x08));
    nge_sprite_set_pattern(&a[2], 2);
    CHECK(a[2].attr3 == (0xC0 | 1) && a[2].attr4 == (0x80 | 0x08));
    nge_sprite_set_scale(&a[2], NGE_SPRITE_SCALE_1);         /* stays 4-bit */
    CHECK(a[2].attr4 == 0x80);

    /* ---- upload: any first sprite, the records' bytes in order ---- */
    test_port_reset();
    nge_sprite_upload(112, a, 3);
    CHECK(test_port_log_count == 1 + 3 * ATTR_BYTES);
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 112);
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == 3 * ATTR_BYTES);
    CHECK(memcmp(&test_port_log_val[1], a, 3 * ATTR_BYTES) == 0);
    test_port_reset();
    nge_sprite_upload(0, a, 0);                              /* nothing to send */
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == 0);
    nge_sprite_upload(125, a, 3);                            /* up to the last sprite */
    TEST_EXPECT_HALT(nge_sprite_upload(126, a, 3));
    CHECK(test_halt_count == 1);

    /* ---- hide_all: all 128 in the hardware, records unchanged ---- */
    keep = a[0];
    test_port_reset();
    nge_sprite_hide_all();
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 0);
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == NGE_SPRITE_MAX * ATTR_BYTES);
    ok = 1;
    for (i = 0; i < NGE_SPRITE_MAX; i++) {
        ok = ok && test_port_log_val[1 + i * ATTR_BYTES + 3] == 0x40;
    }
    CHECK(ok);
    CHECK(memcmp(&keep, &a[0], ATTR_BYTES) == 0);

    /* ---- out of range patterns ---- */
    TEST_EXPECT_HALT(nge_sprite_set(&a[3], 0, 0, NGE_SPRITE_PATTERNS, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_set(&a[3], 0, 0, NGE_SPRITE_PATTERNS4, 0, NGE_SPRITE_PATTERN4));
    CHECK(test_halt_count == 1);
    nge_sprite_set(&a[3], 0, 0, 0, 0, 0);
    TEST_EXPECT_HALT(nge_sprite_set_pattern(&a[3], NGE_SPRITE_PATTERNS));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_sprite_clear(NULL, 1));
    CHECK(test_halt_count == 1);

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
