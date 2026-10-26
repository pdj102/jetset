/*
 * The help line: common memory, so banked callers can pass their literals
 * (N-BANK-5).
 */
#include "game/ui/screen.h"

#include <stddef.h>

#include "io/nge_print.h"
#include "io/nge_text.h"

#include "game/ui/screen_internal.h"

NGE_FILE *screen_help_lines[2] = { NULL, NULL };   /* initialised: screen_init() closes them */
uint8_t screen_depth;

void screen_help(const char *text)
{
    NGE_FILE *help = screen_help_lines[(screen_depth != 0) ? SCREEN_BUFFER_OVERLAY : SCREEN_BUFFER_MAIN];

    nge_text_cls(help);
    nge_text_ink(NGE_INK_CYAN, help);
    nge_puts(text, help);
}
