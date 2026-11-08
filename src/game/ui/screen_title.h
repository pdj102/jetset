#ifndef SCREEN_TITLE_H
#define SCREEN_TITLE_H

/**
 * @file screen_title.h
 * @brief The title: an overlay screen (game/ui/screen.h) with the game's
 *        name, until a key. The main loop (game/game.h) then starts a game,
 *        which opens with character creation (game/chargen/chargen.h):
 *        the class and the name are chosen there.
 *
 *     rows 0-30   the title
 *     row  31     the help line
 *
 * Banking: banked (NGE_BANKED, page 116), with the other screens. Don't
 * call it while game code has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"

/** @brief Show the title over everything, wait for a key, then close it. */
void screen_title_run(void) NGE_BANKED;

#endif
