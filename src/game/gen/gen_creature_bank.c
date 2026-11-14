/*
 * The creature generator: banked in page 106, with its table of creature
 * kinds in page 107 (gen_creature_pdata.c), mapped into slot 7 while
 * this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_106
#pragma constseg PAGE_106
#endif

#include "game/gen/gen_internal.h"

#include <string.h>

#include <stdlib.h>

#include "core/nge_util.h"
#include "game/actions/act.h"
#include "ecs/systems/nge_timer.h"
#include "game/timers.h"

/*
 * Make an item of kind in id's inventory and equip it, as chargen equips the
 * player's kit. A refused equip leaves it carried. With no entity free, id
 * goes without: the level's budget allows for a kit (R12)
 */
static void equip(nge_entity_id_t id, uint8_t kind)
{
    act_t a = { 0 };
    uint8_t ok;

    a.item = gen_item(kind, id);
    if (a.item == NGE_ENTITY_INVALID) {
        return;
    }
    a.actor = id;
    a.slot = SLOT_NONE;   /* the first free slot it fits */
    ok = act_equip(&a, ACT_DO);
    nge_util_assert(ok == ACT_SUCCESS);
    (void)ok;
}

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    gen_creature_kind_t b;   /* a copy on the stack: the calls below map other pages */
    nge_entity_id_t id;
    uint8_t item;
    uint8_t i;

    memcpy(&b, &gen_creatures[kind], sizeof(b));
    id = gen_begin(CLASS_CREATURE, kind, owner);
    if (id == NGE_ENTITY_INVALID) {
        return id;
    }
    gen_render(id, &b.render);
    nge_block_add(id, b.block.block, b.block.pass);
    stats_add(id, &b.stats);
    health_add(id, &b.health);
    if (MANA_KIND_HAS(&b.mana)) {
        mana_add(id, &b.mana);
    }
    if (b.spells[0] != SP_NONE) {
        spell_add(id, b.spells, GEN_SPELLS_MAX);
    }
    if (ATTACK_KIND_HAS(&b.melee)) {
        attack_add(id, ATTACK_MELEE, &b.melee);
    }
    if (ATTACK_KIND_HAS(&b.ranged)) {
        attack_add(id, ATTACK_RANGED, &b.ranged);
    }
    if (b.flags & CREATURE_PLAYER) {
        nge_entity_set_flags(id, NGE_ENTITY_PERSISTENT);   /* at 0 hp the game ends instead (EV_DIED) */
        pc_add(id);                                        /* a new character: level 1 */
    }
    nge_util_assert(((b.flags & CREATURE_PLAYER) != 0) != AI_KIND_HAS(&b.ai));   /* the player, or a brain */
    if (AI_KIND_HAS(&b.ai)) {
        ai_add(id, &b.ai);                                 /* a monster: the game decides its turns */
    }
    if (b.inventory.capacity != 0) {
        inv_add(id, b.inventory.capacity, b.inventory.slots);
    }
    /* Its turns, every speed ticks: the player's wait for a command (player_turn), a monster's are the AI's */
    nge_timer_add(id, (b.flags & CREATURE_PLAYER) ? TIMER_PLAYER : TIMER_AI, stats_speed[id], 0);
    /* Its kit, equipped: once it has every component, as equipping recomputes its attributes */
    for (i = 0; i < GEN_KIT_MAX; i++) {
        item = b.kit[i];   /* in a local before branching on it (T-ZSDCC-1) */
        if (item != IT_NONE) {
            equip(id, item);
        }
    }
    if (b.innate.ef != EF_NONE) {
        effect_apply(b.innate.ef, b.innate.power, id, NGE_ENTITY_INVALID);   /* an entity of its own, owned by it */
    }
    if (MANA_KIND_HAS(&b.mana)) {
        effect_apply(EF_MANA_REGEN, 1, id, NGE_ENTITY_INVALID);   /* a rule, not a hook: every kind with mana (docs/spells.md, D12) */
    }
    return id;
}

nge_entity_id_t gen_creature(uint8_t kind, nge_owner_t owner) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    return create(kind, owner);
}

void gen_creature_seqs(uint8_t kind, gen_seq_t *seq) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    memcpy(seq, gen_creatures[kind].seqs, sizeof(gen_creatures[kind].seqs));
}

nge_entity_id_t gen_creature_random(uint8_t depth, nge_owner_t owner) NGE_BANKED
{
    uint8_t kind;

    GEN_PICK(gen_creatures, CR_COUNT, depth, kind);
    return (kind == KIND_INVALID) ? NGE_ENTITY_INVALID : create(kind, owner);
}

nge_entity_id_t gen_creature_random_flags(uint8_t depth, nge_owner_t owner, uint8_t flags) NGE_BANKED
{
    uint16_t total = 0;
    uint16_t roll = 0;
    uint8_t rarity;
    uint8_t kind;
    uint8_t pass;

    /* Two passes over the same test: the weights, then the pick. Fields in locals (T-ZSDCC-1) */
    for (pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            if (total == 0) {
                return NGE_ENTITY_INVALID;
            }
            roll = (uint16_t)((unsigned int)rand() % total);
        }
        for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
            rarity = gen_creatures[kind].where.rarity;
            if ((gen_creatures[kind].flags & flags) == flags && gen_creatures[kind].where.depth <= depth &&
                gen_creatures[kind].spells[0] == SP_NONE) {
                if (pass == 0) {
                    total += rarity;
                } else if (roll < rarity) {
                    return create(kind, owner);
                } else {
                    roll -= rarity;
                }
            }
        }
    }
    return NGE_ENTITY_INVALID;   /* not reached: the pick is within the total */
}

uint8_t gen_creature_flags(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    return gen_creatures[kind].flags;
}

uint16_t gen_creature_hit(uint8_t kind, uint8_t use) NGE_BANKED
{
    gen_hook_t h;

    nge_util_assert(kind < CR_COUNT);

    h = (use == ATTACK_MELEE) ? gen_creatures[kind].melee_hit : gen_creatures[kind].ranged_hit;
    return GEN_HOOK_PACK(h.ef, h.power);
}

uint16_t gen_creature_xp(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    return gen_creatures[kind].xp;
}

void gen_creature_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    GEN_COPY_NAME(gen_creatures[kind].name, buf, size);
}
