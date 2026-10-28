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

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"
#include "io/nge_kbd_stream.h"
#include "io/nge_line.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "ui/nge_panel.h"

#include "game/components/pc.h"
#include "game/ui/screen.h"
#include "game/ui/screen_main.h"

/* The title's panels, by PANEL_* */
#define PANEL_TITLE   0
#define PANEL_PROMPT  1
#define PANEL_COUNT   2

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_TITLE]  = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 27 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_PROMPT] = { { .x1 = 0, .x2 = 39, .y1 = 28, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

#define ROW_NAME   11   /* the title panel's row of the game's name */
#define ROW_START  14   /* ...of what starts it */

/*
 * Ask for the player's name on the prompt panel and read it from the
 * keyboard into player_name, running paused frames until ENTER. An empty
 * name keeps the current one
 */
static void ask_name(NGE_FILE *prompt, char *player_name)
{
    char name[PC_NAME_MAX + 1];   /* on the stack: common memory, as the line editor needs */
    NGE_FILE *keyboard = nge_open_keyboard();
    nge_line_t line;
    uint8_t done;

    nge_util_assert(keyboard != NULL);
    screen_help("Type a name, then ENTER");
    nge_text_at(1, 1, prompt);
    nge_text_ink(NGE_INK_YELLOW, prompt);
    nge_puts("What is your name? ", prompt);
    nge_text_ink(NGE_INK_WHITE, prompt);
    nge_line_begin(&line, name, sizeof(name), keyboard, prompt);
    do {
        screen_frame();
        done = nge_line_update(&line);
    } while (!done);
    nge_close(keyboard);

    if (name[0] != 0) {
        strcpy(player_name, name);
    }
}

void screen_title_run(char *name) NGE_BANKED
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

    ask_name(streams[PANEL_PROMPT], name);
    nge_panel_close_all(streams, PANEL_COUNT);
    screen_close();

    screen_message("Welcome to the dungeon.");
    screen_message("Move: cursor keys or h j k l.");
    screen_message("i inv, d drop, f fire, x look, Q quit.");
}
