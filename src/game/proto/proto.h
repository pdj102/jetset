#ifndef PROTO_H
#define PROTO_H

/**
 * @file proto.h
 * @brief Development prototypes (GAME_DEV_KIT only, game/dev.h): small
 *        experiments run from the title, kept apart from the game.
 *
 * The Layer 2 UI prototype (docs/game_loop.md, step 5) shows UI panels on
 * Layer 2 over a tilemap that scrolls a pixel at a time under the arrow
 * keys, with overlay sprites that stay on their tiles. It times the Layer 2
 * drawing and shows the times in the panel and on the CSpect console.
 * Shift+L on the title runs it; ENTER returns to the title.
 *
 * Banking: banked (NGE_BANKED, page 126). Don't call it while game code
 * has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"
#include "game/dev.h"

#if GAME_DEV_KIT
/**
 * @brief Run the Layer 2 UI prototype until ENTER, then put the screen
 *        back as the title expects it (screen_init(), Layer 2 hidden).
 */
void proto_layer2_run(void) NGE_BANKED;
#endif

#endif
