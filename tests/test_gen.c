/* game/gen: the entity generators, every kind against its kind record. */
#include "test.h"

#include <stdlib.h>
#include <string.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"

#include "game/effects/effects_game.h"
#include "game/gen/gen_internal.h"
#include "game/timers.h"

#define AREA  NGE_OWNER_AREA(0)

/* The sprite component matches a record's sprite part */
static int render_is(nge_entity_id_t id, const gen_render_t *r)
{
    return NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_SPRITE) &&
           nge_render_sprite_first[id] == r->pattern &&
           nge_render_sprite_anim_count[id] == r->anim_count &&
           (r->anim_count == 1 || nge_render_sprite_anim_rate[id] == r->anim_rate) &&
           nge_render_sprite_flags[id] == r->flags &&
           nge_render_sprite_layer[id] == r->layer;
}

/* An entity just generated: alive, owned, of this class and kind, not placed */
static int made(nge_entity_id_t id, nge_owner_t owner, uint8_t c, uint8_t kind)
{
    return id != NGE_ENTITY_INVALID && NGE_ENTITY_IS_ALIVE(id) && NGE_OWNER_OF(id) == owner &&
           KIND_IS(id, c) && kind_index[id] == kind &&
           !NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION);
}

/* The melee attack component matches a record's melee part (or there is none) */
static int melee_is(nge_entity_id_t id, const attack_kind_t *m)
{
    if (!ATTACK_KIND_HAS(m)) {
        return !ATTACK_HAS(id, ATTACK_MELEE);
    }
    return ATTACK_HAS(id, ATTACK_MELEE) && attack_to_hit[ATTACK_MELEE][id] == m->to_hit &&
           attack_dice[ATTACK_MELEE][id] == m->dice && attack_sides[ATTACK_MELEE][id] == m->sides &&
           attack_bonus[ATTACK_MELEE][id] == m->bonus && attack_type[ATTACK_MELEE][id] == m->type &&
           attack_range[ATTACK_MELEE][id] == 1;
}

static int creature_ok(nge_entity_id_t id, nge_owner_t owner, uint8_t kind)
{
    const gen_creature_kind_t *b = &gen_creatures[kind];
    uint8_t a;

    if (!made(id, owner, CLASS_CREATURE, kind) || !render_is(id, &b->render)) {
        return 0;
    }
    if (!NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_BLOCK | COMPONENT_STATS | NGE_COMPONENT_TIMER) ||
        nge_block_of[id] != b->block.block || nge_block_pass[id] != b->block.pass) {
        return 0;
    }
    if (!NGE_ENTITY_HAS_COMPONENTS(id, COMPONENT_HEALTH) ||
        health_hp[id] != b->health.hp_max || health_hp_max[id] != b->health.hp_max ||
        health_immune[id] != b->health.immune || health_resist[id] != b->health.resist ||
        health_vuln[id] != b->health.vuln || health_ac[id] != b->health.ac ||
        stats_speed[id] != b->stats.speed || !melee_is(id, &b->melee)) {
        return 0;
    }
    /* Mana if its kind has some, full */
    if (MANA_IS(id) != MANA_KIND_HAS(&b->mana) ||
        (MANA_IS(id) && (mana_mp[id] != b->mana.mp_max || mana_mp_max[id] != b->mana.mp_max))) {
        return 0;
    }
    /* Its abilities: the record's scores, as values and bases */
    for (a = 0; a < ABIL_COUNT; a++) {
        if (stats_ability[a][id] != b->stats.ability[a] || attr_base[ATTR_ABILITY(a)][id] != b->stats.ability[a] ||
            b->stats.ability[a] < STATS_ABILITY_MIN || b->stats.ability[a] > STATS_ABILITY_MAX) {
            return 0;
        }
    }
    /* Only the player is persistent: at 0 hp the game ends instead */
    if (NGE_ENTITY_HAS_FLAGS(id, NGE_ENTITY_PERSISTENT) != ((b->flags & CREATURE_PLAYER) != 0)) {
        return 0;
    }
    /* An inventory if it can carry things */
    if (b->inventory.capacity == 0 ? INV_HAS(id)
                                   : (!INV_HAS(id) || inv_capacity[id] != b->inventory.capacity ||
                                      inv_slots[id] != b->inventory.slots)) {
        return 0;
    }
    /* Its turns: every speed ticks, with no limit */
    return nge_timer_kind[id] == ((b->flags & CREATURE_PLAYER) ? TIMER_PLAYER : TIMER_MONSTER) &&
           nge_timer_period[id] == b->stats.speed && nge_timer_count[id] == b->stats.speed &&
           nge_timer_cycles[id] == 0;
}

static int item_ok(nge_entity_id_t id, nge_owner_t owner, uint8_t kind)
{
    return made(id, owner, CLASS_ITEM, kind) && render_is(id, &gen_items[kind].render) &&
           ITEM_IS(id) && item_slot[id] == SLOT_NONE && melee_is(id, &gen_items[kind].melee) &&
           !NGE_ENTITY_HAS_ANY_COMPONENTS(id, NGE_COMPONENT_BLOCK | COMPONENT_STATS | NGE_COMPONENT_TIMER |
                                              COMPONENT_INVENTORY | COMPONENT_HEALTH);
}

static void reset(void)
{
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
}

int main(void)
{
    static const gen_where_t where[3] = { { 1, 1 }, { 5, 100 }, { 0, 0 } };
    static const gen_where_t deep_first[2] = { { 5, 100 }, { 1, 1 } };
    nge_entity_id_t id;
    nge_entity_id_t chest;
    char buf[20];
    int seen[CR_COUNT > IT_COUNT ? CR_COUNT : IT_COUNT];
    int i;
    int ok;
    int lo;
    uint8_t kind;

    test_init();
    srand(3);

    /* ---- every component, nge's and the game's, has a bit of its own (game/components/components.h) ---- */
    {
        static const nge_component_mask_t masks[] = {
            NGE_COMPONENT_LOCATION, NGE_COMPONENT_SPRITE, NGE_COMPONENT_BLOCK, NGE_COMPONENT_TIMER,
            COMPONENT_KIND, COMPONENT_STATS, COMPONENT_ITEM, COMPONENT_INVENTORY, COMPONENT_HEALTH,
            COMPONENT_MELEE, COMPONENT_RANGED, COMPONENT_EFFECT, COMPONENT_FEATURE, COMPONENT_MANA,
        };
        nge_component_mask_t seen = 0;
        unsigned int m;

        ok = sizeof(masks) / sizeof(masks[0]) == NGE_COMPONENT_USER + COMPONENT_COUNT;
        for (m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
            ok = ok && masks[m] != 0 && (masks[m] & (masks[m] - 1)) == 0 && (seen & masks[m]) == 0;
            seen |= masks[m];
        }
        CHECK(ok);
    }

    /* ---- every kind: its components match its kind record ---- */
    reset();
    ok = 1;
    for (kind = 0; kind < CR_COUNT; kind++) {
        id = gen_creature(kind, AREA);
        ok = ok && creature_ok(id, AREA, kind);
    }
    CHECK(ok);
    ok = 1;
    for (kind = 0; kind < IT_COUNT; kind++) {
        id = gen_item(kind, AREA);
        ok = ok && item_ok(id, AREA, kind) && gen_item_fits(kind) == gen_items[kind].fits;
    }
    CHECK(ok);
    CHECK(gen_item_fits(IT_POTION) == 0 && gen_item_fits(IT_SWORD) == SLOT_BIT(SLOT_WEAPON) &&
          gen_item_fits(IT_RING) == (SLOT_BIT(SLOT_RING_L) | SLOT_BIT(SLOT_RING_R)));

    /* ---- hooks and subclasses: what the records say, by kind and by entity ---- */
    reset();
    ok = 1;
    for (kind = 0; kind < IT_COUNT; kind++) {
        id = gen_item(kind, AREA);
        ok = ok && gen_item_subclass(kind) == gen_items[kind].subclass;
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && gen_item_hook(kind, (uint8_t)i) == gen_items[kind].hooks[i] &&
                 gen_hook(id, (uint8_t)i) == gen_items[kind].hooks[i] && gen_items[kind].hooks[i] < EF_COUNT;
        }
    }
    for (kind = 0; kind < CR_COUNT; kind++) {
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && gen_creature_hook(kind, (uint8_t)i) == gen_creatures[kind].hooks[i] &&
                 gen_creatures[kind].hooks[i] < EF_COUNT;
        }
    }
    CHECK(ok);
    /* Features: names, hooks and info from their records; a door is a closed, openable, solid feature */
    for (kind = 0; kind < FT_COUNT; kind++) {
        gen_feature_info_t info;

        id = gen_feature(kind, AREA);
        gen_feature_info(kind, &info);
        gen_name(id, buf, sizeof(buf));
        ok = ok && strcmp(buf, gen_features[kind].name) == 0 && feature_state[id] == gen_features[kind].state &&
             memcmp(&info, &gen_features[kind].info, sizeof(info)) == 0;
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && gen_feature_hook(kind, (uint8_t)i) == gen_features[kind].hooks[i] &&
                 gen_hook(id, (uint8_t)i) == gen_features[kind].hooks[i];
        }
    }
    CHECK(ok);
    CHECK(gen_features[FT_DOOR].info.can == FCAN_OPEN && gen_features[FT_DOOR].state == 0 &&
          gen_features[FT_DOOR].where.rarity == 0);
    TEST_EXPECT_HALT(gen_feature_hook(FT_COUNT, GEN_HOOK_USE));
    CHECK(test_halt_count == 1);

    /* A worn hook names a worn effect; the other hooks name effects that can be applied */
    ok = 1;
    for (kind = 0; kind < IT_COUNT; kind++) {
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            uint8_t ef = gen_items[kind].hooks[i];

            ok = ok && (ef == EF_NONE || (effect_defs[ef].duration == EFFECT_WORN) == (i == GEN_HOOK_WORN));
        }
    }
    for (kind = 0; kind < CR_COUNT; kind++) {
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            uint8_t ef = gen_creatures[kind].hooks[i];

            ok = ok && (ef == EF_NONE || (i != GEN_HOOK_WORN && i != GEN_HOOK_USE &&
                                          effect_defs[ef].duration != EFFECT_WORN));
        }
    }
    CHECK(ok);
    CHECK(gen_item_hook(IT_POTION, GEN_HOOK_USE) == EF_HEALING && gen_item_hook(IT_RING, GEN_HOOK_WORN) == EF_RING_HEALTH &&
          gen_item_hook(IT_ANTIDOTE, GEN_HOOK_USE) == EF_CURE && gen_item_hook(IT_FOOD, GEN_HOOK_USE) == EF_POISON &&
          gen_item_hook(IT_ELIXIR, GEN_HOOK_USE) == EF_VITALITY && gen_item_hook(IT_STRENGTH, GEN_HOOK_USE) == EF_STRENGTH && gen_item_hook(IT_SWORD, GEN_HOOK_HIT) == EF_NONE);
    CHECK(gen_item_subclass(IT_FOOD) == ITEM_FOOD && gen_item_subclass(IT_ELIXIR) == ITEM_POTION);
    id = gen_creature(CR_RAT, AREA);
    CHECK(gen_hook(id, GEN_HOOK_HIT) == EF_POISON && gen_hook(id, GEN_HOOK_INNATE) == EF_NONE);
    CHECK(!MANA_IS(id));                                       /* monsters have no mana yet */
    CHECK(nge_owner_count(id) == 0);                           /* no innate effect */
    id = gen_creature(CR_PLAYER, AREA);
    CHECK(gen_hook(id, GEN_HOOK_INNATE) == EF_REGEN && nge_owner_count(id) == 1);   /* its innate effect */
    CHECK(MANA_IS(id) && mana_mp[id] == 5 && mana_mp_max[id] == 5);                 /* full */
    chest = nge_entity_create();                               /* no kind: no hooks */
    CHECK(gen_hook(chest, GEN_HOOK_USE) == EF_NONE);
    TEST_EXPECT_HALT(gen_hook(id, GEN_HOOK_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_item_hook(IT_COUNT, GEN_HOOK_USE));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_creature_hook(CR_COUNT, GEN_HOOK_HIT));
    CHECK(test_halt_count == 1);

    /* Any owner: an item in a chest, a creature not yet placed */
    chest = gen_item(IT_SWORD, AREA);
    id = gen_item(IT_POTION, chest);
    CHECK(item_ok(id, chest, IT_POTION));
    id = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    CHECK(creature_ok(id, NGE_OWNER_WORLD, CR_PLAYER));

    /* Hit points are fixed by kind: every rat the same */
    reset();
    ok = 1;
    for (i = 0; i < 20 && ok; i++) {
        id = gen_creature(CR_RAT, AREA);
        ok = health_hp[id] == gen_creatures[CR_RAT].health.hp_max && health_hp[id] == 3;
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    CHECK(ok);

    /* ---- names: copied into the caller's buffer, cut to fit ---- */
    reset();
    id = gen_creature(CR_GOBLIN, AREA);
    gen_name(id, buf, sizeof(buf));
    CHECK(strcmp(buf, gen_creatures[CR_GOBLIN].name) == 0 && strcmp(buf, "goblin") == 0);
    id = gen_item(IT_SWORD, AREA);
    gen_name(id, buf, sizeof(buf));
    CHECK(strcmp(buf, "sword") == 0);
    memset(buf, 'x', sizeof(buf));
    gen_name(id, buf, 3);
    CHECK(strcmp(buf, "sw") == 0 && buf[3] == 'x');
    memset(buf, 'x', sizeof(buf));
    gen_name(id, buf, 1);
    CHECK(buf[0] == '\0' && buf[1] == 'x');
    id = nge_entity_create();                     /* no kind */
    gen_name(id, buf, sizeof(buf));
    CHECK(strcmp(buf, "something") == 0);

    /* ---- random kinds: weighted by rarity, only from their depth ---- */
    reset();
    memset(seen, 0, sizeof(seen));
    ok = 1;
    for (i = 0; i < 300 && ok; i++) {
        id = gen_creature_random(1, AREA);
        ok = id != NGE_ENTITY_INVALID && creature_ok(id, AREA, kind_index[id]);
        if (ok) {
            seen[kind_index[id]]++;
            nge_entity_destroy(id);
            nge_entity_release(nge_entity_next_dying());
        }
    }
    CHECK(ok);
    CHECK(seen[CR_PLAYER] == 0 && seen[CR_RAT] > 0 && seen[CR_GOBLIN] > 0);   /* rarity 0: never */
    CHECK(seen[CR_RAT] > seen[CR_GOBLIN]);                                      /* rarity 10 against 6 */
    memset(seen, 0, sizeof(seen));
    for (i = 0; i < 300; i++) {
        id = gen_item_random(1, AREA);
        if (id != NGE_ENTITY_INVALID && item_ok(id, AREA, kind_index[id])) {
            seen[kind_index[id]]++;
            nge_entity_destroy(id);
            nge_entity_release(nge_entity_next_dying());
        }
    }
    CHECK(seen[IT_POTION] + seen[IT_SWORD] + seen[IT_RING] + seen[IT_ANTIDOTE] + seen[IT_FOOD] +
          seen[IT_ELIXIR] + seen[IT_STRENGTH] == 300);
    /* Rarities 10, 5, 4 and 4, 3, 1 and 1 */
    CHECK(seen[IT_POTION] > seen[IT_SWORD] && seen[IT_SWORD] > seen[IT_RING] && seen[IT_RING] > seen[IT_ELIXIR] &&
          seen[IT_ELIXIR] > 0);
    CHECK(seen[IT_ANTIDOTE] > seen[IT_ELIXIR] && seen[IT_FOOD] > seen[IT_ELIXIR] && seen[IT_STRENGTH] > 0);
    /* Nothing appears at depth 0 */
    CHECK(gen_creature_random(0, AREA) == NGE_ENTITY_INVALID);
    CHECK(gen_item_random(0, AREA) == NGE_ENTITY_INVALID);
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX);

    /* gen_pick: the depth limit and the weights */
    ok = 1;
    for (i = 0; i < 200 && ok; i++) {
        ok = gen_pick(where, 3, 4) == 0;   /* kind 1 from depth 5; kind 2 rarity 0 */
    }
    CHECK(ok);
    lo = 0;
    for (i = 0; i < 200; i++) {
        lo += gen_pick(where, 3, 5) == 1;
    }
    CHECK(lo > 180);                                                 /* 100 against 1 */
    CHECK(gen_pick(where, 3, 0) == KIND_INVALID);
    /* A kind too deep, ahead of the one that appears, is passed over */
    ok = 1;
    for (i = 0; i < 200 && ok; i++) {
        ok = gen_pick(deep_first, 2, 4) == 1;
    }
    CHECK(ok);

    /* ---- no free entity: nothing made ---- */
    reset();
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(gen_creature(CR_RAT, AREA) == NGE_ENTITY_INVALID);
    CHECK(gen_item(IT_POTION, AREA) == NGE_ENTITY_INVALID);
    CHECK(gen_item_random(1, AREA) == NGE_ENTITY_INVALID);

    /* ---- asserts ---- */
    reset();
    TEST_EXPECT_HALT(gen_creature(CR_COUNT, AREA));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_item(IT_COUNT, AREA));
    CHECK(test_halt_count == 1);

    return test_summary("gen");
}
