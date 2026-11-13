/*
 * Targeting: banked in page 124 with its literals (a banked game module, see
 * "Banked game modules" in docs/memory_map.md). It runs while the player
 * chooses a tile, not every frame of play, and passes its literals only to
 * common code (the screen).
 */
#ifdef __SDCC
#pragma codeseg PAGE_124
#pragma constseg PAGE_124
#endif

#include "game/target/target.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/components/nge_location.h"
#include "map/nge_map.h"
#include "sprite/nge_sprite.h"

#include "game/actors.h"
#include "game/game.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/ui/screen.h"
#include "game/ui/screen_main.h"

/* Characters of the keys that choose (nge_key_ascii) */
#define CHAR_ENTER  13
#define CHAR_SPACE  ' '

/* Every caller's prompt and keys (docs/targeting.md D2) */
static const char prompt[] = "Select target";
static const char help[] = "Arrows: move  ENTER: target  Other: back";

/* One check a move, kept for the cursor's colour and ENTER (D6). The check changes nothing (act.h), so a copy is
   enough to keep the caller's context as it was */
void target_check(target_t *t) NGE_BANKED
{
    act_t a;
    uint8_t action = t->action;   /* in a local, read before the copy (T-ZSDCC-1) */

    a = *t->act;
    a.x = t->x;
    a.y = t->y;
    act_do(action, &a, ACT_CHECK);
    t->reason = a.reason;
}

uint8_t target_cursor_bank(const target_t *t) NGE_BANKED
{
    uint8_t reason = t->reason;   /* in locals (T-ZSDCC-1) */

    if (reason == ACT_R_NONE) {
        return BANK_GREEN;
    }
    if (reason == ACT_R_OUT_OF_RANGE || reason == ACT_R_NO_LINE) {
        return BANK_RED;   /* can't be reached */
    }
    return BANK_ORANGE;    /* reached, but refused: no target, occupied (blink, D8), anything else */
}

/*
 * Scroll the view one tile towards the cursor if it is within the margin of
 * an edge. nge_map_move_camera() stops at the map's edges, so there the
 * cursor goes on to the view's edge
 */
static void scroll(const target_t *t)
{
    int8_t dx = 0;
    int8_t dy = 0;

    if (t->x < nge_map.cam_x + TARGET_SCROLL_MARGIN) {
        dx = -1;
    } else if (t->x + TARGET_SCROLL_MARGIN >= nge_map.cam_x + nge_map.view_w) {
        dx = 1;
    }
    if (t->y < nge_map.cam_y + TARGET_SCROLL_MARGIN) {
        dy = -1;
    } else if (t->y + TARGET_SCROLL_MARGIN >= nge_map.cam_y + nge_map.view_h) {
        dy = 1;
    }
    if (dx != 0 || dy != 0) {
        nge_map_move_camera(dx, dy);
    }
}

/* Why ENTER was refused: the action's own message, with the check's context */
static void refused(const target_t *t)
{
    act_t a;
    uint8_t action = t->action;   /* in a local, read before the copy (T-ZSDCC-1) */

    a = *t->act;
    a.x = t->x;
    a.y = t->y;
    act_do(action, &a, ACT_CHECK);   /* again, for its outputs (a->target, a->item): the message names them */
    messages_act_failed(&a);
}

/* The cursor's keys: described in the help screen (game/ui/screen_guide_pdata.c) */
uint8_t target_key(target_t *t, nge_key_event_t ev) NGE_BANKED
{
    int8_t dx;
    int8_t dy;
    int16_t x;
    int16_t y;
    uint8_t c;

    if (player_key_direction(ev, &dx, &dy)) {
        x = (int16_t)t->x + dx;
        y = (int16_t)t->y + dy;
        if (x >= 0 && x < nge_map.width && y >= 0 && y < nge_map.height) {
            t->x = (uint8_t)x;
            t->y = (uint8_t)y;
            scroll(t);
            target_check(t);
        }
        return TARGET_STEP_MOVING;
    }
    c = nge_key_ascii(ev);
    if (c == CHAR_ENTER || c == CHAR_SPACE) {
        if (t->reason == ACT_R_NONE) {
            return TARGET_STEP_CHOSEN;
        }
        refused(t);
        return TARGET_STEP_MOVING;
    }
    screen_message("Never mind.");
    t->x = NGE_MAP_X_INVALID;
    t->y = NGE_MAP_Y_INVALID;
    return TARGET_STEP_CANCELLED;
}

/* The cursor (an overlay sprite) on the cursor's tile, through the camera: one pattern, its bank
   target_cursor_bank() */
static void place(const target_t *t)
{
    uint16_t sx;
    uint16_t sy;

    nge_map_to_screen((uint16_t)t->x * NGE_MAP_TILE_PIXELS, (uint16_t)t->y * NGE_MAP_TILE_PIXELS, &sx, &sy);
    screen_overlay_set(SCREEN_OVERLAY_CURSOR, sx, sy, SPRITE_CURSOR,
                       NGE_SPRITE_PALETTE(target_cursor_bank(t)));
}

uint8_t target_choose(target_t *t) NGE_BANKED
{
    uint8_t step = TARGET_STEP_MOVING;

    nge_util_assert(t->action < ACT_COUNT && t->act != NULL);
    nge_util_assert(t->x < nge_map.width && t->y < nge_map.height);

    nge_key_flush();   /* keys typed for play don't move the cursor */
    screen_message(prompt);
    screen_help(help);
    target_check(t);
    place(t);

    /* Frames that only draw, so no time passes (game_wait_key(), game/game.h) */
    while (step == TARGET_STEP_MOVING) {
        step = target_key(t, game_wait_key(0));
        if (step == TARGET_STEP_MOVING) {
            place(t);
        }
    }

    screen_overlay_hide(SCREEN_OVERLAY_CURSOR);
    nge_map_centre_camera(NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));
    game_play_help();
    nge_key_flush();   /* a choosing key held down doesn't leak into play */
    return step == TARGET_STEP_CHOSEN;
}
