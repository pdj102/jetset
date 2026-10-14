#ifndef NGE_INPUT_INTERNAL_H
#define NGE_INPUT_INTERNAL_H

/*
 * Shared by nge_input.c (common memory: per-frame update and queries) and
 * nge_input_setup.c (banked: setup, ASCII and waiting). Not for use outside
 * the input module.
 */

#include "input/nge_input.h"

/* Snapshot rows: 8 keyboard half-rows, then extended keys 0xB0 and 0xB1 */
#define NGE_INPUT_ROWS  10

/* Next registers: extended keys, and ULA control bit 4 (cancel extended keys in the 8x5 matrix) */
#define NGE_REG_EXT_KEYS_0         0xB0
#define NGE_REG_EXT_KEYS_1         0xB1
#define NGE_REG_ULA_CONTROL        0x68
#define NGE_RUC_CANCEL_EXT_MATRIX  0x10

extern uint8_t nge_input_now[NGE_INPUT_ROWS];    /* keys held, 1 = down */
extern uint8_t nge_input_prev[NGE_INPUT_ROWS];   /* last frame's */

extern nge_key_event_t nge_input_queue[NGE_KEY_QUEUE];
extern uint8_t nge_input_head;    /* next event to take */
extern uint8_t nge_input_count;   /* events waiting */

extern uint8_t nge_input_repeat_key;     /* key that repeats, or NGE_KEY_NONE's low byte */
extern uint8_t nge_input_repeat_mods;    /* shift keys held when it went down: repeats keep them */
extern uint8_t nge_input_repeat_timer;   /* frames to the next repeat */
extern uint8_t nge_input_repeat_delay;   /* 0: no repeat */
extern uint8_t nge_input_repeat_rate;

#endif
