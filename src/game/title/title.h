#ifndef TITLE_H
#define TITLE_H

/**
 * @file title.h
 * @brief The title state (GAME_TITLE, game/game.h): the title message, then
 *        any key asks the player's name and starts play.
 *
 * Banking: everything here is banked (NGE_BANKED, page 104), as it runs
 * once per game; the module has no common part. Don't call these while
 * game code has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"

/** @brief Entry action of the title state: show the title. */
void title_enter(void) NGE_BANKED;

/**
 * @brief The title state's update, once per frame: on any key, ask the
 *        player's name (blocking: the screen stands still while they type),
 *        draw the stats, show the help and go to GAME_PLAY.
 */
void title_update(void) NGE_BANKED;

#endif
