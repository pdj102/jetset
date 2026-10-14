/*
 * Keyboard input in common memory: the per-frame update, which reads the
 * keyboard into the snapshot and queues events, and the queries. The setup,
 * ASCII and waiting functions are banked, in nge_input_setup.c.
 */
#include "input/nge_input_internal.h"

#include <arch/zxn.h>
#include <z80.h>

#include "core/nge_util.h"

#define NO_KEY  ((uint8_t)NGE_KEY_NONE)

uint8_t nge_input_now[NGE_INPUT_ROWS];
uint8_t nge_input_prev[NGE_INPUT_ROWS];

nge_key_event_t nge_input_queue[NGE_KEY_QUEUE];
uint8_t nge_input_head;
uint8_t nge_input_count;

uint8_t nge_input_repeat_key = NO_KEY;
uint8_t nge_input_repeat_mods;
uint8_t nge_input_repeat_timer;
uint8_t nge_input_repeat_delay = NGE_KEY_REPEAT_DELAY;
uint8_t nge_input_repeat_rate = NGE_KEY_REPEAT_RATE;

static const uint8_t bit_mask[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

/* Add an event; dropped if the queue is full */
static void push(nge_key_event_t ev)
{
    if (nge_input_count < NGE_KEY_QUEUE) {
        nge_input_queue[(nge_input_head + nge_input_count) & (NGE_KEY_QUEUE - 1)] = ev;
        nge_input_count++;
    }
}

typedef char nge_key_queue_check[(NGE_KEY_QUEUE & (NGE_KEY_QUEUE - 1)) == 0 ? 1 : -1];

static uint8_t held(uint8_t k)
{
    return (nge_input_now[k >> 3] & bit_mask[k & 7]) != 0;
}

void nge_input_update(void)
{
    uint8_t r;
    uint8_t bit;
    uint8_t k;
    uint8_t down;
    uint8_t mods;
    uint8_t half_row = 0xFE;   /* high byte of the port: one 0 bit selects a half-row */
    uint8_t new_key = 0;

    for (r = 0; r < NGE_INPUT_ROWS; r++) {
        nge_input_prev[r] = nge_input_now[r];
    }
    /* Keyboard half-rows: active low, 5 keys each */
    for (r = 0; r < 8; r++) {
        nge_input_now[r] = (uint8_t)~z80_inp(((uint16_t)half_row << 8) | 0xFE) & 0x1F;
        half_row = (uint8_t)((half_row << 1) | 1);
    }
    /* Extended keys: active high */
    nge_input_now[8] = ZXN_READ_REG(NGE_REG_EXT_KEYS_0);
    nge_input_now[9] = ZXN_READ_REG(NGE_REG_EXT_KEYS_1);

    mods = (held(NGE_KEY_CAPS) ? NGE_MOD_CAPS : 0) | (held(NGE_KEY_SYM) ? NGE_MOD_SYM : 0);

    /* An event for each key that went down, in scan order */
    for (r = 0; r < NGE_INPUT_ROWS; r++) {
        down = nge_input_now[r] & (uint8_t)~nge_input_prev[r];
        for (bit = 0; down != 0; bit++, down >>= 1) {
            if ((down & 1) == 0) {
                continue;
            }
            k = (uint8_t)(r * 8 + bit);
            if (k == NGE_KEY_CAPS || k == NGE_KEY_SYM) {
                continue;
            }
            push(((nge_key_event_t)mods << 8) | k);
            /* The last key to go down is the one that repeats, with the same shift keys */
            nge_input_repeat_key = k;
            nge_input_repeat_mods = mods;
            nge_input_repeat_timer = nge_input_repeat_delay;
            new_key = 1;
        }
    }

    /*
     * Repeat a held key; only into an empty queue, so repeats never pile up.
     * A repeat has the shift keys of the press, not the ones held now, so
     * holding 5 and then pressing CAPS SHIFT keeps repeating 5, not LEFT.
     */
    if (nge_input_repeat_key != NO_KEY && !new_key) {
        if (!held(nge_input_repeat_key)) {
            nge_input_repeat_key = NO_KEY;
        } else if (nge_input_repeat_delay != 0 && --nge_input_repeat_timer == 0) {
            if (nge_input_count == 0) {
                push(((nge_key_event_t)nge_input_repeat_mods << 8) | nge_input_repeat_key);
            }
            nge_input_repeat_timer = nge_input_repeat_rate;
        }
    }
}

uint8_t nge_key_down(uint8_t k)
{
    nge_util_assert(k < NGE_KEY_COUNT);

    return held(k);
}

uint8_t nge_key_pressed(uint8_t k)
{
    nge_util_assert(k < NGE_KEY_COUNT);

    return held(k) && (nge_input_prev[k >> 3] & bit_mask[k & 7]) == 0;
}

uint8_t nge_key_released(uint8_t k)
{
    nge_util_assert(k < NGE_KEY_COUNT);

    return !held(k) && (nge_input_prev[k >> 3] & bit_mask[k & 7]) != 0;
}

nge_key_event_t nge_key_get(void)
{
    nge_key_event_t ev;

    if (nge_input_count == 0) {
        return NGE_KEY_NONE;
    }
    ev = nge_input_queue[nge_input_head];
    nge_input_head = (nge_input_head + 1) & (NGE_KEY_QUEUE - 1);
    nge_input_count--;
    return ev;
}

void nge_key_flush(void)
{
    nge_input_count = 0;
}
