/*
 * The fire action: banked in BANK_56 with the other actions (see act.h
 * for the pattern its steps follow).
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/attack.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/events.h"
#include "game/gen/gen.h"

/*
 * What actor shoots with: *attacker is whose ranged attack is made, *shot
 * the item used up, or NGE_ENTITY_INVALID if none is. Arrows with no
 * launcher are no use, so the actor's own attack is what is left then.
 * Returns ACT_R_NONE or why not
 */
static uint8_t find_shooter(nge_entity_id_t actor, nge_entity_id_t *attacker, nge_entity_id_t *shot)
{
    nge_entity_id_t ammo = NGE_ENTITY_INVALID;
    nge_entity_id_t launcher = NGE_ENTITY_INVALID;
    nge_entity_id_t who = actor;

    *shot = NGE_ENTITY_INVALID;
    if (INV_HAS(actor)) {
        ammo = inv_equipped(actor, SLOT_AMMO);
        launcher = inv_equipped(actor, SLOT_RANGED);
    }
    if (ammo != NGE_ENTITY_INVALID && gen_item_group(kind_index[ammo]) == GROUP_THROWN) {
        who = ammo;
        *shot = ammo;
    } else if (launcher != NGE_ENTITY_INVALID) {
        if (ammo == NGE_ENTITY_INVALID) {
            return ACT_R_NO_AMMO;
        }
        if (gen_item_group(kind_index[ammo]) != gen_item_group(kind_index[launcher])) {
            return ACT_R_WRONG_AMMO;
        }
        who = launcher;
        *shot = ammo;
    }
    *attacker = who;
    return ATTACK_HAS(who, ATTACK_RANGED) ? ACT_R_NONE : ACT_R_NO_ATTACK;
}

uint8_t act_fire(act_t *a, uint8_t mode) NGE_BANKED
{
    attack_kind_t ammo;
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t attacker;
    nge_entity_id_t shot;
    nge_entity_id_t item;
    uint8_t reason;

    /* 1. Context: the shooter, and its range */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));

    reason = find_shooter(actor, &attacker, &shot);
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    item = (attacker != actor) ? attacker : NGE_ENTITY_INVALID;
    a->item = item;
    a->amount = attack_range[ATTACK_RANGED][attacker];

    /* 2. Validate: a line to something that can be hit */
    reason = act_line(a, a->amount, ACT_LINE_BOLT);   /* a shot flies (D29) */
    if (reason != ACT_R_NONE) {
        return act_fail(a, reason);
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3, 4. Resolve and apply: the attacker's roll with the ammo's bonuses, and the ammo's hit effect */
    ammo.to_hit = 0;
    ammo.bonus = 0;
    if (shot != NGE_ENTITY_INVALID && shot != attacker) {
        gen_item_attack(kind_index[shot], &ammo);
    }
    nge_event_post(a->target, EV_FIRED, actor, item, 0);
    reason = act_strike(a, attacker, ATTACK_RANGED, (shot != NGE_ENTITY_INVALID) ? shot : attacker,
                        ammo.to_hit, ammo.bonus);

    /* 4. Apply: the shot is used up, hit or miss */
    if (shot != NGE_ENTITY_INVALID) {
        item_count[shot]--;
        if (item_count[shot] == 0) {
            nge_entity_destroy(shot);
        }
    }
    return reason;
}
