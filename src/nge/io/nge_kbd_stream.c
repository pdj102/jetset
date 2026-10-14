#include "io/nge_kbd_stream.h"

#include <stddef.h>

#include "input/nge_input.h"
#include "io/nge_stream_dev.h"

/* Keyboard device input function: the next key event's character */
static int kbd_get(NGE_FILE *stream)
{
    nge_key_event_t ev;
    uint8_t c;

    (void)stream;
    while ((ev = nge_key_get()) != NGE_KEY_NONE) {
        c = nge_key_ascii(ev);
        if (c != 0) {
            return c;
        }
    }
    return NGE_EOF;
}

NGE_FILE *nge_open_keyboard(void)
{
    return nge_stream_alloc(NULL, kbd_get, NULL);
}
