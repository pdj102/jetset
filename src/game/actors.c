#include "game/actors.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "event/nge_event.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/timers.h"
#include "game/level/level.h"
#include "sprite/nge_sprite.h"

/* The demo effect: every 5 turns, 4 times */
#define GLOW_PERIOD  (5 * NGE_TIMER_TICKS_PER_TURN)

#ifndef GAME_DEV_KIT
/*
 * Development starting kit: 1 gives the player items to try out (a sword
 * and three rings) in their pack. On in debug builds (CSpect), off with
 * NDEBUG (hardware); -DGAME_DEV_KIT=0 or 1 overrides it
 */
#ifdef NDEBUG
#define GAME_DEV_KIT  0
#else
#define GAME_DEV_KIT  1
#endif
#endif

#if GAME_DEV_KIT
/* The development kit's items, owned by the player: in their pack */
static const uint8_t dev_kit[] = { IT_SWORD, IT_RING, IT_RING, IT_RING };
#endif
#define GLOW_CYCLES  4

nge_entity_id_t actor_player;

void actors_load_graphics(void)
{
    /* The loaders map the graphics' page into slot 7 while they copy */
    nge_sprite_load_patterns(DEMO_SPRITE_PAGE, 0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
    nge_sprite_load_palette(DEMO_SPRITE_PAGE, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);
}

void actors_create(void)
{
    nge_entity_id_t glow;

    /*
     * The player, from its base data: placed by level_generate(). Created
     * first, so entity 0: on a tick where others are due too, its turn
     * comes first (timers fire in ID order)
     */
    actor_player = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    nge_util_assert(actor_player != NGE_ENTITY_INVALID);

#if GAME_DEV_KIT
    {
        uint8_t i;

        for (i = 0; i < sizeof(dev_kit); i++) {
            gen_item(dev_kit[i], actor_player);   /* owned, not placed: stowed */
        }
    }
#endif

    /* A demo effect on the player: an entity with a timer, owned by the player */
    glow = nge_entity_create();
    nge_util_assert(glow != NGE_ENTITY_INVALID);
    nge_owner_set(glow, actor_player);
    nge_timer_add(glow, TIMER_GLOW, GLOW_PERIOD, GLOW_CYCLES);
}

void actors_on_glow(nge_entity_id_t id, uint8_t cycles)
{
    nge_entity_id_t target = NGE_OWNER_OF(id);

    if (cycles == 1) {
        nge_event_post(target, EV_GLOW_FADED, id, 0, 0);
        nge_entity_destroy(id);   /* its last firing: the effect goes */
        return;
    }
    nge_event_post(target, EV_GLOWED, id, 0, 0);
}
