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

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_render.h"
#include "map/nge_map.h"
#include "sprite/nge_sprite.h"

#include "game/actors.h"
#include "game/game.h"
#include "game/player/player.h"
#include "game/screen.h"

/* Characters of the keys that choose (nge_key_ascii) */
#define CHAR_ENTER  13
#define CHAR_SPACE  ' '

/* What each purpose asks, and its keys on the help bar */
typedef struct target_purpose {
    const char *prompt;
    const char *help;
} target_purpose_t;

static const target_purpose_t purposes[TARGET_PURPOSE_COUNT] = {
    [TARGET_LOOK] = { "Look at what?", "Arrows: move  ENTER: look  Other: back" },
};

uint8_t target_in_range(const target_t *t) NGE_BANKED
{
    return nge_map_within(t->origin_x, t->origin_y, t->x, t->y, t->range);
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
        }
        return TARGET_STEP_MOVING;
    }
    c = nge_key_ascii(ev);
    if (c == CHAR_ENTER || c == CHAR_SPACE) {
        if (target_in_range(t)) {
            return TARGET_STEP_CHOSEN;
        }
        screen_message("That is out of range.");
        return TARGET_STEP_MOVING;
    }
    screen_message("Never mind.");
    t->x = NGE_MAP_X_INVALID;
    t->y = NGE_MAP_Y_INVALID;
    return TARGET_STEP_CANCELLED;
}

/* The cursor's sprite on the cursor's tile, through the camera: green in range, red out of it */
static void place(nge_sprite_attr_t *cursor, const target_t *t)
{
    uint16_t sx;
    uint16_t sy;

    nge_map_to_screen((uint16_t)t->x * NGE_MAP_TILE_PIXELS, (uint16_t)t->y * NGE_MAP_TILE_PIXELS, &sx, &sy);
    nge_sprite_set(cursor, sx, sy, target_in_range(t) ? DEMO_SPRITE_CURSOR_IN : DEMO_SPRITE_CURSOR_OUT, 0,
                   NGE_SPRITE_SCALE_1);
}

uint8_t target_choose(target_t *t) NGE_BANKED
{
    nge_sprite_attr_t cursor;   /* on the stack: common memory, where nge_sprite_upload() can read it */
    nge_key_event_t ev;
    uint8_t step = TARGET_STEP_MOVING;

    nge_util_assert(t->purpose < TARGET_PURPOSE_COUNT);
    nge_util_assert(t->x < nge_map.width && t->y < nge_map.height);

    nge_key_flush();   /* keys typed for play don't move the cursor */
    screen_message(purposes[t->purpose].prompt);
    screen_help(purposes[t->purpose].help);
    place(&cursor, t);

    /* Frames of its own: presentation only, so no time passes (game/game.h) */
    while (step == TARGET_STEP_MOVING) {
        game_frame_start();
        nge_sprite_upload(SCREEN_SPRITE_CURSOR, &cursor, 1);
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE) {
            step = target_key(t, ev);
            if (step == TARGET_STEP_MOVING) {
                place(&cursor, t);
            }
        }
        nge_render_update();   /* the entities' sprites, through the camera if it moved */
    }

    nge_sprite_clear(&cursor, 1);
    nge_sprite_upload(SCREEN_SPRITE_CURSOR, &cursor, 1);   /* hidden now: one sprite, so not worth waiting for vblank */
    nge_map_centre_camera(NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));
    game_play_help();
    nge_key_flush();   /* a choosing key held down doesn't leak into play */
    return step == TARGET_STEP_CHOSEN;
}
