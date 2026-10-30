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

#include "game/ai/ai_internal.h"
#include "game/effects/effects_game.h"
#include "game/gen/gen_internal.h"
#include "game/level/level.h"
#include "game/timers.h"

#define AREA  NGE_OWNER_AREA(0)

/* The sprite component matches a record's sprite part */
static int render_is(nge_entity_id_t id, const gen_render_t *r)
{
    return NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_SPRITE) &&
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
           !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION);
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
    if (!NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_BLOCK) || !NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS) ||
        !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_TIMER) || nge_block_of[id] != b->block.block || nge_block_pass[id] != b->block.pass) {
        return 0;
    }
    if (!NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH) ||
        health_hp[id] != b->health.hp_max || health_hp_max[id] != b->health.hp_max ||
        health_immune[id] != b->health.immune || health_resist[id] != b->health.resist ||
        health_vuln[id] != b->health.vuln || health_ac[id] != b->health.ac ||
        stats_speed[id] != b->stats.speed || !melee_is(id, &b->melee)) {
        return 0;
    }
    /* AI unless it is the player: its kind's brain, in its start state */
    if (AI_IS(id) != !(b->flags & CREATURE_PLAYER) || AI_IS(id) != AI_KIND_HAS(&b->ai) ||
        (AI_IS(id) && (ai_brain[id] != b->ai.brain || ai_get_state(id) != b->ai.state))) {
        return 0;
    }
    /* Mana if its kind has some, full */
    if (MANA_IS(id) != MANA_KIND_HAS(&b->mana) ||
        (MANA_IS(id) && (mana_mp[id] != b->mana.mp_max || mana_mp_max[id] != b->mana.mp_max))) {
        return 0;
    }
    /* Its abilities: the record's scores, as values and bases */
    for (a = 0; a < ABIL_COUNT; a++) {
        if (stats_ability[a][id] != b->stats.ability[a] || stats_ability_base[a][id] != b->stats.ability[a] ||
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
    return nge_timer_kind[id] == ((b->flags & CREATURE_PLAYER) ? TIMER_PLAYER : TIMER_AI) &&
           nge_timer_period[id] == b->stats.speed && nge_timer_count[id] == b->stats.speed &&
           nge_timer_cycles[id] == 0;
}

/* A table for GEN_PICK() */
typedef struct pick_kind {
    gen_where_t where;
} pick_kind_t;

static uint8_t pick(const pick_kind_t *table, uint8_t count, uint8_t depth)
{
    uint8_t kind;

    GEN_PICK(table, count, depth, kind);
    return kind;
}

/* A packed hook holds what the record says */
static int hook_is(uint16_t h, const gen_hook_t *hook)
{
    return GEN_HOOK_EF(h) == hook->ef && GEN_HOOK_POWER(h) == hook->power;
}

/* A packed hook is what an item kind's record says for that hook: its one hook, or none */
static int item_hook_is(uint16_t h, uint8_t kind, uint8_t hook)
{
    const gen_item_hook_t *k = &gen_items[kind].hook;

    return (k->hook == hook) ? GEN_HOOK_EF(h) == k->ef && GEN_HOOK_POWER(h) == k->power : GEN_HOOK_EF(h) == EF_NONE;
}

/* An item's attack components match its record's attack part: melee, or ranged at range 2 or more */
static int attack_ok(nge_entity_id_t id, const attack_kind_t *a)
{
    int ranged = ATTACK_KIND_HAS(a) && a->range > 1;

    if (!ATTACK_HAS(id, ATTACK_RANGED) != !ranged) {
        return 0;
    }
    if (!ranged) {
        return melee_is(id, a);
    }
    return !ATTACK_HAS(id, ATTACK_MELEE) && attack_range[ATTACK_RANGED][id] == a->range &&
           attack_dice[ATTACK_RANGED][id] == a->dice && attack_type[ATTACK_RANGED][id] == a->type;
}

/* An item kind's record is well formed for its subclass */
static int kind_ok(uint8_t kind)
{
    const gen_item_kind_t *k = &gen_items[kind];
    const gen_item_hook_t *h = &k->hook;
    uint8_t depth_max = k->where.depth_max;

    if (strlen(k->name) > 19 || k->subclass >= ITEM_COUNT || depth_max > LEVEL_DEPTH_BOTTOM ||
        k->where.depth < 1 || k->where.depth > depth_max || (k->fits & ~SLOTS_HUMANOID) != 0 ||
        (k->where.rarity == 0) != (kind == IT_QUEST)) {
        return 0;
    }
    switch (k->subclass) {
    case ITEM_WAND:
        return h->hook == GEN_HOOK_AIM && h->ef != EF_NONE && k->fits == 0 && k->count != 0;
    case ITEM_RANGED:
        return k->fits == SLOT_BIT(SLOT_RANGED) && k->attack.range >= 2 && k->attack.dice != 0 &&
               k->group != GROUP_NONE && k->group != GROUP_THROWN && k->group < GROUP_COUNT;
    case ITEM_AMMO:
        if (k->fits != SLOT_BIT(SLOT_AMMO) || k->count == 0 || k->group == GROUP_NONE || k->group >= GROUP_COUNT) {
            return 0;
        }
        return (k->group == GROUP_THROWN) ? k->attack.dice != 0 && k->attack.range >= 2 : k->attack.dice == 0;
    case ITEM_KEY:
        return k->group >= KEY_IRON && k->group < KEY_COUNT;
    case ITEM_WEAPON:
        return k->fits == SLOT_BIT(SLOT_WEAPON) && k->attack.dice != 0 && k->attack.range == 1;
    case ITEM_POTION:
    case ITEM_FOOD:
    case ITEM_DRINK:
        return h->hook == GEN_HOOK_USE && h->ef != EF_NONE;
    case ITEM_RING:
    case ITEM_AMULET:
    case ITEM_ARMOUR:
    case ITEM_LIGHT:
        return h->hook == GEN_HOOK_WORN && h->ef != EF_NONE && k->fits != 0;
    default:
        return 1;
    }
}

static int item_ok(nge_entity_id_t id, nge_owner_t owner, uint8_t kind)
{
    return made(id, owner, CLASS_ITEM, kind) && render_is(id, &gen_items[kind].render) &&
           ITEM_IS(id) && item_slot[id] == SLOT_NONE && attack_ok(id, &gen_items[kind].attack) && item_count[id] == gen_items[kind].count &&
           !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_BLOCK) && !NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS) &&
           !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_TIMER) && !NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_INVENTORY) &&
           !NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH) && !AI_IS(id);
}

static void reset(void)
{
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
}

int main(void)
{
    static const pick_kind_t where[3] = { { { 1, 9, 1 } }, { { 5, 9, 100 } }, { { 0, 9, 0 } } };
    static const pick_kind_t deep_first[2] = { { { 5, 9, 100 } }, { { 1, 9, 1 } } };
    static const pick_kind_t band[2] = { { { 1, 3, 1 } }, { { 4, 9, 1 } } };
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
        static const nge_component_t masks[] = {
            NGE_COMPONENT_LOCATION, NGE_COMPONENT_SPRITE, NGE_COMPONENT_BLOCK, NGE_COMPONENT_TIMER,
            COMPONENT_KIND, COMPONENT_STATS, COMPONENT_ITEM, COMPONENT_INVENTORY, COMPONENT_HEALTH,
            COMPONENT_MELEE, COMPONENT_RANGED, COMPONENT_EFFECT, COMPONENT_FEATURE, COMPONENT_MANA, COMPONENT_PC,
            COMPONENT_AI,
        };
        uint8_t seen[NGE_COMPONENT_TABLES] = {0};
        unsigned int m;

        ok = sizeof(masks) / sizeof(masks[0]) == NGE_COMPONENT_USER + COMPONENT_COUNT;
        for (m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
            unsigned int t = masks[m] >> 8;
            uint8_t bit = (uint8_t)masks[m];

            ok = ok && t < NGE_COMPONENT_TABLES && bit != 0 && (bit & (bit - 1)) == 0 && (seen[t] & bit) == 0;
            if (ok) {
                seen[t] |= bit;
            }
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
        nge_entity_destroy(id);                       /* more kinds than entities */
        nge_entity_release(nge_entity_next_dying());
    }
    CHECK(ok);
    CHECK(gen_item_fits(IT_HEAL_POTION) == 0 && gen_item_fits(IT_SWORD) == SLOT_BIT(SLOT_WEAPON) &&
          gen_item_fits(IT_RING_HEALTH_2) == (SLOT_BIT(SLOT_RING_L) | SLOT_BIT(SLOT_RING_R)));

    /* ---- packed hooks: a negative power survives ---- */
    CHECK(GEN_HOOK_EF(GEN_HOOK_PACK(EF_POISON, -3)) == EF_POISON && GEN_HOOK_POWER(GEN_HOOK_PACK(EF_POISON, -3)) == -3);
    CHECK(GEN_HOOK_POWER(GEN_HOOK_PACK(EF_COUNT - 1, 127)) == 127 && GEN_HOOK_EF(GEN_HOOK_PACK(EF_COUNT - 1, -128)) == EF_COUNT - 1);

    /* ---- hooks and subclasses: what the records say, by kind and by entity ---- */
    reset();
    ok = 1;
    for (kind = 0; kind < IT_COUNT; kind++) {
        id = gen_item(kind, AREA);
        ok = ok && gen_item_subclass(kind) == gen_items[kind].subclass;
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && item_hook_is(gen_item_hook(kind, (uint8_t)i), kind, (uint8_t)i) &&
                 item_hook_is(gen_hook_of(id, (uint8_t)i), kind, (uint8_t)i);
        }
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    for (kind = 0; kind < CR_COUNT; kind++) {
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && hook_is(gen_creature_hook(kind, (uint8_t)i), &gen_creatures[kind].hooks[i]) &&
                 gen_creatures[kind].hooks[i].ef < EF_COUNT;
        }
    }
    CHECK(ok);
    /* Features: names, hooks and info from their records; a door is a closed, openable, solid feature */
    for (kind = 0; kind < FT_COUNT; kind++) {
        gen_feature_info_t info;

        id = gen_feature(kind, AREA);
        gen_feature_info(kind, &info);
        gen_name(id, buf, sizeof(buf));
        ok = ok && !AI_IS(id) && strcmp(buf, gen_features[kind].name) == 0 && feature_state[id] == gen_features[kind].state &&
             memcmp(&info, &gen_features[kind].info, sizeof(info)) == 0;
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && hook_is(gen_feature_hook(kind, (uint8_t)i), &gen_features[kind].hooks[i]) &&
                 hook_is(gen_hook_of(id, (uint8_t)i), &gen_features[kind].hooks[i]);
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
        const gen_item_hook_t *h = &gen_items[kind].hook;

        ok = ok && h->ef < EF_COUNT && h->hook < GEN_HOOK_COUNT && h->hook != GEN_HOOK_INNATE;
        ok = ok && (h->ef == EF_NONE || ((effect_defs[h->ef].duration == EFFECT_WORN) == (h->hook == GEN_HOOK_WORN) &&
                                         h->power != 0));
    }
    for (kind = 0; kind < CR_COUNT; kind++) {
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            uint8_t ef = gen_creatures[kind].hooks[i].ef;

            ok = ok && (ef == EF_NONE || (i != GEN_HOOK_WORN && i != GEN_HOOK_USE &&
                                          effect_defs[ef].duration != EFFECT_WORN && gen_creatures[kind].hooks[i].power != 0));
        }
    }
    CHECK(ok);
    CHECK(GEN_HOOK_EF(gen_item_hook(IT_HEAL_POTION, GEN_HOOK_USE)) == EF_HEALING && GEN_HOOK_EF(gen_item_hook(IT_RING_HEALTH_2, GEN_HOOK_WORN)) == EF_WORN_HP &&
          GEN_HOOK_EF(gen_item_hook(IT_ANTIDOTE, GEN_HOOK_USE)) == EF_CURE && GEN_HOOK_EF(gen_item_hook(IT_DEAD_RAT, GEN_HOOK_USE)) == EF_POISON &&
          GEN_HOOK_EF(gen_item_hook(IT_ELIXIR, GEN_HOOK_USE)) == EF_VITALITY && GEN_HOOK_EF(gen_item_hook(IT_POTION_STR, GEN_HOOK_USE)) == EF_STRENGTH && GEN_HOOK_EF(gen_item_hook(IT_SWORD, GEN_HOOK_HIT)) == EF_NONE);
    CHECK(gen_item_subclass(IT_DEAD_RAT) == ITEM_FOOD && gen_item_subclass(IT_ELIXIR) == ITEM_POTION);
    id = gen_creature(CR_RAT, AREA);
    CHECK(GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_HIT)) == EF_POISON && GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_INNATE)) == EF_NONE);
    CHECK(!MANA_IS(id));                                       /* monsters have no mana yet */
    CHECK(!PC_IS(id) && AI_IS(id));                            /* a monster */
    CHECK(ai_brain[id] == AI_BRAIN_DEFAULT && ai_get_state(id) == AI_STATE_ASLEEP);
    id = gen_creature(CR_GOBLIN, AREA);
    CHECK(AI_IS(id) && ai_brain[id] == AI_BRAIN_DEFAULT && ai_get_state(id) == AI_STATE_ASLEEP);
    CHECK(ai_can(id) == (AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP | AI_CAN_USE | AI_CAN_OPEN));   /* Int 10, a weapon slot */
    id = gen_creature(CR_RAT, AREA);
    CHECK(ai_can(id) == AI_CAN_MELEE);                          /* Int 2, carries nothing: bites */
    CHECK(nge_owner_count(id) == 0);                           /* no innate effect */
    id = gen_creature(CR_PLAYER, AREA);
    CHECK(GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_INNATE)) == EF_REGEN && nge_owner_count(id) == 1);   /* its innate effect */
    CHECK(MANA_IS(id) && mana_mp[id] == 5 && mana_mp_max[id] == 5);                 /* full */
    CHECK(PC_IS(id) && pc_level == 1 && pc_xp == 0 && pc_gold == 0);                 /* a new character */
    CHECK(!AI_IS(id));
    CHECK(strcmp(pc_name, "Rogue") == 0 && pc_class == PC_CLASS_FIGHTER && pc_depth_max == 1);
    CHECK(gen_creature_xp(CR_RAT) == 5 && gen_creature_xp(CR_GOBLIN) == 5 && gen_creature_xp(CR_PLAYER) == 0);
    chest = nge_entity_create();                               /* no kind: no hooks */
    CHECK(GEN_HOOK_EF(gen_hook_of(chest, GEN_HOOK_USE)) == EF_NONE);
    TEST_EXPECT_HALT(gen_hook_of(id, GEN_HOOK_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_item_hook(IT_COUNT, GEN_HOOK_USE));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_creature_hook(CR_COUNT, GEN_HOOK_HIT));
    CHECK(test_halt_count == 1);

    /* Any owner: an item in a chest, a creature not yet placed */
    chest = gen_item(IT_SWORD, AREA);
    id = gen_item(IT_HEAL_POTION, chest);
    CHECK(item_ok(id, chest, IT_HEAL_POTION));
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
    for (i = 0; i < 3000; i++) {
        id = gen_item_random(1, AREA);
        if (id != NGE_ENTITY_INVALID && item_ok(id, AREA, kind_index[id])) {
            seen[kind_index[id]]++;
            nge_entity_destroy(id);
            nge_entity_release(nge_entity_next_dying());
        }
    }
    /* Rarities 12, 6 and 3 */
    CHECK(seen[IT_HEAL_DRAUGHT] > seen[IT_ARROWS] && seen[IT_ARROWS] > seen[IT_RING_HEALTH_1] && seen[IT_RING_HEALTH_1] > 0);
    CHECK(seen[IT_ELIXIR] == 0 && seen[IT_SWORD_3] == 0);        /* too deep for depth 1 */
    /* Nothing appears at depth 0 */
    CHECK(gen_creature_random(0, AREA) == NGE_ENTITY_INVALID);
    CHECK(gen_item_random(0, AREA) == NGE_ENTITY_INVALID);
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX);
    /* The quest item: never at random, at any depth (docs/winning.md R2) */
    ok = 1;
    for (i = 0; i < 1000 && ok; i++) {
        id = gen_item_random((uint8_t)(1 + i % LEVEL_DEPTH_BOTTOM), AREA);
        ok = id != NGE_ENTITY_INVALID && kind_index[id] != IT_QUEST;
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    CHECK(ok && nge_entity_free_count() == NGE_ENTITY_MAX);

    /* ---- the catalogue: every kind well formed, every subclass as its mechanics will need ---- */
    ok = 1;
    for (kind = 0; kind < IT_COUNT; kind++) {
        ok = ok && kind_ok(kind);
    }
    CHECK(ok);
    /* Each equipment slot has a kind that fits */
    ok = 1;
    for (i = 1; i < SLOT_COUNT; i++) {
        int fits = 0;

        for (kind = 0; kind < IT_COUNT; kind++) {
            fits = fits || (gen_items[kind].fits & SLOT_BIT(i)) != 0;
        }
        ok = ok && fits;
    }
    CHECK(ok);
    /* Each depth has kinds, and no subclass passes a quarter of the weight (docs/items.md 8.2) */
    ok = 1;
    for (i = 1; i <= LEVEL_DEPTH_BOTTOM; i++) {
        int share[ITEM_COUNT] = { 0 };
        int total = 0;
        int s;

        for (kind = 0; kind < IT_COUNT; kind++) {
            const gen_where_t *w = &gen_items[kind].where;

            if (i >= w->depth && i <= w->depth_max) {
                share[gen_items[kind].subclass] += w->rarity;
                total += w->rarity;
            }
        }
        ok = ok && total > 0;
        for (s = 0; s < ITEM_COUNT; s++) {
            ok = ok && share[s] * 4 <= total;
        }
    }
    CHECK(ok);
    /* A negative power and a ranged kind, as made */
    CHECK((int8_t)gen_items[IT_RING_FRAILTY].hook.power == -3 && GEN_HOOK_POWER(gen_item_hook(IT_RING_FRAILTY, GEN_HOOK_WORN)) == -3);
    id = gen_item(IT_LONG_BOW, AREA);
    CHECK(ATTACK_HAS(id, ATTACK_RANGED) && !ATTACK_HAS(id, ATTACK_MELEE) && attack_range[ATTACK_RANGED][id] == 8);
    id = gen_item(IT_ARROWS, AREA);
    CHECK(!ATTACK_HAS(id, ATTACK_RANGED) && !ATTACK_HAS(id, ATTACK_MELEE) && item_count[id] == 15);
    id = gen_item(IT_DARTS, AREA);
    CHECK(ATTACK_HAS(id, ATTACK_RANGED) && item_count[id] == 8);

    /* GEN_PICK: the depth limits and the weights */
    ok = 1;
    for (i = 0; i < 200 && ok; i++) {
        ok = pick(where, 3, 4) == 0;   /* kind 1 from depth 5; kind 2 rarity 0 */
    }
    CHECK(ok);
    lo = 0;
    for (i = 0; i < 200; i++) {
        lo += pick(where, 3, 5) == 1;
    }
    CHECK(lo > 180);                                                 /* 100 against 1 */
    CHECK(pick(where, 3, 0) == KIND_INVALID);
    /* A kind too deep, ahead of the one that appears, is passed over */
    ok = 1;
    for (i = 0; i < 200 && ok; i++) {
        ok = pick(deep_first, 2, 4) == 1;
    }
    CHECK(ok);
    /* A band ends at depth_max: past it, a kind is never picked */
    ok = 1;
    for (i = 0; i < 200 && ok; i++) {
        ok = pick(band, 2, 3) == 0 && pick(band, 2, 4) == 1 && pick(band, 2, 9) == 1;
    }
    CHECK(ok);
    CHECK(pick(band, 2, 10) == KIND_INVALID);

    /* ---- no free entity: nothing made ---- */
    reset();
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(gen_creature(CR_RAT, AREA) == NGE_ENTITY_INVALID);
    CHECK(gen_item(IT_HEAL_POTION, AREA) == NGE_ENTITY_INVALID);
    CHECK(gen_item_random(1, AREA) == NGE_ENTITY_INVALID);

    /* ---- asserts ---- */
    reset();
    TEST_EXPECT_HALT(gen_creature(CR_COUNT, AREA));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_item(IT_COUNT, AREA));
    CHECK(test_halt_count == 1);

    return test_summary("gen");
}
