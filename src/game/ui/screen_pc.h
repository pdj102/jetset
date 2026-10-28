#ifndef SCREEN_PC_H
#define SCREEN_PC_H

/**
 * @file screen_pc.h
 * @brief The player character screen: an overlay screen (game/ui/screen.h)
 *        showing the player's character, their attributes and what is
 *        affecting them, at a glance. See docs/pc_screen.md.
 *
 *     col 0              19 20             39
 *     row 0  |   "Rogue the Fighter"           |   title
 *         2  | level, XP, next | the abilities |
 *            | gold, depth     | now, mod, base|
 *        10  | HP, MP, AC, speed: now, value, base; the melee attack |
 *        17  | the damage defences: immune, resist, vulnerable       |
 *        22  | the statuses                                          |
 *        24  | the active effects: turns left, saving throw          |
 *        30  | the colours' key                                      |
 *        31  | help line: SPACE:back                                 |
 *
 * It only shows: no cursor, no actions, no turn used. An attribute's value
 * is white at its base, cyan when better than its base, red when worse,
 * judged for the player (a lower speed is better; a vulnerability is
 * worse). Values include worn items' and active effects' modifiers; the
 * screen doesn't say which gave them.
 *
 * Banking: banked (NGE_BANKED, page 104: page 116 is full). Don't call it
 * while game code has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"

/** @brief Show the player character screen until SPACE. */
void screen_pc_run(void) NGE_BANKED;

#endif
