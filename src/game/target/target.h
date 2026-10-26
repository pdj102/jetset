#ifndef TARGET_H
#define TARGET_H

/**
 * @file target.h
 * @brief Targeting: the player moves a cursor over the map and chooses a
 *        tile, or cancels. Used by commands that need a tile, not a
 *        direction (look; later fire and cast). See docs/targeting.md.
 *
 * The caller fills in a context (target_t, on its own stack) and calls
 * target_choose(), which returns when the player has chosen or cancelled:
 *
 *     if (target_choose(&t)) {
 *         ... use the tile t.x, t.y ...
 *     }
 *
 * It is a chooser (game/game.h): it waits with game_wait_key() until the
 * player answers, during the player's turn. The screen stays live, but no
 * time passes and no events are handled until it returns. The cursor is an
 * overlay sprite (SCREEN_OVERLAY_CURSOR, game/ui/screen_main.h).
 *
 * What a tile is wanted for (the purpose, TARGET_*) chooses the prompt and
 * the help text, which this module owns: a caller's banked literals would
 * be mapped out here. Range is measured from the origin with
 * nge_map_within() (map/nge_map.h), the game's one range test.
 *
 * Banking: banked (NGE_BANKED, page 124): don't call these while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "input/nge_input.h"

/** Range for any tile on the map: nge_map_within() holds for every tile at 255. */
#define TARGET_RANGE_ANY  0xFF

/** Tiles kept between the cursor and the view's edge: closer, and the view scrolls. */
#define TARGET_SCROLL_MARGIN  4

/* Purposes: what a tile is wanted for */
#define TARGET_LOOK           0   /**< Look at what is on a tile (messages_look()). */
#define TARGET_PURPOSE_COUNT  1

/* target_key()'s answers */
#define TARGET_STEP_MOVING     0   /**< Still targeting: the cursor moved, or a choice was refused. */
#define TARGET_STEP_CHOSEN     1   /**< The cursor's tile was chosen. */
#define TARGET_STEP_CANCELLED  2   /**< Cancelled: x and y are invalid. */

/** A targeting context: the caller's set-up, and the answer. */
typedef struct target {
    uint8_t purpose;    /**< In: TARGET_*. */
    uint8_t origin_x;   /**< In: the tile the range is measured from (usually the player's). */
    uint8_t origin_y;
    uint8_t range;      /**< In: the furthest range a tile can be chosen at, or TARGET_RANGE_ANY. */
    uint8_t x;          /**< In: the cursor's start, on the map. Out: the tile chosen, or NGE_MAP_X_INVALID. */
    uint8_t y;          /**< In: the cursor's start, on the map. Out: the tile chosen, or NGE_MAP_Y_INVALID. */
} target_t;

/**
 * @brief Run targeting until the player chooses a tile in range (ENTER or
 *        SPACE) or cancels (any other key but a movement key). The cursor
 *        is green in range and red out of it; the view scrolls to keep it
 *        TARGET_SCROLL_MARGIN tiles inside. Waiting key events are thrown
 *        away at the start and the end; at the end the camera is back on
 *        the player and play's help is shown.
 * @return 1 with the tile in t->x, t->y; 0 with them NGE_MAP_X_INVALID and NGE_MAP_Y_INVALID.
 */
uint8_t target_choose(target_t *t) NGE_BANKED;

/**
 * @brief Targeting's answer to one key event: a movement key moves the
 *        cursor (not off the map) and scrolls the view if the cursor comes
 *        into the margin; ENTER or SPACE chooses the tile, or says it is out
 *        of range; any other key cancels.
 * @return TARGET_STEP_*.
 */
uint8_t target_key(target_t *t, nge_key_event_t ev) NGE_BANKED;

/** @brief 1 if the cursor's tile is within range of the origin (nge_map_within()). */
uint8_t target_in_range(const target_t *t) NGE_BANKED;

#endif
