/*
 * The quest item's functions: banked in page 128 with their literals (a
 * banked game module, see "Banked game modules" in docs/memory_map.md), as
 * they run once per event, refused climb or change of level.
 */
#ifdef __SDCC
#pragma codeseg PAGE_128
#pragma constseg PAGE_128
#endif

#include "game/quest/quest.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "game/components/pc.h"
#include "game/events.h"
#include "game/level/level.h"

static uint8_t found;   /* 1 once the player has picked it up this game */

void quest_set(nge_entity_id_t id) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);

    quest_item = id;
    found = 0;
}

uint8_t quest_carried(nge_entity_id_t actor) NGE_BANKED
{
    nge_owner_t o;

    if (quest_item == NGE_ENTITY_INVALID) {
        return 0;
    }
    o = NGE_OWNER_OF(quest_item);
    while (NGE_OWNER_IS_ENTITY(o)) {
        if (o == actor) {
            return 1;
        }
        o = NGE_OWNER_OF(o);   /* ownership has no cycles: this ends */
    }
    return 0;
}

void quest_on_event(const nge_event_t *e) NGE_BANKED
{
    /* The fields in locals: see messages_on_event() */
    uint8_t id = e->id;
    nge_entity_id_t target = e->target;
    nge_entity_id_t source = e->source;

    if (id == EV_PICKED_UP && target == quest_item && !found && source < NGE_ENTITY_MAX && PC_IS(source)) {
        found = 1;
        nge_event_post(quest_item, EV_QUEST_FOUND, source, 0, 0);
    }
}

#if GAME_DEV_KIT
void quest_check(void) NGE_BANKED
{
    nge_entity_id_t id = quest_item;
    nge_owner_t o;
    uint8_t depth;

    nge_util_assert(id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id));
    nge_util_assert(nge_entity_flags[id] & NGE_ENTITY_PERSISTENT);
    o = NGE_OWNER_OF(id);
    if (NGE_OWNER_IS_AREA(o)) {
        /* On the floor of the current level, or parked off it */
        nge_util_assert((NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) != 0) == (o == LEVEL_AREA));
    }
    while (NGE_OWNER_IS_ENTITY(o)) {
        o = NGE_OWNER_OF(o);
    }
    nge_util_assert(NGE_OWNER_IS_AREA(o));
    depth = NGE_OWNER_AREA_OF(o);
    nge_util_assert(depth >= 1 && depth <= LEVEL_DEPTH_BOTTOM);
    (void)depth;
}
#endif
