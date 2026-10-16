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
#include "input/nge_input.h"
#include "io/nge_kbd_stream.h"
#include "io/nge_line.h"
#include "io/nge_print.h"
#include "io/nge_tm_stream.h"

#include "game/game.h"
#include "game/screen.h"

/*
 * Ask for the player's name in the message window and read it from the
 * keyboard into screen_name (blocking: runs frames until ENTER). An empty
 * name keeps the current one.
 */
static void ask_name(void)
{
    char name[SCREEN_NAME_MAX + 1];
    NGE_FILE *keyboard = nge_open_keyboard();

    nge_util_assert(keyboard != NULL);
    screen_message_line();
    nge_tm_ink(NGE_INK_YELLOW, screen_messages);
    nge_puts("What is your name? ", screen_messages);
    nge_tm_ink(NGE_INK_WHITE, screen_messages);
    nge_gets(name, sizeof(name), keyboard, screen_messages);
    nge_close(keyboard);

    if (name[0] != 0) {
        strcpy(screen_name, name);
    }
}

void title_enter(void) NGE_BANKED
{
    screen_message("NGE ROGUE. Press any key to start.");
}

void title_update(void) NGE_BANKED
{
    if (nge_key_get() == NGE_KEY_NONE) {
        return;
    }
    ask_name();
    screen_message("Welcome to the dungeon.");
    screen_message("Move: cursor keys or h j k l.");
    screen_message("i inventory, f fire, Q quit.");
    game_set_state(GAME_NEW_GAME);
}
