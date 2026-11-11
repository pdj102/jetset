/*
 * The cast action: banked in BANK_56 with the other actions (see act.h for
 * the pattern its steps follow). See docs/action_cast.md.
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "event/nge_event.h"
#include "game/components/mana.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/level/level.h"
#include "game/spells/spells.h"

uint8_t act_cast(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    uint8_t sp = a->spell;
    spell_def_t def;   /* a copy: the table is in the spells' page */
    uint8_t shape;
    uint8_t ef;
    int8_t power;
    uint8_t range;
    uint8_t cost;
    uint8_t reason;
    uint8_t tx = a->x;   /* the tile chosen, in locals (T-ZSDCC-1) */
    uint8_t ty = a->y;

    /* 1. Context: the row's fields in locals (T-ZSDCC-1) */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(NGE_ENTITY_HAS_COMPONENT(actor, NGE_COMPONENT_LOCATION));
    nge_util_assert(sp < SPELL_COUNT);
    spell_get(sp, &def);
    shape = def.shape;
    ef = def.ef;
    power = def.power;
    range = def.range;
    cost = spell_cost(def.level);

    /* 2. Validate: known, paid for (an ability needs no mana), and a target for its shape */
    if (!spell_knows(actor, sp)) {
        return act_fail(a, ACT_R_NO_SPELL);
    }
    if (cost != 0 && (!MANA_IS(actor) || !MANA_HAS(actor, cost))) {
        return act_fail(a, ACT_R_NO_MANA);
    }
    if (shape == SPELL_SELF) {
        a->target = actor;
    } else if (shape == SPELL_TILE) {
        /* A free tile in range and line: a creature there is in the way; the effect is the caster's */
        reason = act_line(a, range, ACT_LINE_DIRECT);
        if (reason == ACT_R_NONE) {
            return act_fail(a, ACT_R_BLOCKED_ENTITY);   /* a->target: the creature */
        }
        if (reason != ACT_R_NO_TARGET || (tx == NGE_LOCATION_X(actor) && ty == NGE_LOCATION_Y(actor))) {
            return act_fail(a, reason);   /* out of range, no line, or its own tile */
        }
        if (!level_can_enter(actor, tx, ty, &a->target)) {
            return act_fail(a, (a->target != NGE_ENTITY_INVALID) ? ACT_R_BLOCKED_ENTITY : ACT_R_BLOCKED);
        }
        a->target = actor;
    } else {
        /* The actor's own tile is ACT_R_NO_TARGET: a check there asks whether it can cast at all */
        reason = act_line(a, range, shape == SPELL_DIRECT ? ACT_LINE_DIRECT : ACT_LINE_BOLT);
        if (reason != ACT_R_NONE) {
            return act_fail(a, reason);
        }
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll: a valid cast always happens, and a save is the effect's */

    /* 4. Apply: the cast, the MP, then the effect with the actor as its source (a kill earns its XP) */
    nge_event_post(a->target, EV_CAST, actor, sp, 0);
    if (cost != 0) {
        mana_spend(actor, cost);
    }
    if (shape == SPELL_TILE) {
        effect_apply_at(ef, power, actor, actor, tx, ty);
    } else {
        effect_apply(ef, power, a->target, actor);
    }
    return act_succeed(a);
}
