/* zxn_frame.c: waiting for vertical blank. */
#include "test.h"

#include "zxn_frame.h"

/* Successive values read from the active video line register */
static const uint8_t *sequence;
static int reads;

static uint8_t read_line(uint8_t reg)
{
    (void)reg;
    return sequence[reads++];
}

int main(void)
{
    /* Starts inside a retrace: waits for it to end, then for the next one */
    static const uint8_t in_retrace[] = { 0x80, 0x80, 0x10, 0x20, 0x80 };
    static const uint8_t in_display[] = { 0x10, 0x10, 0x10, 0x80 };

    test_init();
    test_reg_read_hook = read_line;

    sequence = in_retrace;
    reads = 0;
    zxn_frame_wait_vblank();
    CHECK(reads == 5);

    sequence = in_display;
    reads = 0;
    zxn_frame_wait_vblank();
    CHECK(reads == 4);

    test_reg_read_hook = NULL;
    return test_summary("frame");
}
