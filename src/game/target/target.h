#ifndef TARGET_H
#define TARGET_H

/**
 * @file target.h
 * @brief Targeting: the player moves a cursor over the map and chooses a
 *        tile, or cancels. Used by commands that need a tile, not a
 *        direction (look, fire and aim; later cast). See docs/targeting.md.
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
 * With an action (ACT_*, game/actions/act.h), the cursor is coloured and
 * ENTER refused by that action's check at the cursor's tile, so the cursor
 * and the action never disagree (docs/targeting.md section 9). Without one
 * (ACT_NONE: look), by the range alone, measured from the origin with
 * nge_map_within() (map/nge_map.h), the game's one range test. Every caller
 * gets the same prompt and help, which this module owns.
 *
 * Banking: banked (NGE_BANKED, page 124): don't call these while game code
 * has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "input/nge_input.h"

#include "game/actions/act.h"

/** Range for any tile on the map: nge_map_within() holds for every tile at 255. */
#define TARGET_RANGE_ANY  0xFF

/** Tiles kept between the cursor and the view's edge: closer, and the view scrolls. */
#define TARGET_SCROLL_MARGIN  4

/* target_key()'s answers */
#define TARGET_STEP_MOVING     0   /**< Still targeting: the cursor moved, or a choice was refused. */
#define TARGET_STEP_CHOSEN     1   /**< The cursor's tile was chosen. */
#define TARGET_STEP_CANCELLED  2   /**< Cancelled: x and y are invalid. */

/** A targeting context: the caller's set-up, and the answer. */
typedef struct target {
    uint8_t action;     /**< In: ACT_* checked at the cursor's tile, or ACT_NONE: the range alone. */
    const act_t *act;   /**< In: the action's context (actor, item, spell...), or NULL; not changed. */
    uint8_t origin_x;   /**< In: without an action, the tile the range is measured from. */
    uint8_t origin_y;
    uint8_t range;      /**< In: without an action, the furthest range, or TARGET_RANGE_ANY. */
    uint8_t x;          /**< In: the cursor's start, on the map. Out: the tile chosen, or NGE_MAP_X_INVALID. */
    uint8_t y;          /**< In: the cursor's start, on the map. Out: the tile chosen, or NGE_MAP_Y_INVALID. */
    uint8_t reason;     /**< Private: the last check's ACT_R_* (ACT_R_OUT_OF_RANGE or ACT_R_NONE without an action). */
} target_t;

/**
 * @brief Run targeting until the player chooses a tile the check passes
 *        (ENTER or SPACE) or cancels (any other key but a movement key). The
 *        cursor is coloured by target_cursor_bank(); the view scrolls to keep it
 *        TARGET_SCROLL_MARGIN tiles inside. Waiting key events are thrown
 *        away at the start and the end; at the end the camera is back on
 *        the player and play's help is shown.
 * @return 1 with the tile in t->x, t->y; 0 with them NGE_MAP_X_INVALID and NGE_MAP_Y_INVALID.
 */
uint8_t target_choose(target_t *t) NGE_BANKED;

/**
 * @brief Targeting's answer to one key event: a movement key moves the
 *        cursor (not off the map), scrolls the view if the cursor comes
 *        into the margin and checks the new tile (target_check()); ENTER or
 *        SPACE chooses the tile, or says why not (the action's message, or
 *        "That is out of range." without one); any other key cancels.
 *        t->reason must be the cursor's tile's: target_check() first.
 * @return TARGET_STEP_*.
 */
uint8_t target_key(target_t *t, nge_key_event_t ev) NGE_BANKED;

/**
 * @brief Check the cursor's tile, into t->reason: the action's check
 *        (act_do() in ACT_CHECK mode) on a copy of t->act with x and y the
 *        cursor's, or without an action ACT_R_OUT_OF_RANGE or ACT_R_NONE by
 *        target_in_range().
 */
void target_check(target_t *t) NGE_BANKED;

/** @brief 1 if the cursor's tile is within range of the origin (nge_map_within()). */
uint8_t target_in_range(const target_t *t) NGE_BANKED;

/**
 * @brief The cursor's palette bank from t->reason (docs/targeting.md R20):
 *        BANK_GREEN, ENTER will work (ACT_R_NONE); BANK_RED, the tile can't
 *        be reached (ACT_R_OUT_OF_RANGE, ACT_R_NO_LINE); BANK_ORANGE, it is
 *        reached but ENTER will be refused (any other reason).
 */
uint8_t target_cursor_bank(const target_t *t) NGE_BANKED;

#endif
