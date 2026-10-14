#ifndef NGE_INPUT_H
#define NGE_INPUT_H

/**
 * @file nge_input.h
 * @brief Keyboard input: a per-frame snapshot of the keys, and key events.
 *
 * Call nge_input_update() once per frame, straight after
 * nge_frame_wait_vblank(). It reads the 40 keys of the Spectrum keyboard and
 * the Next's extended keys (cursor keys, DELETE, EDIT...) into a snapshot,
 * and queues an event for each key that went down. Nothing reads the
 * keyboard in between: interrupts are off, so the ROM never scans it.
 *
 * Two ways to read it, which can be mixed:
 *
 * - Keys held (action games): nge_key_down(k), nge_key_pressed(k) (went
 *   down this frame), nge_key_released(k). Every caller sees the same
 *   snapshot for the whole frame.
 * - Key events (commands, menus, text): nge_key_get() takes the next event,
 *   or NGE_KEY_NONE. Events keep their order, each is taken once, and a
 *   held key repeats after a delay. A repeat is queued only while the queue
 *   is empty, so repeats never pile up while the game is busy. Up to
 *   NGE_KEY_QUEUE events wait; later ones are dropped.
 *   nge_key_wait() is the blocking version.
 *
 * Text: nge_key_ascii() turns an event into a character, and the keyboard
 * stream (io/nge_kbd_stream.h) and line editor (io/nge_line.h) build on it.
 *
 * Banking: init, set_repeat, ascii and wait are banked (NGE_BANKED, page
 * 88); update and the queries are in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"

/** Events held at once. */
#define NGE_KEY_QUEUE  8

/** Frames before a held key repeats, and between repeats, set by nge_input_init(). */
#define NGE_KEY_REPEAT_DELAY  15
#define NGE_KEY_REPEAT_RATE   4

/*
 * Key codes: row * 8 + bit. Rows 0-7 are the keyboard half-rows (ports
 * $FEFE to $7FFE), bits 0-4; rows 8 and 9 are the extended keys (Next
 * registers 0xB0 and 0xB1).
 */
#define NGE_KEY_CAPS     0
#define NGE_KEY_Z        1
#define NGE_KEY_X        2
#define NGE_KEY_C        3
#define NGE_KEY_V        4
#define NGE_KEY_A        8
#define NGE_KEY_S        9
#define NGE_KEY_D        10
#define NGE_KEY_F        11
#define NGE_KEY_G        12
#define NGE_KEY_Q        16
#define NGE_KEY_W        17
#define NGE_KEY_E        18
#define NGE_KEY_R        19
#define NGE_KEY_T        20
#define NGE_KEY_1        24
#define NGE_KEY_2        25
#define NGE_KEY_3        26
#define NGE_KEY_4        27
#define NGE_KEY_5        28
#define NGE_KEY_0        32
#define NGE_KEY_9        33
#define NGE_KEY_8        34
#define NGE_KEY_7        35
#define NGE_KEY_6        36
#define NGE_KEY_P        40
#define NGE_KEY_O        41
#define NGE_KEY_I        42
#define NGE_KEY_U        43
#define NGE_KEY_Y        44
#define NGE_KEY_ENTER    48
#define NGE_KEY_L        49
#define NGE_KEY_K        50
#define NGE_KEY_J        51
#define NGE_KEY_H        52
#define NGE_KEY_SPACE    56
#define NGE_KEY_SYM      57
#define NGE_KEY_M        58
#define NGE_KEY_N        59
#define NGE_KEY_B        60
/* Extended keys, register 0xB0 */
#define NGE_KEY_RIGHT      64
#define NGE_KEY_LEFT       65
#define NGE_KEY_DOWN       66
#define NGE_KEY_UP         67
#define NGE_KEY_PERIOD     68
#define NGE_KEY_COMMA      69
#define NGE_KEY_QUOTE      70
#define NGE_KEY_SEMICOLON  71
/* Extended keys, register 0xB1 */
#define NGE_KEY_EXTEND     72
#define NGE_KEY_CAPSLOCK   73
#define NGE_KEY_GRAPH      74
#define NGE_KEY_TRUEVIDEO  75
#define NGE_KEY_INVVIDEO   76
#define NGE_KEY_BREAK      77
#define NGE_KEY_EDIT       78
#define NGE_KEY_DELETE     79

/** Number of key codes. */
#define NGE_KEY_COUNT  80

/*
 * Events: the key code in the low byte, the shift keys held when it went
 * down in the high byte. CAPS SHIFT and SYMBOL SHIFT on their own are not
 * events.
 */
typedef uint16_t nge_key_event_t;

#define NGE_MOD_CAPS  0x01   /**< CAPS SHIFT was held. */
#define NGE_MOD_SYM   0x02   /**< SYMBOL SHIFT was held. */

#define NGE_KEY_NONE  0xFFFF   /**< No event. */

#define NGE_KEY_CODE(ev)  ((uint8_t)(ev))
#define NGE_KEY_MODS(ev)  ((uint8_t)((ev) >> 8))

/* ---- Common memory: per frame ---- */

/** @brief Read the keyboard and queue events: once per frame. */
void nge_input_update(void);

/** @brief 1 if key k is held. */
uint8_t nge_key_down(uint8_t k);

/** @brief 1 if key k went down this frame. */
uint8_t nge_key_pressed(uint8_t k);

/** @brief 1 if key k was released this frame. */
uint8_t nge_key_released(uint8_t k);

/** @brief Take the next event, without waiting: NGE_KEY_NONE if there is none. */
nge_key_event_t nge_key_get(void);

/** @brief Throw away waiting events (for example when a menu opens or closes). */
void nge_key_flush(void);

/* ---- Banked ---- */

/**
 * @brief Clear the snapshot and the queue, set the default repeat, and stop
 *        the extended keys also showing in the 8x5 matrix (register 0x68
 *        bit 4: otherwise UP also reads as CAPS+7).
 */
void nge_input_init(void) NGE_BANKED;

/**
 * @brief Set the repeat of a held key, in frames.
 * @param delay Frames before the first repeat; 0 turns repeat off.
 * @param rate Frames between repeats (at least 1).
 */
void nge_input_set_repeat(uint8_t delay, uint8_t rate) NGE_BANKED;

/**
 * @brief The character of an event, as the Spectrum keyboard types it with
 *        its shift keys (z88dk's key translation table).
 *
 * Extended keys: ; " , . as themselves; LEFT 8, RIGHT 9, DOWN 10, UP 11,
 * DELETE 12, EDIT 7. CAPS+0 is also DELETE (12), ENTER is 13.
 * @return The character, or 0 if the key has none.
 */
uint8_t nge_key_ascii(nge_key_event_t ev) NGE_BANKED;

/**
 * @brief Wait for the next event, running frames (nge_frame_wait_vblank()
 *        and nge_input_update(), nothing else) until one arrives.
 * @param frames Frames to wait at most; 0 waits for ever.
 * @return The event, or NGE_KEY_NONE after the timeout.
 */
nge_key_event_t nge_key_wait(uint16_t frames) NGE_BANKED;

#endif
