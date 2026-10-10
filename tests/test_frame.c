/* zxn_frame.c: waiting for vertical blank. */
#include "test.h"

#include <arch/zxn.h>

#include "zxn_frame.h"

/*
 * Successive video line numbers. Reading the high register returns bit 8 of
 * the current line; reading the low register returns its low byte and then
 * moves on to the next line in the sequence, so the line can change between
 * the two halves of a read, as on the hardware.
 */
static const uint16_t *sequence;
static int length;
static int pos;
static int reads;

static uint8_t read_line(uint8_t reg)
{
    /* After the last line the display stays on it (the final check of
       frame_line() reads the high byte once more) */
    uint16_t line = sequence[pos < length ? pos : length - 1];

    reads++;
    if (reg == REG_ACTIVE_VIDEO_LINE_H) {
        return (uint8_t)(0xFE | (line >> 8));   /* only bit 0 is defined */
    }
    CHECK(reg == REG_ACTIVE_VIDEO_LINE_L);
    pos++;
    return (uint8_t)line;
}

/* Run zxn_frame_wait_vblank() over a sequence; return the index it stopped at */
#define wait_over(lines)  wait_over_n(lines, sizeof(lines) / sizeof(lines[0]))

static int wait_over_n(const uint16_t *lines, int n)
{
    sequence = lines;
    length = n;
    pos = 0;
    reads = 0;
    zxn_frame_wait_vblank();
    return pos - 1;
}

int main(void)
{
    /* In the display area: returns at the first blanking line */
    static const uint16_t in_display[] = { 10, 100, 191, 192 };
    /* Line 128 sets bit 7 of the low byte, but is still in the display area */
    static const uint16_t line_128[] = { 127, 128, 150, 192 };
    /* Inside a blanking period: waits for it to end, then for the next one */
    static const uint16_t in_blank[] = { 200, 300, 311, 0, 50, 192 };
    /* Lines 256-311 have a low byte below 192 but are still blanking */
    static const uint16_t high_lines[] = { 256, 260, 311, 5, 195 };
    /* 255 -> 256 between the high and low reads: torn value 0 is rejected */
    static const uint16_t torn[] = { 255, 256, 257, 0, 192 };

    test_init();
    test_reg_read_hook = read_line;

    CHECK(wait_over(in_display) == 3);
    CHECK(reads == 3 * 4);
    CHECK(wait_over(line_128) == 3);
    CHECK(wait_over(in_blank) == 5);
    CHECK(wait_over(high_lines) == 4);
    CHECK(wait_over(torn) == 4);

    test_reg_read_hook = NULL;
    return test_summary("frame");
}
