/*
 * The features' reactions to events (docs/features.md 4.7): banked in page
 * 132 with the feature component, in a file of its own as it calls the
 * actions (act_spring()), which the component's other users don't link.
 */
#ifdef __SDCC
#pragma codeseg PAGE_132
#pragma constseg PAGE_132
#endif

#include "game/components/feature.h"

#include "ecs/components/nge_block.h"
#include "game/actions/act.h"
#include "game/components/inventory.h"
#include "game/components/kind.h"
#include "game/components/stats.h"
#include "game/events.h"
#include "game/gen/gen.h"

void feature_on_event(const nge_event_t *e) NGE_BANKED
{
    gen_feature_info_t info;
    nge_entity_id_t target = e->target;   /* locals: see messages_on_event() */
    nge_entity_id_t source = e->source;
    uint8_t id = e->id;

    if (!NGE_ENTITY_IS_ALIVE(target) || !FEATURE_IS(target)) {
        return;
    }
    gen_feature_info(kind_index[target], &info);
    if (id == EV_OPENED) {
        if (info.can & FCAN_CONTAINER) {
            inv_drop_all(target);   /* its tile is open now: the items are picked up from it */
        }
    } else if (id == EV_STOOD_ON && (info.can & FCAN_TRAP) && source < NGE_ENTITY_MAX &&
               NGE_ENTITY_IS_ALIVE(source)) {
        /*
         * A trap fires at a walker: one that flies passes over it (R15), by
         * kind (it passes holes) or for a while (STATUS_FLYING: levitation)
         */
        if ((!NGE_ENTITY_HAS_COMPONENT(source, NGE_COMPONENT_BLOCK) ||
             !(NGE_BLOCK_PASS(source) & NGE_BLOCK_NO_FLOOR)) &&
            !(NGE_ENTITY_HAS_COMPONENT(source, COMPONENT_STATS) && STATUS_HAS(source, STATUS_FLYING))) {
            act_spring(target, source);
        }
    }
}
