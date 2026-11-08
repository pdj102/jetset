/*
 * The title screen: banked in page 116 with the other screens and their
 * literals. It passes its literals only to common code (the screen, the
 * output path, the panels).
 */
#ifdef __SDCC
#pragma codeseg PAGE_116
#pragma constseg PAGE_116
#endif

#include "game/ui/screen_title.h"

#include "core/nge_util.h"
#include "io/nge_text.h"
#include "ui/nge_panel.h"

#include "game/ui/screen.h"

/* The title's panels, by PANEL_* */
#define PANEL_TITLE   0
#define PANEL_COUNT   1

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_TITLE]  = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

#define ROW_NAME   11   /* the title panel's row of the game's name */
#define ROW_START  14   /* ...of what starts it */

void screen_title_run(void) NGE_BANKED
{
    NGE_FILE *streams[PANEL_COUNT];
    uint8_t ok;

    screen_open();
    ok = nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, PANEL_COUNT, streams);
    nge_util_assert(ok);
    nge_text_ink(NGE_INK_YELLOW, streams[PANEL_TITLE]);
    nge_panel_centre(&panels[PANEL_TITLE], ROW_NAME, "NGE ROGUE", streams[PANEL_TITLE]);
    nge_text_ink(NGE_INK_WHITE, streams[PANEL_TITLE]);
    nge_panel_centre(&panels[PANEL_TITLE], ROW_START, "Press any key to start", streams[PANEL_TITLE]);
    screen_help("Any key: start a new game");
    screen_wait_key();

    nge_panel_close_all(streams, PANEL_COUNT);
    screen_close();
}
