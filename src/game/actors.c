#include "game/actors.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "event/nge_event.h"
#include "game/events.h"
#include "game/timers.h"
#include "game/level/level.h"
#include "sprite/nge_sprite.h"

#define PLAYER_FRAME_RATE  25   /* frames per animation step: two steps a second */

/* The demo effect: every 5 turns, 4 times */
#define GLOW_PERIOD  (5 * NGE_TIMER_TICKS_PER_TURN)
#define GLOW_CYCLES  4

nge_entity_id_t actor_player;

void actors_init(void)
{
    nge_entity_id_t glow;

    /* The loaders map the graphics' page into slot 7 while they copy */
    nge_sprite_load_patterns(DEMO_SPRITE_PAGE, 0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
    nge_sprite_load_palette(DEMO_SPRITE_PAGE, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);

    /* The player, two frames to show the frame loop is running: placed by level_generate() */
    actor_player = nge_entity_create();
    nge_util_assert(actor_player != NGE_ENTITY_INVALID);
    nge_block_set(actor_player, NGE_BLOCK_OCCUPIED, NGE_BLOCK_NONE);
    nge_render_add(actor_player, DEMO_SPRITE_PLAYER, 0, NGE_SPRITE_SCALE_1, LAYER_PLAYER);
    nge_render_set_anim(actor_player, DEMO_SPRITE_PLAYER, 2, PLAYER_FRAME_RATE);
    nge_timer_add(actor_player, TIMER_PLAYER, 1, 0, 0);   /* its turn first */

    /* A demo effect on the player: an entity with a timer, owned by the player */
    glow = nge_entity_create();
    nge_util_assert(glow != NGE_ENTITY_INVALID);
    nge_owner_set(glow, actor_player);
    nge_timer_add(glow, TIMER_GLOW, GLOW_PERIOD, GLOW_PERIOD, GLOW_CYCLES);
}

uint8_t actors_move_player(int8_t dx, int8_t dy)
{
    uint8_t x = (uint8_t)(NGE_LOCATION_X(actor_player) + dx);
    uint8_t y = (uint8_t)(NGE_LOCATION_Y(actor_player) + dy);
    nge_entity_id_t id;
    nge_entity_id_t blocker;

    if (!level_can_enter(actor_player, x, y, &blocker)) {
        if (blocker != NGE_ENTITY_INVALID) {
            nge_event_post(blocker, EV_BLOCKED, actor_player, 0, 0);   /* later: an attack */
        } else {
            nge_event_post0(actor_player, EV_BUMPED);
        }
        return 0;
    }
    nge_location_set(actor_player, x, y);
    nge_map_centre_camera(x, y);

    /* Events: what happened, for the systems to react to */
    nge_event_post0(actor_player, EV_MOVED);
    NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, x, y, 0) {
        if (id != actor_player) {
            nge_event_post(id, EV_STOOD_ON, actor_player, 0, 0);
        }
    }
    return 1;
}

uint8_t actors_on_glow(nge_entity_id_t id, uint8_t cycles)
{
    nge_entity_id_t target = NGE_OWNER_OF(id);

    if (cycles == 1) {
        nge_event_post(target, EV_GLOW_FADED, id, 0, 0);
        nge_entity_destroy(id);   /* the timer goes with it */
        return NGE_TIMER_STOP;
    }
    nge_event_post(target, EV_GLOWED, id, 0, 0);
    return NGE_TIMER_REPEAT;
}
