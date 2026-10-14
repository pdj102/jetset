/*
 * Keyboard input functions used rarely: banked in page 88 with their
 * constants (the ASCII tables). The per-frame update and the queries are in
 * nge_input.c.
 */
#ifdef __SDCC
#pragma codeseg PAGE_88
#pragma constseg PAGE_88
#endif

#include "input/nge_input_internal.h"

#include <arch/zxn.h>

#include "core/nge_util.h"
#include "hw/nge_frame.h"

/*
 * Characters of the 40 keyboard keys, indexed by half-row * 5 + bit, in four
 * sets of 40: unshifted, CAPS SHIFT, SYMBOL SHIFT, both. A copy of z88dk's
 * in_key_translation_table (libsrc/newlib/input/zx/z80), which emulates a
 * PC keyboard with CAPS + SYMBOL SHIFT as CTRL; 0 where its value is 255
 * (no character).
 */
static const uint8_t key_ascii[160] = {
    /* unshifted */
      0, 'z', 'x', 'c', 'v',   /* CAPS SHIFT, Z, X, C, V */
    'a', 's', 'd', 'f', 'g',   /* A, S, D, F, G */
    'q', 'w', 'e', 'r', 't',   /* Q, W, E, R, T */
    '1', '2', '3', '4', '5',   /* 1, 2, 3, 4, 5 */
    '0', '9', '8', '7', '6',   /* 0, 9, 8, 7, 6 */
    'p', 'o', 'i', 'u', 'y',   /* P, O, I, U, Y */
     13, 'l', 'k', 'j', 'h',   /* ENTER, L, K, J, H */
     32,   0, 'm', 'n', 'b',   /* SPACE, SYM SHIFT, M, N, B */
    /* CAPS SHIFT */
      0, 'Z', 'X', 'C', 'V',   /* CAPS SHIFT, Z, X, C, V */
    'A', 'S', 'D', 'F', 'G',   /* A, S, D, F, G */
    'Q', 'W', 'E', 'R', 'T',   /* Q, W, E, R, T */
      7,   6, 128, 129,   8,   /* 1, 2, 3, 4, 5 */
     12,   8,   9,  11,  10,   /* 0, 9, 8, 7, 6 */
    'P', 'O', 'I', 'U', 'Y',   /* P, O, I, U, Y */
     13, 'L', 'K', 'J', 'H',   /* ENTER, L, K, J, H */
     32,   0, 'M', 'N', 'B',   /* SPACE, SYM SHIFT, M, N, B */
    /* SYMBOL SHIFT */
      0, ':', '`', '?', '/',   /* CAPS SHIFT, Z, X, C, V */
    '~', '|', '\\', '{', '}',   /* A, S, D, F, G */
    131, 132, 133, '<', '>',   /* Q, W, E, R, T */
    '!', '@', '#', '$', '%',   /* 1, 2, 3, 4, 5 */
    '_', ')', '(', '\'', '&',   /* 0, 9, 8, 7, 6 */
    '"', ';', 130, ']', '[',   /* P, O, I, U, Y */
     13, '=', '+', '-', '^',   /* ENTER, L, K, J, H */
     32,   0, '.', ',', '*',   /* SPACE, SYM SHIFT, M, N, B */
    /* CAPS + SYMBOL SHIFT */
      0,  26,  24,   3,  22,   /* CAPS SHIFT, Z, X, C, V */
      1,  19,   4,   6,   7,   /* A, S, D, F, G */
     17,  23,   5,  18,  20,   /* Q, W, E, R, T */
     27,  28,  29,  30,  31,   /* 1, 2, 3, 4, 5 */
    127,   0, 134, '`', 135,   /* 0, 9, 8, 7, 6 */
     16,  15,   9,  21,  25,   /* P, O, I, U, Y */
     13,  12,  11,  10,   8,   /* ENTER, L, K, J, H */
     32,   0,  13,  14,   2,   /* SPACE, SYM SHIFT, M, N, B */
};

/* Extended keys, codes 64-79: ; " , . as themselves, cursor keys, DELETE, EDIT */
static const uint8_t ext_ascii[16] = {
    9, 8, 10, 11, '.', ',', '"', ';',   /* 0xB0: RIGHT LEFT DOWN UP . , " ; */
    0, 0, 0, 0, 0, 0, 7, 12,            /* 0xB1: EXTEND CAPSLOCK GRAPH TRUE INV BREAK EDIT DELETE */
};

void nge_input_init(void) NGE_BANKED
{
    uint8_t r;

    for (r = 0; r < NGE_INPUT_ROWS; r++) {
        nge_input_now[r] = 0;
        nge_input_prev[r] = 0;
    }
    nge_input_head = 0;
    nge_input_count = 0;
    nge_input_repeat_key = (uint8_t)NGE_KEY_NONE;
    nge_input_repeat_mods = 0;
    nge_input_repeat_delay = NGE_KEY_REPEAT_DELAY;
    nge_input_repeat_rate = NGE_KEY_REPEAT_RATE;

    ZXN_WRITE_REG(NGE_REG_ULA_CONTROL, ZXN_READ_REG(NGE_REG_ULA_CONTROL) | NGE_RUC_CANCEL_EXT_MATRIX);
}

void nge_input_set_repeat(uint8_t delay, uint8_t rate) NGE_BANKED
{
    nge_util_assert(rate >= 1);

    nge_input_repeat_delay = delay;
    nge_input_repeat_rate = rate;
}

uint8_t nge_key_ascii(nge_key_event_t ev) NGE_BANKED
{
    uint8_t k = NGE_KEY_CODE(ev);
    uint8_t mods = NGE_KEY_MODS(ev) & (NGE_MOD_CAPS | NGE_MOD_SYM);

    if (ev == NGE_KEY_NONE || k >= NGE_KEY_COUNT) {
        return 0;
    }
    if (k >= 64) {
        return ext_ascii[k - 64];
    }
    if ((k & 7) > 4) {
        return 0;   /* no such key: bits 5-7 of a half-row */
    }
    return key_ascii[mods * 40 + (k >> 3) * 5 + (k & 7)];
}

nge_key_event_t nge_key_wait(uint16_t frames) NGE_BANKED
{
    nge_key_event_t ev;
    uint8_t timed = frames != 0;

    while ((ev = nge_key_get()) == NGE_KEY_NONE) {
        if (timed && frames-- == 0) {
            break;
        }
        nge_frame_wait_vblank();
        nge_input_update();
    }
    return ev;
}
