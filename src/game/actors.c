#include "game/actors.h"

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "event/nge_event.h"
#include "game/events.h"
#include "game/level/level.h"
#include "sprite/nge_sprite.h"

#define PLAYER_FRAME_RATE  25   /* frames per animation step: two steps a second */

nge_entity_id_t actor_player;
nge_entity_id_t actor_potion;

/* An entity in the level's area on tile (x, y), drawn with a still pattern */
static nge_entity_id_t create(uint8_t x, uint8_t y, uint8_t pattern, uint8_t layer)
{
    nge_entity_id_t id = nge_entity_create();

    nge_util_assert(id != NGE_ENTITY_INVALID);
    nge_util_assert(level_is_open(x, y));

    nge_owner_set(id, LEVEL_AREA);
    nge_location_set(id, x, y);
    nge_render_add(id, pattern, 0, NGE_SPRITE_SCALE_1, layer);
    return id;
}

void actors_init(void)
{
    /* The loaders map the graphics' page into slot 7 while they copy */
    nge_sprite_load_patterns(DEMO_SPRITE_PAGE, 0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
    nge_sprite_load_palette(DEMO_SPRITE_PAGE, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);

    /* The player, in room A, two frames to show the frame loop is running */
    actor_player = create(6, 5, DEMO_SPRITE_PLAYER, LAYER_PLAYER);
    nge_render_set_anim(actor_player, DEMO_SPRITE_PLAYER, 2, PLAYER_FRAME_RATE);

    /* A test object in room C */
    actor_potion = create(14, 24, DEMO_SPRITE_POTION, LAYER_OBJECTS);
}

uint8_t actors_move_player(int8_t dx, int8_t dy)
{
    uint8_t x = (uint8_t)(NGE_LOCATION_X(actor_player) + dx);
    uint8_t y = (uint8_t)(NGE_LOCATION_Y(actor_player) + dy);
    nge_entity_id_t id;

    if (!level_is_open(x, y)) {
        nge_event_post0(actor_player, EV_BUMPED);
        return 0;
    }
    nge_location_set(actor_player, x, y);
    nge_map_centre_camera(&level_map, x, y);

    /* Events: what happened, for the systems to react to */
    nge_event_post0(actor_player, EV_MOVED);
    NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, x, y, 0) {
        if (id != actor_player) {
            nge_event_post(id, EV_STOOD_ON, actor_player, 0, 0);
        }
    }
    return 1;
}
