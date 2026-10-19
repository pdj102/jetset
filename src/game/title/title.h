#ifndef TITLE_H
#define TITLE_H

/**
 * @file title.h
 * @brief The title: the title message, then any key asks the player's
 *        name. The main loop (game/game.h) starts a game when it returns.
 *
 * Banking: everything here is banked (NGE_BANKED, page 104), as it runs
 * once per game; the module has no common part. Don't call it while game
 * code has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"

/**
 * @brief Show the title, with no level: no entity sprites, the map window
 *        cleared. Wait for a key, ask the player's name (running frames
 *        until ENTER), greet them and return.
 */
void title_run(void) NGE_BANKED;

#endif
