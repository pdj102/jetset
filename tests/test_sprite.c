/* zxn_sprite.c: sprite attributes, uploads, clip window and status. */
#include "test.h"

#include <arch/zxn.h>

#include "zxn_sprite.h"

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

/*
 * Run zxn_sprite_update() and check its writes: slot 0 selected, then only
 * attribute bytes. Returns how many sprites were sent; their bytes start at
 * test_port_log_val[1].
 */
static int upload(void)
{
    test_port_reset();
    zxn_sprite_update();
    CHECK(test_port_log_count >= 1);
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 0);
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == test_port_log_count - 1);
    CHECK((test_port_log_count - 1) % ATTR_BYTES == 0);
    return (test_port_log_count - 1) / ATTR_BYTES;
}

/* Attribute byte n (0-4) of sprite id in the last upload */
static uint8_t attr(int id, int n)
{
    return test_port_log_val[1 + id * ATTR_BYTES + n];
}

static uint8_t status_value;

static uint8_t read_status(uint16_t port)
{
    CHECK(port == __IO_SPRITE_FLAGS);
    return status_value;
}

int main(void)
{
    static uint8_t patterns[3 * ZXN_SPRITE_PATTERN_SIZE];
    static const zxn_rect_t clip = { .x1 = 16, .x2 = 143, .y1 = 32, .y2 = 223 };
    int i;
    int ok;

    test_init();

    /* ---- init: all 128 hidden, transparency, layer bits ---- */
    test_regs[REG_SPRITE_LAYER_SYSTEM] = RSLS_LAYER_PRIORITY_SLU | 0x40;  /* keep other bits */
    zxn_sprite_init();
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 0);
    CHECK(port_writes(__IO_SPRITE_ATTRIBUTE) == ZXN_SPRITE_MAX * ATTR_BYTES);
    ok = 1;
    for (i = 0; i < ZXN_SPRITE_MAX; i++) {
        /* Hidden, and 5-byte attributes so every sprite takes the same bytes */
        ok = ok && test_port_log_val[1 + i * ATTR_BYTES + 3] == 0x40;
    }
    CHECK(ok);
    CHECK(test_regs[REG_SPRITE_TRANSPARENCY_INDEX] == ZXN_SPRITE_TRANSPARENT);
    CHECK(test_regs[REG_SPRITE_LAYER_SYSTEM] ==
          (RSLS_LAYER_PRIORITY_SLU | 0x40 | RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER));

    /* Nothing used yet: an update only selects slot 0 */
    CHECK(upload() == 0);

    /* ---- set: position, pattern, flags ---- */
    zxn_sprite_set(0, 100, 50, 5, 0);
    CHECK(upload() == 1);
    CHECK(attr(0, 0) == 100 && attr(0, 1) == 50);
    CHECK(attr(0, 2) == 0);
    CHECK(attr(0, 3) == (0x80 | 0x40 | 5));
    CHECK(attr(0, 4) == 0);

    /* 9-bit coordinates: bit 8 of X in attr2, of Y in attr4 */
    zxn_sprite_set(2, 0x13F, 0x1F0, 63,
                   ZXN_SPRITE_MIRROR_X | ZXN_SPRITE_ROTATE | ZXN_SPRITE_PALETTE(3));
    CHECK(upload() == 3);   /* sprites 0-2: the unused sprite 1 is sent hidden */
    CHECK(attr(1, 3) == 0x40);
    CHECK(attr(2, 0) == 0x3F && attr(2, 1) == 0xF0);
    CHECK(attr(2, 2) == (0x30 | 0x08 | 0x02 | 0x01));
    CHECK(attr(2, 3) == (0xC0 | 63));
    CHECK(attr(2, 4) == 0x01);

    /* Moving keeps the flags; X bit 8 clears */
    zxn_sprite_move(2, 20, 0x100);
    upload();
    CHECK(attr(2, 0) == 20 && attr(2, 1) == 0);
    CHECK(attr(2, 2) == (0x30 | 0x08 | 0x02));
    CHECK(attr(2, 4) == 0x01);

    /* New flags keep the position; scale keeps Y bit 8 */
    zxn_sprite_move(2, 0x101, 0x100);
    zxn_sprite_set_flags(2, ZXN_SPRITE_MIRROR_Y);
    zxn_sprite_set_scale(2, ZXN_SPRITE_SCALE_X2 | ZXN_SPRITE_SCALE_Y8);
    upload();
    CHECK(attr(2, 2) == (0x04 | 0x01));
    CHECK(attr(2, 4) == (0x08 | 0x06 | 0x01));
    zxn_sprite_set_scale(2, ZXN_SPRITE_SCALE_1);
    upload();
    CHECK(attr(2, 4) == 0x01);

    /* Changing pattern keeps visibility */
    zxn_sprite_set_pattern(0, 7);
    upload();
    CHECK(attr(0, 3) == (0xC0 | 7));

    /* ---- 4-bit patterns: H set, N6 = lowest bit of the pattern ---- */
    zxn_sprite_set4(3, 8, 8, 13, ZXN_SPRITE_PALETTE(15));
    upload();
    CHECK(attr(3, 2) == 0xF0);
    CHECK(attr(3, 3) == (0xC0 | 6));
    CHECK(attr(3, 4) == (0x80 | 0x40));
    zxn_sprite_set_pattern4(3, 12);
    upload();
    CHECK(attr(3, 3) == (0xC0 | 6) && attr(3, 4) == 0x80);
    /* Back to an 8-bit pattern clears H and N6 */
    zxn_sprite_set_pattern(3, 1);
    upload();
    CHECK(attr(3, 3) == (0xC0 | 1) && attr(3, 4) == 0);

    /* ---- visibility ---- */
    zxn_sprite_show(0, 0);
    upload();
    CHECK(attr(0, 3) == (0x40 | 7));
    zxn_sprite_show(0, 1);
    upload();
    CHECK(attr(0, 3) == (0xC0 | 7));
    zxn_sprite_hide_all();
    CHECK(upload() == 4);
    ok = 1;
    for (i = 0; i < 4; i++) {
        ok = ok && (attr(i, 3) & 0x80) == 0;
    }
    CHECK(ok);

    /* ---- pattern uploads ---- */
    for (i = 0; i < (int)sizeof(patterns); i++) {
        patterns[i] = (uint8_t)(i * 7);
    }
    test_port_reset();
    zxn_sprite_load_patterns(10, patterns, 3);
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 10);
    CHECK(port_writes(__IO_SPRITE_PATTERN) == 3 * ZXN_SPRITE_PATTERN_SIZE);
    CHECK(memcmp(&test_port_log_val[1], patterns, sizeof(patterns)) == 0);

    /* Odd 4-bit pattern: second half of 256-byte slot 2 */
    test_port_reset();
    zxn_sprite_load_patterns4(5, patterns, 2);
    CHECK(test_port_log_val[0] == (0x80 | 2));
    CHECK(port_writes(__IO_SPRITE_PATTERN) == 2 * ZXN_SPRITE_PATTERN4_SIZE);
    test_port_reset();
    zxn_sprite_load_patterns4(4, patterns, 1);
    CHECK(test_port_log_val[0] == 2);

    /* ---- clip window ---- */
    test_reg_log_count = 0;
    zxn_sprite_set_clip(&clip);
    CHECK(test_reg_log_reg[0] == REG_CLIP_WINDOW_CONTROL && test_reg_log_val[0] == RCWC_RESET_SPRITE_CLIP_INDEX);
    CHECK(test_reg_log_reg[1] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[1] == 16);
    CHECK(test_reg_log_reg[2] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[2] == 143);
    CHECK(test_reg_log_reg[3] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[3] == 32);
    CHECK(test_reg_log_reg[4] == REG_CLIP_WINDOW_SPRITES && test_reg_log_val[4] == 223);
    CHECK(test_regs[REG_SPRITE_LAYER_SYSTEM] & 0x20);
    zxn_sprite_clear_clip();
    CHECK(test_regs[REG_SPRITE_LAYER_SYSTEM] ==
          (RSLS_LAYER_PRIORITY_SLU | 0x40 | RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER));

    /* ---- status ---- */
    test_port_read_hook = read_status;
    status_value = 0xFD;   /* undefined bits are masked off */
    CHECK(zxn_sprite_status() == ZXN_SPRITE_STATUS_COLLISION);
    status_value = 0x02;
    CHECK(zxn_sprite_status() == ZXN_SPRITE_STATUS_LINE_FULL);
    test_port_read_hook = NULL;

    /* ---- out of range ---- */
    TEST_EXPECT_HALT(zxn_sprite_set(ZXN_SPRITE_MAX, 0, 0, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_sprite_move(200, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_sprite_set(0, 0, 0, ZXN_SPRITE_PATTERNS, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_sprite_set4(0, 0, 0, ZXN_SPRITE_PATTERNS4, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_sprite_load_patterns(63, patterns, 2));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_sprite_load_patterns4(127, patterns, 2));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(zxn_sprite_load_patterns(0, NULL, 1));
    CHECK(test_halt_count == 1);

    return test_summary("sprite");
}
