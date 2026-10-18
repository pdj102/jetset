/*
 * Leaving a level, and entities going from one level to another: banked
 * in page 102 with the level generator. Runs once per change of level, or
 * when an entity leaves the level.
 *
 * Kept entities (level.h) survive a level: the persistent ones, and what
 * they own. Everything else a level's area owns, directly or through other
 * entities, is destroyed when the level is left. Ownership is stored only
 * as each entity's owner, so the alive list is scanned and each entity's
 * owners walked up: at most a few steps for each of NGE_ENTITY_MAX
 * entities, once per change.
 */
#ifdef __SDCC
#pragma codeseg PAGE_102
#pragma constseg PAGE_102
#endif

#include "game/level/level_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "game/components/feature.h"
#include "game/effects/effects.h"
#include "game/events.h"

/*
 * 1 if id is owned by root, directly or through other entities, with no
 * persistent entity on the way (id included, root excluded); id == root
 * counts, if root is an entity
 */
static uint8_t unkept_under(nge_entity_id_t id, nge_owner_t root)
{
    nge_owner_t o = id;

    while (NGE_OWNER_IS_ENTITY(o)) {
        if (o == root) {
            return 1;
        }
        if (nge_entity_flags[o] & NGE_ENTITY_PERSISTENT) {
            return 0;
        }
        o = NGE_OWNER_OF(o);
    }
    return o == root;
}

/*
 * Destroy every alive entity under root that isn't kept. A feature loses
 * its location first: its map is about to go, or it goes with the entity
 * that owns it, so there is no floor to put back
 */
static void destroy_unkept(nge_owner_t root)
{
    nge_entity_id_t id;
    uint8_t i;

    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        if (unkept_under(id, root)) {
            if (FEATURE_IS(id)) {
                nge_location_remove(id);
            }
            nge_entity_destroy(id);
        }
    }
}

void level_park(nge_entity_id_t id, nge_owner_t area) NGE_BANKED
{
    nge_entity_id_t owned;
    uint8_t i;

    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(NGE_OWNER_IS_AREA(area));

    nge_owner_set(id, area);
    if (NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION)) {
        nge_location_remove(id);
    }
    /* Effects that would run out end now: skipping their firings would still count their cycles */
    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(owned, COMPONENT_EFFECT) &&
            NGE_ENTITY_HAS_COMPONENTS(owned, NGE_COMPONENT_TIMER) && nge_timer_cycles[owned] != 0) {
            effect_end(owned);
        }
    }
}

void level_leave(nge_entity_id_t player) NGE_BANKED
{
    nge_owner_t area = LEVEL_AREA;
    nge_entity_id_t id;
    uint8_t i;

    nge_util_assert(player < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(player));

    nge_owner_set(player, NGE_OWNER_WORLD);   /* out of the way: placed again on the new level */
    destroy_unkept(area);
    level_release_dying();   /* kept entities held by those destroyed pass up into the area */
    NGE_OWNER_FOR_EACH_OWNED(i, id, area) {
        level_park(id, area);
    }
}

uint8_t level_send(nge_entity_id_t id, uint8_t depth) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));
    nge_util_assert(level_holds(id) && NGE_OWNER_IS_AREA(NGE_OWNER_OF(id)));

    if (depth < 1) {
        depth = 1;
    } else if (depth > LEVEL_DEPTH_BOTTOM) {
        depth = LEVEL_DEPTH_BOTTOM;
    }
    if (depth == level_depth) {
        return 0;
    }
    nge_event_post(id, EV_LEFT_LEVEL, NGE_EVENT_NONE, depth, 0);
    if (nge_entity_flags[id] & NGE_ENTITY_PERSISTENT) {
        level_park(id, NGE_OWNER_AREA(depth));
    } else {
        destroy_unkept(id);   /* it and what it owns; kept ones drop where it was */
    }
    return 1;
}
