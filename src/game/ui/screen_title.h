#ifndef SCREEN_TITLE_H
#define SCREEN_TITLE_H

/**
 * @file screen_title.h
 * @brief The title: an overlay screen (game/ui/screen.h) with the game's
 *        name; any key asks the player's name. The main loop (game/game.h)
 *        starts a game when it returns.
 *
 *     rows 0-27   the title
 *     rows 28-30  the prompt: the player's name, typed
 *     row  31     the help line
 *
 * Banking: banked (NGE_BANKED, page 116), with the other screens. Don't
 * call it while game code has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"

/**
 * @brief Show the title over everything, wait for a key, ask the player's
 *        name (into screen_name, game/ui/screen_main.h) until ENTER, then
 *        close it and greet them in the message window.
 */
void screen_title_run(void) NGE_BANKED;

#endif
