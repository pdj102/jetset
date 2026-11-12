/*
 * The learn action: banked in BANK_56 with the other actions (see act.h for
 * the pattern its steps follow). See docs/action_learn.md.
 */
#ifdef __SDCC
#pragma codeseg BANK_56
#pragma constseg BANK_56
#endif

#include "game/actions/act_internal.h"

#include "core/nge_util.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/gen/gen.h"
#include "game/spells/spells.h"

uint8_t act_learn(act_t *a, uint8_t mode) NGE_BANKED
{
    nge_entity_id_t actor = a->actor;
    nge_entity_id_t item = a->item;
    uint8_t sp = a->spell;
    uint8_t kind;
    uint8_t level;
    spell_def_t def;   /* a copy: the table is in the spells' page */

    /* 1. Context */
    nge_util_assert(actor < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(actor));
    nge_util_assert(item < NGE_ENTITY_MAX);
    nge_util_assert(sp < SPELL_COUNT);

    /* 2. Validate: a book the actor carries, a player who can learn, then the spell (docs/action_learn.md) */
    if (!NGE_ENTITY_IS_ALIVE(item) || !ITEM_IS(item) || NGE_OWNER_OF(item) != actor ||
        item_slot[item] != SLOT_NONE) {
        return act_fail(a, ACT_R_NOT_CARRIED);   /* from the pack only, not the floor (R2) */
    }
    kind = kind_index[item];
    if (gen_item_subclass(kind) != ITEM_BOOK) {
        return act_fail(a, ACT_R_NOT_BOOK);
    }
    if (!PC_IS(actor) || !MANA_IS(actor) || !SPELLS_IS(actor)) {
        return act_fail(a, ACT_R_CANT_LEARN);
    }
    if (sp == SP_NONE) {
        nge_util_assert(mode == ACT_CHECK);   /* only the book asked about (D2) */
        return act_checked(a);
    }
    if (!spell_in_book(gen_item_book_spells(kind), sp)) {
        return act_fail(a, ACT_R_NOT_IN_BOOK);
    }
    if (spell_knows(actor, sp)) {
        return act_fail(a, ACT_R_KNOWN);
    }
    spell_get(sp, &def);
    level = def.level;
    if (spell_slots(actor, level) == 0) {
        return act_fail(a, ACT_R_BEYOND);       /* no slot at that level yet (D3) */
    }
    if (spell_free(actor, level) == 0) {
        return act_fail(a, ACT_R_SLOTS_FULL);   /* slots, all filled */
    }
    if (mode == ACT_CHECK) {
        return act_checked(a);
    }

    /* 3. Resolve: nothing to roll */

    /* 4. Apply: the one way to add a spell, which posts EV_SPELL_LEARNED (docs/spells.md R8). The book is kept (R4) */
    spell_learn(actor, sp);
    return act_succeed(a);
}
