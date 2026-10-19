/*
 * The title state: banked in page 104 with its literals, as it runs once per
 * game (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). It passes its literals only to common code (the
 * screen and output functions).
 */
#ifdef __SDCC
#pragma codeseg PAGE_104
#pragma constseg PAGE_104
#endif

#include "game/title/title.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"
#include "ecs/systems/nge_render.h"
#include "input/nge_input.h"
#include "io/nge_kbd_stream.h"
#include "io/nge_line.h"
#include "io/nge_print.h"
#include "io/nge_tm_stream.h"

#include "game/game.h"
#include "game/screen.h"

/*
 * Ask for the player's name in the message window and read it from the
 * keyboard into screen_name, running the game's frames until ENTER (the
 * screen stays live). An empty name keeps the current one.
 */
static void ask_name(void)
{
    char name[SCREEN_NAME_MAX + 1];   /* on the stack: common memory, as the line editor needs */
    NGE_FILE *keyboard = nge_open_keyboard();
    nge_line_t line;
    uint8_t done;

    nge_util_assert(keyboard != NULL);
    screen_help("Type a name, then ENTER");
    screen_message_line();
    nge_tm_ink(NGE_INK_YELLOW, screen_messages);
    nge_puts("What is your name? ", screen_messages);
    nge_tm_ink(NGE_INK_WHITE, screen_messages);
    nge_line_begin(&line, name, sizeof(name), keyboard, screen_messages);
    do {
        game_frame_start();
        done = nge_line_update(&line);
        game_frame_end();
    } while (!done);
    nge_close(keyboard);

    if (name[0] != 0) {
        strcpy(screen_name, name);
    }
}

void title_run(void) NGE_BANKED
{
    /* No level on the title: no entity sprites, and the map window cleared */
    nge_render_set_view(NGE_RENDER_NO_VIEW);
    nge_tm_cls(screen_panel);
    nge_key_flush();
    screen_message("NGE ROGUE. Press any key to start.");
    screen_help("Any key: start a new game");
    game_wait_key(0);

    ask_name();
    screen_message("Welcome to the dungeon.");
    screen_message("Move: cursor keys or h j k l.");
    screen_message("i inv, d drop, f fire, x look, Q quit.");
}
