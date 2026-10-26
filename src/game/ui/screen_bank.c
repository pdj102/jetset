/*
 * Opening and closing overlay screens: banked in page 116 with the other
 * screens (a banked game module, see "Banked game modules" in
 * docs/memory_map.md).
 */
#ifdef __SDCC
#pragma codeseg PAGE_116
#pragma constseg PAGE_116
#endif

#include "game/ui/screen.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "io/nge_l2_stream.h"
#include "io/nge_text.h"
#include "layer2/nge_layer2.h"

#include "game/game.h"
#include "game/ui/screen_internal.h"

const nge_rect_t screen_help_area = { .x1 = 0, .x2 = 39, .y1 = SCREEN_HELP_ROW, .y2 = SCREEN_HELP_ROW };

void screen_open(void) NGE_BANKED
{
    NGE_FILE *help;

    nge_util_assert(screen_depth < SCREEN_DEPTH_MAX);

    if (screen_depth == 0) {
        help = nge_open_layer2(SCREEN_BUFFER_OVERLAY, &screen_help_area, SCREEN_PAPER);
        nge_util_assert(help != NULL);
        nge_text_clip(1, help);
        nge_text_cls(help);
        screen_help_lines[SCREEN_BUFFER_OVERLAY] = help;
    }
    screen_depth++;
    nge_key_flush();   /* keys typed for what was shown don't act here */
}

void screen_frame(void) NGE_BANKED
{
    game_pause_frame();
    nge_layer2_show_buffer((screen_depth != 0) ? SCREEN_BUFFER_OVERLAY : SCREEN_BUFFER_MAIN);
}

nge_key_event_t screen_wait_key(void) NGE_BANKED
{
    nge_key_event_t ev;

    do {
        screen_frame();
        ev = nge_key_get();
    } while (ev == NGE_KEY_NONE);
    return ev;
}

void screen_close(void) NGE_BANKED
{
    nge_util_assert(screen_depth != 0);

    screen_depth--;
    if (screen_depth == 0) {
        nge_close(screen_help_lines[SCREEN_BUFFER_OVERLAY]);
        screen_help_lines[SCREEN_BUFFER_OVERLAY] = NULL;
        screen_frame();   /* the main screen, from this blanking period */
    }
    nge_key_flush();      /* a key held down doesn't act on what is shown now */
}
