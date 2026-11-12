/*
 * The aim action: banked in BANK_56 with the other actions (see act.h for
 * the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/spells/spells.h"
#include "game/events.h"
#include "game/gen/gen.h"

uint8_t act_aim(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    act_shape_t s;
    uint16_t hook;
    uint8_t ef;
    uint8_t wand;
    uint8_t shape;
    uint8_t reason;

    /* 1. Context: the item's shape, from its kind */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(item < NGE_ENTITY_MAX);

    /* 2. Validate: the actor has the item, stowed, with an aim hook and a charge, and a target */
    if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item) || NGE_OWNER_OF(item) != actor) {
        return act_fail(a, ACT_R_NOT_CARRIED);
    }
    if (item_slot[item] != SLOT_NONE) {
        return act_fail(a, ACT_R_EQUIPPED);
    }
    hook = gen_hook_of(item, GEN_HOOK_AIM);
    ef = GEN_HOOK_EF(hook);
    if (ef == EF_NONE) {
        return act_fail(a, ACT_R_CANT_AIM);
    }
    wand = gen_item_subclass(kind_index[item]) == ITEM_WAND;
    if (wand && item_count[item] == 0) {
        return act_fail(a, ACT_R_NO_CHARGES);
    }
    a->effect = ef;
    shape = gen_item_shape(kind_index[item]);
    s.shape = shape;
    if ((shape == SPELL_BOLT || shape == SPELL_DIRECT) && a->x == NGE_LOCATION_X(actor) &&
        a->y == NGE_LOCATION_Y(actor)) {
        a->target = actor;   /* only aim does this: a staff of healing can heal its holder */
    } else {
        reason = act_shape_find(a, &s, ACT_AIM_RANGE);
        if (reason != ACT_R_NONE) {
            return act_fail(a, reason);
        }
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll: the effect's own saving throw decides what it does */

    /* 4. Apply: a charge used, or a parchment used up, then the effect, with the actor as its source */
    nge_event_post(SPELL_IS_AREA(shape) ? NGE_EVENT_NONE : a->target, EV_AIMED, actor, item,
                   0);   /* an area's creatures are its own events' */
    if (wand) {
        item_count[item]--;
    } else {
        nge_entity_destroy(item);
    }
    act_shape_apply(a, &s, ef, GEN_HOOK_POWER(hook));
    return act_succeed(a);
}
