#ifndef NGE_KBD_STREAM_H
#define NGE_KBD_STREAM_H

/**
 * @file nge_kbd_stream.h
 * @brief Input stream on the keyboard: the characters of key events.
 *
 * nge_getc() takes key events (input/nge_input.h) and returns the first
 * with a character (nge_key_ascii()), or NGE_EOF when no event is waiting.
 * Events without a character, such as EXTEND, are skipped. The events come
 * from nge_input_update(), which must run once per frame.
 */

#include "io/nge_stream.h"

/**
 * @brief Open an input stream on the keyboard.
 * @return The stream, or NULL if no stream slot is free.
 */
NGE_FILE *nge_open_keyboard(void);

#endif
