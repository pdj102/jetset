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

#include "game/components/inventory_internal.h"
#include "game/components/attack_internal.h"
#include "game/attrs/attrs_internal.h"
#include "game/actors.h"
#include "game/ai/ai_internal.h"
#include "game/effects/effects_defs.h"
#include "game/gen/gen_internal.h"
#include "game/level/level.h"
#include "game/level/level_internal.h"
#include "game/spells/spells.h"
#include "game/spells/spells_internal.h"
#include "game/timers.h"
#include "game/effects/effects.h"

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
           attack_to_damage[ATTACK_MELEE][id] == m->to_damage && attack_type[ATTACK_MELEE][id] == m->type &&
           attack_range[ATTACK_MELEE][id] == 1;
}

/* A creature's ranged attack component matches its record's natural ranged part: none if its dice are 0 */
static int ranged_is(nge_entity_id_t id, const attack_kind_t *r)
{
    if (!ATTACK_KIND_HAS(r)) {
        return !ATTACK_HAS(id, ATTACK_RANGED);
    }
    return ATTACK_HAS(id, ATTACK_RANGED) && attack_to_hit[ATTACK_RANGED][id] == r->to_hit &&
           attack_dice[ATTACK_RANGED][id] == r->dice && attack_sides[ATTACK_RANGED][id] == r->sides &&
           attack_to_damage[ATTACK_RANGED][id] == r->to_damage && attack_type[ATTACK_RANGED][id] == r->type &&
           attack_range[ATTACK_RANGED][id] == r->range;
}

/* The items id owns are its kit, each equipped, and nothing else (R6) */
static int kit_equipped(nge_entity_id_t id, const gen_creature_kind_t *b)
{
    nge_entity_id_t owned;
    uint8_t found[GEN_KIT_MAX] = { 0 };
    uint8_t i;
    uint8_t k;

    NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
        if (!ITEM_IS(owned)) {
            continue;
        }
        for (k = 0; k < GEN_KIT_MAX; k++) {
            if (!found[k] && b->kit[k] != IT_NONE && kind_index[owned] == b->kit[k]) {
                break;
            }
        }
        if (k == GEN_KIT_MAX || item_slot[owned] == SLOT_NONE) {
            return 0;
        }
        found[k] = 1;
    }
    for (k = 0; k < GEN_KIT_MAX; k++) {
        if (found[k] != (b->kit[k] != IT_NONE)) {
            return 0;
        }
    }
    return 1;
}

/*
 * A creature kind's kit fits it: real item kinds, each fitting a slot the
 * kind has or ammo for its kit's launcher, and room for all (R6)
 */
static int kit_ok(uint8_t kind)
{
    const gen_creature_kind_t *b = &gen_creatures[kind];
    uint8_t k;
    uint8_t j;
    uint8_t item;
    uint8_t n = 0;
    int ammo;

    for (k = 0; k < GEN_KIT_MAX; k++) {
        item = b->kit[k];
        if (item == IT_NONE) {
            continue;
        }
        if (item >= IT_COUNT) {
            return 0;
        }
        n++;
        ammo = 0;
        for (j = 0; j < GEN_KIT_MAX; j++) {
            ammo = ammo || (gen_items[item].subclass == ITEM_AMMO && b->kit[j] != IT_NONE && b->kit[j] < IT_COUNT &&
                            gen_items[b->kit[j]].subclass == ITEM_RANGED &&
                            gen_item_ammo_group(b->kit[j]) == gen_item_ammo_group(item));
        }
        if ((gen_items[item].fits & b->inventory.slots) == 0 && !ammo) {
            return 0;
        }
    }
    return n <= b->inventory.capacity;
}

static int creature_ok(nge_entity_id_t id, nge_owner_t owner, uint8_t kind)
{
    const gen_creature_kind_t *b = &gen_creatures[kind];
    uint8_t ac = b->health.ac;   /* the record's, and its kit's armour, equipped (the hobgoblin's shield) */
    uint8_t a;

    for (a = 0; a < GEN_KIT_MAX; a++) {
        if (b->kit[a] != IT_NONE) {
            ac = (uint8_t)(ac + gen_item_armour_ac(b->kit[a]));
        }
    }

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
        health_vuln[id] != b->health.vuln || health_ac[id] != ac ||
        stats_speed[id] != b->stats.speed || !melee_is(id, &b->melee) || !ranged_is(id, &b->ranged)) {
        return 0;
    }
    /* Its kit, equipped (R6) */
    if (!kit_ok(kind) || !kit_equipped(id, b)) {
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
    /* Spells if its kind lists some: those and no others */
    if (SPELLS_IS(id) != (b->spells[0] != SP_NONE)) {
        return 0;
    }
    for (a = 1; a < SPELL_COUNT; a++) {
        if (spell_knows(id, a) != (memchr(b->spells, a, GEN_SPELLS_MAX) != NULL)) {
            return 0;
        }
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

/* A hook names a real effect, with a power exactly when it names one (items.md D3) */
static int hook_ok(const gen_hook_t *h)
{
    return h->ef < EF_COUNT && (h->ef == EF_NONE) == (h->power == 0);
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

/*
 * Each reader of the record's data (docs/items.md 4.7, R30) gives its member
 * for its subclasses and its "another subclass" value for the rest, and a
 * subclass with no meaning leaves the byte 0
 */
static int data_readers_ok(uint8_t kind)
{
    const gen_item_kind_t *k = &gen_items[kind];
    uint8_t sub = k->subclass;
    uint8_t ammo = (sub == ITEM_RANGED || sub == ITEM_AMMO);

    if (gen_item_ammo_group(kind) != (ammo ? k->data.ammo_group : GROUP_NONE) ||
        gen_item_key_lock_group(kind) != (sub == ITEM_KEY ? k->data.key_lock_group : KEY_NONE) ||
        gen_item_pick_bonus(kind) != (sub == ITEM_TOOL ? k->data.pick_bonus : 0) ||
        gen_item_armour_ac(kind) != (sub == ITEM_ARMOUR ? k->data.armour_ac : 0)) {
        return 0;
    }
    if (sub == ITEM_BOOK) {   /* its reader asserts on any other subclass */
        return gen_item_book_spells(kind) == k->data.book_spells;
    }
    return ammo || sub == ITEM_KEY || sub == ITEM_TOOL || sub == ITEM_ARMOUR || k->data.armour_ac == 0;
}

/* An item kind's record is well formed for its subclass */
static int kind_ok(uint8_t kind)
{
    const gen_item_kind_t *k = &gen_items[kind];
    const gen_item_hook_t *h = &k->hook;
    uint8_t depth_max = k->where.depth_max;

    if (strlen(k->name) > 19 || k->subclass >= ITEM_COUNT || depth_max > LEVEL_DEPTH_BOTTOM ||
        k->where.depth < 1 || k->where.depth > depth_max || (k->fits & ~SLOTS_HUMANOID) != 0 ||
        (k->where.rarity == 0) != (kind == IT_QUEST || k->subclass == ITEM_KEY)) {   /* never at random */
        return 0;
    }
    switch (k->subclass) {
    case ITEM_WAND:
        return h->hook == GEN_HOOK_AIM && h->ef != EF_NONE && k->fits == 0 && k->count != 0;
    case ITEM_RANGED:
        return k->fits == SLOT_BIT(SLOT_RANGED) && k->attack.range >= 2 && k->attack.dice != 0 &&
               k->data.ammo_group != GROUP_NONE && k->data.ammo_group != GROUP_THROWN && k->data.ammo_group < GROUP_COUNT;
    case ITEM_AMMO:
        if (k->fits != SLOT_BIT(SLOT_AMMO) || k->count == 0 || k->data.ammo_group == GROUP_NONE ||
            k->data.ammo_group >= GROUP_COUNT) {
            return 0;
        }
        return (k->data.ammo_group == GROUP_THROWN) ? k->attack.dice != 0 && k->attack.range >= 2 : k->attack.dice == 0;
    case ITEM_KEY:
        return k->data.key_lock_group >= KEY_IRON && k->data.key_lock_group < KEY_COUNT;
    case ITEM_BOOK:   /* a list of spells, never used, worn or aimed (docs/spells.md R40) */
        return k->data.book_spells < SPELL_BOOK_COUNT && h->ef == EF_NONE && k->fits == 0 &&
               k->attack.dice == 0 && k->count == 0;
    case ITEM_WEAPON:
        return k->fits == SLOT_BIT(SLOT_WEAPON) && k->attack.dice != 0 && k->attack.range == 1;
    case ITEM_POTION:
    case ITEM_FOOD:
    case ITEM_DRINK:
        return h->hook == GEN_HOOK_USE && h->ef != EF_NONE;
    case ITEM_ARMOUR:   /* an AC, a worn property, or both; its AC never on the hook (docs/items.md R21, R22) */
        if (h->ef == EF_NONE) {
            return k->fits != 0 && k->data.armour_ac != 0;
        }
        return k->fits != 0 && h->hook == GEN_HOOK_WORN && h->ef != EF_WORN_AC;
    case ITEM_RING:
    case ITEM_AMULET:
    case ITEM_LIGHT:
        return h->hook == GEN_HOOK_WORN && h->ef != EF_NONE && k->fits != 0;
    default:
        return 1;
    }
}

/* A feature kind's record is well formed for its abilities (docs/features.md, section 9) */
static int feature_kind_ok(uint8_t kind)
{
    const gen_feature_kind_t *k = &gen_features[kind];
    const gen_hook_t *use = &k->use;
    const gen_hook_t *hit = &k->hit;
    uint8_t can = k->info.can;

    if (strlen(k->name) > 19 || k->verb >= FVERB_COUNT || k->where.depth_max > LEVEL_DEPTH_BOTTOM) {
        return 0;
    }
    if (k->where.rarity != 0 && (k->where.depth < 1 || k->where.depth > k->where.depth_max)) {
        return 0;
    }
    if (!hook_ok(use) || !hook_ok(hit)) {
        return 0;
    }
    /* Operated: a use hook and a verb; nothing else has a use hook */
    if (((can & FCAN_OPERATE) != 0) != (use->ef != EF_NONE) || ((can & FCAN_OPERATE) && k->verb == FVERB_NONE)) {
        return 0;
    }
    /* A trap blocks nothing and does something when it fires; only a trap fires */
    if (can & FCAN_TRAP) {
        if (k->info.block_shut != NGE_BLOCK_NONE ||
            (hit->ef == EF_NONE && k->attack.dice == 0 && !(can & FCAN_DESCEND))) {
            return 0;
        }
    } else if (hit->ef != EF_NONE || k->attack.dice != 0 || (can & FCAN_DISARM)) {
        return 0;   /* a hit hook only on a trap (R17) */
    }
    /* Disarmed or found by a roll against a DC; a container holds something */
    if (((can & FCAN_DISARM) || (k->state & FEAT_HIDDEN)) && k->dc == 0) {
        return 0;
    }
    if (((can & FCAN_CONTAINER) != 0) != (k->holds != 0)) {
        return 0;
    }
    return k->health.hp_max == 0 || k->health.ac >= 1;
}

static int item_ok(nge_entity_id_t id, nge_owner_t owner, uint8_t kind)
{
    gen_render_t r = { gen_items[kind].render.pattern, 1, 0, gen_items[kind].render.flags, LAYER_OBJECTS };

    return made(id, owner, CLASS_ITEM, kind) && render_is(id, &r) &&
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
            NGE_COMPONENT_LOCATION, NGE_COMPONENT_SPRITE, NGE_COMPONENT_BLOCK, NGE_COMPONENT_TIMER, NGE_COMPONENT_EFFECT,
            COMPONENT_KIND, COMPONENT_STATS, COMPONENT_ITEM, COMPONENT_INVENTORY, COMPONENT_HEALTH,
            COMPONENT_MELEE, COMPONENT_RANGED, COMPONENT_FEATURE, COMPONENT_MANA, COMPONENT_PC,
            COMPONENT_AI, COMPONENT_SPELLS,
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
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        nge_event_init();   /* each kind's start (effects, its kit) on its own: they don't all fit the queue */
        id = gen_creature(kind, AREA);
        ok = ok && creature_ok(id, AREA, kind);
        /* A monster's kit, innate effects and mana regeneration fit in what it may own (R12) */
        ok = ok && (PC_IS(id) || nge_owner_count(id) <= LEVEL_CREATURE_OWNED);
    }
    CHECK(ok);
    /* The goblin archer: its bow ready, its arrows in the quiver */
    reset();
    nge_event_init();
    id = gen_creature(CR_GOBLIN_ARCHER, AREA);
    CHECK(inv_equipped(id, SLOT_RANGED) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_RANGED)] == IT_SHORT_BOW &&
          inv_equipped(id, SLOT_AMMO) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_AMMO)] == IT_ARROWS &&
          nge_owner_count(id) == 2 && !ATTACK_HAS(id, ATTACK_RANGED));   /* it shoots with the bow, not its own attack */
    CHECK(gen_creatures[CR_GOBLIN_ARCHER].where.rarity != 0);
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
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
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        id = gen_item(kind, AREA);
        attack_kind_t attack;

        ok = ok && gen_item_subclass(kind) == gen_items[kind].subclass;
        ok = ok && gen_item_flags(kind) == gen_items[kind].flags;
        ok = ok && data_readers_ok(kind);
        gen_item_attack(kind, &attack);
        ok = ok && memcmp(&attack, &gen_items[kind].attack, sizeof(attack)) == 0;
        for (i = 0; i < GEN_HOOK_COUNT; i++) {
            ok = ok && item_hook_is(gen_item_hook(kind, (uint8_t)i), kind, (uint8_t)i);
        }
        /* A hit by it, melee or ranged, applies its hit hook (R17) */
        ok = ok && item_hook_is(gen_hit_of(id, ATTACK_MELEE), kind, GEN_HOOK_HIT) &&
             item_hook_is(gen_hit_of(id, ATTACK_RANGED), kind, GEN_HOOK_HIT);
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    /* A creature's hit hooks are its attacks', each set only with its attack; a ranged part reaches 2 or more (R7, R17) */
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        const gen_creature_kind_t *k = &gen_creatures[kind];

        reset();
        nge_event_init();
        id = gen_creature(kind, AREA);
        ok = ok && hook_ok(&k->melee_hit) && hook_ok(&k->innate) &&
             hook_ok(&k->ranged_hit) && (k->melee_hit.ef == EF_NONE || k->melee.dice != 0) &&
             (k->ranged_hit.ef == EF_NONE || k->ranged.dice != 0) && (k->ranged.dice == 0 || k->ranged.range >= 2) &&
             hook_is(gen_creature_hit(kind, ATTACK_MELEE), &k->melee_hit) &&
             hook_is(gen_creature_hit(kind, ATTACK_RANGED), &k->ranged_hit) &&
             hook_is(gen_hit_of(id, ATTACK_MELEE), &k->melee_hit) && hook_is(gen_hit_of(id, ATTACK_RANGED), &k->ranged_hit);
        /* Its death and struck hooks: instant or timed effects, never worn (M4.1, M4.2) */
        ok = ok && hook_ok(&k->death) && hook_ok(&k->struck) &&
             (k->death.ef == EF_NONE || effect_defs[k->death.ef].duration != EFFECT_WORN) &&
             (k->struck.ef == EF_NONE || effect_defs[k->struck.ef].duration != EFFECT_WORN) &&
             hook_is(gen_creature_death(kind), &k->death) && hook_is(gen_creature_struck(kind), &k->struck);
    }
    /* The kinds that have them: the gas spore's burst; the floating eye's, the salamander's and the pudding's struck */
    ok = ok && GEN_HOOK_EF(gen_creature_death(CR_GAS_SPORE)) == EF_BOLT_FIRE && GEN_HOOK_POWER(gen_creature_death(CR_GAS_SPORE)) == 8 &&
         GEN_HOOK_EF(gen_creature_struck(CR_FLOATING_EYE)) == EF_PARALYSE &&
         GEN_HOOK_EF(gen_creature_struck(CR_SALAMANDER)) == EF_BURNING &&
         GEN_HOOK_EF(gen_creature_struck(CR_BLACK_PUDDING)) == EF_CORROSION &&
         GEN_HOOK_EF(gen_creature_death(CR_GOBLIN)) == EF_NONE && GEN_HOOK_EF(gen_creature_struck(CR_GOBLIN)) == EF_NONE;
    reset();
    CHECK(ok);
    /* Features: names, hooks and info from their records; a door is a closed, openable, solid feature */
    for (kind = FT_NONE + 1; kind < FT_COUNT; kind++) {
        gen_feature_info_t info;

        id = gen_feature(kind, AREA);
        gen_feature_info(kind, &info);
        gen_name(id, buf, sizeof(buf));
        ok = ok && !AI_IS(id) && strcmp(buf, gen_features[kind].name) == 0 && feature_state[id] == gen_features[kind].state &&
             memcmp(&info, &gen_features[kind].info, sizeof(info)) == 0;
        ok = ok && hook_is(gen_feature_use(kind), &gen_features[kind].use) &&
             hook_is(gen_feature_hit(kind), &gen_features[kind].hit) &&
             hook_is(gen_hit_of(id, ATTACK_MELEE), &gen_features[kind].hit);
        /* Its parts: health if it can be broken, a melee attack if it strikes */
        ok = ok && feature_kind_ok(kind) &&
             (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_HEALTH) != 0) == (gen_features[kind].health.hp_max != 0) &&
             (gen_features[kind].health.hp_max == 0 || health_hp[id] == gen_features[kind].health.hp_max) &&
             (gen_features[kind].attack.dice == 0 ? !ATTACK_HAS(id, ATTACK_MELEE)
                                                   : melee_is(id, &gen_features[kind].attack)) &&
             gen_feature_dc(kind) == gen_features[kind].dc && gen_feature_holds(kind) == gen_features[kind].holds &&
             gen_feature_verb(kind) == gen_features[kind].verb;
    }
    CHECK(ok);
    CHECK(gen_features[FT_DOOR].info.can == FCAN_OPEN && gen_features[FT_DOOR].state == 0 &&
          gen_features[FT_DOOR].where.rarity == 0);
    TEST_EXPECT_HALT(gen_feature_use(FT_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_feature_hit(FT_COUNT));
    CHECK(test_halt_count == 1);

    /* A worn hook names a worn effect; the other hooks name effects that can be applied */
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        const gen_item_hook_t *h = &gen_items[kind].hook;

        ok = ok && h->ef < EF_COUNT && h->hook < GEN_HOOK_COUNT;
        ok = ok && (h->ef == EF_NONE || ((effect_defs[h->ef].duration == EFFECT_WORN) == (h->hook == GEN_HOOK_WORN) &&
                                         h->power != 0));
    }
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        uint8_t hit = gen_creatures[kind].melee_hit.ef;
        uint8_t innate = gen_creatures[kind].innate.ef;

        /* A hit names an effect that can be applied, and not an innate one; innate names only an innate one */
        ok = ok && (hit == EF_NONE || (effect_defs[hit].duration != EFFECT_WORN && !(effect_defs[hit].flags & EFFECT_INNATE)));
        ok = ok && (innate == EF_NONE || (effect_defs[innate].flags & EFFECT_INNATE));
    }
    CHECK(ok);
    CHECK(GEN_HOOK_EF(gen_item_hook(IT_HEAL_POTION, GEN_HOOK_USE)) == EF_HEALING && GEN_HOOK_EF(gen_item_hook(IT_RING_HEALTH_2, GEN_HOOK_WORN)) == EF_WORN_HP &&
          GEN_HOOK_EF(gen_item_hook(IT_ANTIDOTE, GEN_HOOK_USE)) == EF_CURE && GEN_HOOK_EF(gen_item_hook(IT_DEAD_RAT, GEN_HOOK_USE)) == EF_POISON &&
          GEN_HOOK_EF(gen_item_hook(IT_ELIXIR, GEN_HOOK_USE)) == EF_VITALITY && GEN_HOOK_EF(gen_item_hook(IT_POTION_STR, GEN_HOOK_USE)) == EF_RAISE_STR && GEN_HOOK_EF(gen_item_hook(IT_SWORD, GEN_HOOK_HIT)) == EF_NONE);
    CHECK(gen_item_ammo_group(IT_SHORT_BOW) == GROUP_ARROW && gen_item_ammo_group(IT_DARTS) == GROUP_THROWN &&
          gen_item_ammo_group(IT_SWORD) == GROUP_NONE);
    /* The ego armour (docs/items.md 4.11): a plain kind's AC and a worn property */
    CHECK(gen_item_armour_ac(IT_MAIL_FIRE_WARD) == 5 && GEN_HOOK_EF(gen_item_hook(IT_MAIL_FIRE_WARD, GEN_HOOK_WORN)) == EF_WORN_RES_FIRE &&
          gen_item_armour_ac(IT_DRAGON_MAIL) == 6 && GEN_HOOK_EF(gen_item_hook(IT_DRAGON_MAIL, GEN_HOOK_WORN)) == EF_WORN_IMMUNE_FIRE &&
          gen_item_armour_ac(IT_ELVEN_CLOAK) == 1 && GEN_HOOK_EF(gen_item_hook(IT_ELVEN_CLOAK, GEN_HOOK_WORN)) == EF_WORN_DEX &&
          gen_item_armour_ac(IT_SHIELD_WARDING) == 2 && GEN_HOOK_EF(gen_item_hook(IT_SHIELD_WARDING, GEN_HOOK_WORN)) == EF_WORN_RES_MAGIC);
    CHECK(IT_COUNT == IT_SP_PANIC + 1 && gen_item_fits(IT_DRAGON_MAIL) == SLOT_BIT(SLOT_BODY) &&
          gen_item_fits(IT_ELVEN_CLOAK) == SLOT_BIT(SLOT_CLOAK) && gen_item_fits(IT_SHIELD_WARDING) == SLOT_BIT(SLOT_SHIELD));
    CHECK(gen_item_key_lock_group(IT_KEY_SKELETON) == KEY_ANY && gen_item_key_lock_group(IT_DARTS) == KEY_NONE &&
          gen_item_pick_bonus(IT_FINE_LOCK_PICK) == 3 && gen_item_pick_bonus(IT_KEY_GOLD) == 0);
    CHECK(gen_item_flags(IT_QUARTERSTAFF) == ITEMF_TWO_HANDS && gen_item_flags(IT_RUSTY_SWORD) == ITEMF_CURSED &&
          gen_item_flags(IT_SWORD) == 0);
    /*
     * The shape field holds a valid value (docs/items_aimed.md 4.2), and
     * direct items are exactly those that don't fly (docs/action_aim.md,
     * docs/items_extensions.md)
     */
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        ok = ok && (gen_item_flags(kind) & ITEMF_SHAPE) <= ITEMF_BURST;
    }
    CHECK(ok);
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        ok = ok && ((gen_item_flags(kind) & ITEMF_SHAPE) == ITEMF_DIRECT) ==
                       (kind == IT_SP_SLEEP || kind == IT_WAND_SLOW || kind == IT_WAND_SLEEP ||
                        kind == IT_WAND_TELEPORT || kind == IT_STAFF_HEALING ||
                        kind == IT_SP_TERROR || kind == IT_SP_CONFUSE || kind == IT_SP_HOLD ||
                        kind == IT_SP_HEX || kind == IT_SP_BANE || kind == IT_WAND_WEAKNESS);
    }
    CHECK(ok);
    /* gen_item_shape(): the field's value as a SPELL_*, for every aim kind */
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        if (GEN_HOOK_EF(gen_item_hook(kind, GEN_HOOK_AIM)) != EF_NONE) {
            uint8_t field = gen_item_flags(kind) & ITEMF_SHAPE;

            ok = ok && gen_item_shape(kind) == ((field == ITEMF_DIRECT) ? SPELL_DIRECT :
                                                (field == ITEMF_TILE)   ? SPELL_TILE :
                                                (field == ITEMF_BALL)   ? SPELL_BALL :
                                                (field == ITEMF_BURST)  ? SPELL_BURST : SPELL_BOLT);
        }
    }
    CHECK(ok && gen_item_shape(IT_WAND_SLEEP) == SPELL_DIRECT && gen_item_shape(IT_WAND_FIRE) == SPELL_BOLT &&
          gen_item_shape(IT_SWORD) == SPELL_BOLT && gen_item_shape(IT_SP_BLINK) == SPELL_TILE &&
          gen_item_shape(IT_STAFF_BLINK) == SPELL_TILE);
    /*
     * Only tile items have an effect that needs a tile (EF_BLINK), every one
     * has one, and it is aimed (docs/items_aimed.md R20)
     */
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        const gen_item_hook_t *h = &gen_items[kind].hook;
        int tile = (gen_items[kind].flags & ITEMF_SHAPE) == ITEMF_TILE;

        ok = ok && (h->ef == EF_BLINK) == tile && (!tile || h->hook == GEN_HOOK_AIM);
    }
    CHECK(ok);
    /* The tile items (docs/items_aimed.md 4.10) */
    CHECK(gen_item_subclass(IT_SP_BLINK) == ITEM_PARCHMENT && gen_items[IT_SP_BLINK].count == 0 &&
          strcmp(gen_items[IT_SP_BLINK].name, "blink parchment") == 0);
    CHECK(gen_item_subclass(IT_STAFF_BLINK) == ITEM_WAND && gen_items[IT_STAFF_BLINK].count == 6 &&
          strcmp(gen_items[IT_STAFF_BLINK].name, "staff of blinking") == 0);
    /* Every ball and burst kind is aimed; the four of docs/items_aimed.md 4.10 */
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        uint8_t field = gen_items[kind].flags & ITEMF_SHAPE;

        ok = ok && ((field != ITEMF_BALL && field != ITEMF_BURST) || gen_items[kind].hook.hook == GEN_HOOK_AIM);
    }
    CHECK(ok);
    CHECK(gen_item_shape(IT_WAND_FIREBALL) == SPELL_BALL && gen_item_shape(IT_WAND_FROSTBALL) == SPELL_BALL &&
          gen_item_shape(IT_SP_FIREBALL) == SPELL_BALL && gen_item_shape(IT_SP_PANIC) == SPELL_BURST);
    CHECK(GEN_HOOK_EF(gen_item_hook(IT_WAND_FIREBALL, GEN_HOOK_AIM)) == EF_BOLT_FIRE &&
          GEN_HOOK_POWER(gen_item_hook(IT_WAND_FIREBALL, GEN_HOOK_AIM)) == 4 && gen_items[IT_WAND_FIREBALL].count == 4);
    CHECK(GEN_HOOK_EF(gen_item_hook(IT_WAND_FROSTBALL, GEN_HOOK_AIM)) == EF_BOLT_COLD &&
          strcmp(gen_items[IT_WAND_FROSTBALL].name, "wand of frost balls") == 0);
    CHECK(GEN_HOOK_POWER(gen_item_hook(IT_SP_FIREBALL, GEN_HOOK_AIM)) == 5 &&
          gen_item_subclass(IT_SP_FIREBALL) == ITEM_PARCHMENT);
    CHECK(GEN_HOOK_EF(gen_item_hook(IT_SP_PANIC, GEN_HOOK_AIM)) == EF_FEAR &&
          strcmp(gen_items[IT_SP_PANIC].name, "panic parchment") == 0);
    /*
     * Every effect is made by some item kind (docs/items_extensions.md R1),
     * but for those meant for monsters and innate ones
     */
    ok = 1;
    for (i = 1; i < EF_COUNT; i++) {
        int used = i == EF_REGEN || i == EF_ROOT || i == EF_HOLD || i == EF_MANA_REGEN ||
                   (i >= EF_DRAIN_STR && i <= EF_DRAIN_CHA) || i == EF_ERRATIC || i == EF_WAKE ||
                   i == EF_SUMMON_UNDEAD;   /* the bat's, the shriek's, the necromancer's */

        for (kind = IT_NONE + 1; kind < IT_COUNT && !used; kind++) {
            used = gen_items[kind].hook.ef == i;
        }
        ok = ok && used;
    }
    CHECK(ok);
    /* The flask of oil sets what it hits burning (R11) */
    CHECK(GEN_HOOK_EF(gen_item_hook(IT_OIL, GEN_HOOK_HIT)) == EF_BURNING && GEN_HOOK_POWER(gen_item_hook(IT_OIL, GEN_HOOK_HIT)) == 4);
    {
        attack_kind_t attack;

        gen_item_attack(IT_LONG_BOW, &attack);
        CHECK(attack.to_hit == 1 && attack.dice == 1 && attack.sides == 8 && attack.range == 8);
        gen_item_attack(IT_HEAL_POTION, &attack);
        CHECK(attack.dice == 0);
    }
    CHECK(gen_item_subclass(IT_DEAD_RAT) == ITEM_FOOD && gen_item_subclass(IT_ELIXIR) == ITEM_POTION);
    id = gen_creature(CR_RAT, AREA);
    CHECK(GEN_HOOK_EF(gen_hit_of(id, ATTACK_MELEE)) == EF_POISON && gen_creatures[CR_RAT].innate.ef == EF_NONE);
    CHECK(!MANA_IS(id));                                       /* monsters have no mana yet */
    CHECK(!PC_IS(id) && AI_IS(id));                            /* a monster */
    CHECK(ai_brain[id] == AI_BRAIN_DEFAULT && ai_get_state(id) == AI_STATE_ASLEEP);
    id = gen_creature(CR_GOBLIN, AREA);
    CHECK(AI_IS(id) && ai_brain[id] == AI_BRAIN_DEFAULT && ai_get_state(id) == AI_STATE_ASLEEP);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP | AI_CAN_USE | AI_CAN_OPEN));   /* Int 10, a weapon slot */
    id = gen_creature(CR_RAT, AREA);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE));                          /* Int 2, carries nothing: bites */
    CHECK(nge_owner_count(id) == 0);                           /* no innate effect */
    /* Rooted when made: these four, and no other; a lichen's hold gets its save before its next hit */
    ok = 1;
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        reset();
        nge_event_init();
        id = gen_creature(kind, AREA);
        ok = ok && STATUS_HAS(id, STATUS_ROOTED) == (kind == CR_LICHEN || kind == CR_SHRIEKER ||
                                                     kind == CR_FLOATING_EYE || kind == CR_MIMIC);
    }
    CHECK(ok);
    /* Mana regeneration: a rule for every kind with mana, after its innate hook (docs/spells.md, D12) */
    ok = 1;
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        nge_entity_id_t owned;
        uint8_t i;
        int n = 0;

        reset();
        nge_event_init();
        id = gen_creature(kind, AREA);
        NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
            n += NGE_ENTITY_HAS_COMPONENT(owned, NGE_COMPONENT_EFFECT) && EFFECT_KIND_OF(owned) == EF_MANA_REGEN;
        }
        ok = ok && n == (MANA_KIND_HAS(&gen_creatures[kind].mana) ? 1 : 0);
    }
    CHECK(ok);
    CHECK(effect_defs[EF_HOLD].period < gen_creatures[CR_LICHEN].stats.speed);
    CHECK(GEN_HOOK_EF(gen_creature_hit(CR_LICHEN, ATTACK_MELEE)) == EF_HOLD && gen_creatures[CR_LICHEN].innate.ef == EF_ROOT);
    reset();
    id = gen_creature(CR_TROLL, AREA);
    CHECK(gen_creatures[CR_TROLL].innate.ef == EF_REGEN && nge_owner_count(id) == 1);   /* regenerates */
    id = gen_creature(CR_GOBLIN_SHAMAN, AREA);
    CHECK(MANA_IS(id) && mana_mp[id] == 6);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP | AI_CAN_USE | AI_CAN_OPEN |
                         AI_CAN_CAST | AI_CAN_SKIRMISH | AI_CAN_SUPPORT));   /* Int 13, a skirmisher (M1.3) that mends (M2.2) */
    id = gen_creature(CR_GOBLIN_ARCHER, AREA);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP | AI_CAN_USE | AI_CAN_OPEN |
                         AI_CAN_RANGED | AI_CAN_SKIRMISH));    /* its kit's arrows */
    id = gen_creature(CR_GOLD_SNAKE, AREA);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE | AI_CAN_RANGED));   /* spits, but stands its ground */
    /* docs/creature_extensions.md 6.1 (plan step 6): kits, a spray, a song, and the hobgoblin's shield in its AC */
    reset();
    nge_event_init();
    id = gen_creature(CR_HOBGOBLIN, AREA);
    CHECK(inv_equipped(id, SLOT_WEAPON) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_WEAPON)] == IT_SHORT_SWORD &&
          inv_equipped(id, SLOT_SHIELD) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_SHIELD)] == IT_SHIELD &&
          health_ac[id] > gen_creatures[CR_HOBGOBLIN].health.ac && (ai_can(id) & AI_CAN_USE));   /* drinks potions */
    id = gen_creature(CR_KOBOLD_SLINGER, AREA);
    CHECK(inv_equipped(id, SLOT_AMMO) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_AMMO)] == IT_STONES &&
          (ai_can(id) & AI_CAN_SKIRMISH));
    id = gen_creature(CR_ORC_ARCHER, AREA);
    CHECK(inv_equipped(id, SLOT_RANGED) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_RANGED)] == IT_CROSSBOW);
    id = gen_creature(CR_BOMBARDIER, AREA);
    CHECK(ATTACK_HAS(id, ATTACK_RANGED) && GEN_HOOK_EF(gen_hit_of(id, ATTACK_RANGED)) == EF_CORROSION &&
          !(ai_can(id) & AI_CAN_SKIRMISH));
    id = gen_creature(CR_HARPY, AREA);
    CHECK(spell_knows(id, SP_CONFUSE) && mana_mp[id] == 6 && nge_block_pass[id] == NGE_BLOCK_NO_FLOOR);
    CHECK(GEN_HOOK_EF(gen_creature_hit(CR_GHOUL, ATTACK_MELEE)) == EF_PARALYSE &&
          GEN_HOOK_EF(gen_creature_hit(CR_SHADOW, ATTACK_MELEE)) == EF_DRAIN_STR &&
          GEN_HOOK_EF(gen_creature_hit(CR_STIRGE, ATTACK_MELEE)) == EF_BLEEDING);
    reset();
    nge_event_init();
    /* Only the archers, the slinger and the casters who keep their distance are skirmishers */
    ok = 1;
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        ok = ok && (gen_creatures[kind].ai.traits == AI_TRAIT_SKIRMISH) ==
                   (kind == CR_GOBLIN_ARCHER || kind == CR_GOBLIN_SHAMAN || kind == CR_KOBOLD_SLINGER ||
                    kind == CR_ORC_ARCHER || kind == CR_SKELETON_ARCHER || kind == CR_NECROMANCER || kind == CR_LICH);
    }
    CHECK(ok);
    /* The families and the mindless, as docs/creature_extensions.md 6.1 to 6.3 give them (M4.3) */
    {
        static const uint8_t family[CR_COUNT] = {
            [CR_RAT] = CREATURE_BEAST,
            [CR_JACKAL] = CREATURE_BEAST, [CR_FIRE_BEETLE] = CREATURE_BEAST, [CR_WOLF] = CREATURE_BEAST,
            [CR_BOMBARDIER] = CREATURE_BEAST, [CR_OWLBEAR] = CREATURE_BEAST, [CR_WARG] = CREATURE_BEAST,
            [CR_BASILISK] = CREATURE_BEAST, [CR_HELL_HOUND] = CREATURE_BEAST,
            [CR_GHOST] = CREATURE_UNDEAD, [CR_WRAITH] = CREATURE_UNDEAD, [CR_GHOUL] = CREATURE_UNDEAD,
            [CR_WIGHT] = CREATURE_UNDEAD, [CR_BANSHEE] = CREATURE_UNDEAD, [CR_VAMPIRE] = CREATURE_UNDEAD,
            [CR_LICH] = CREATURE_UNDEAD,
            [CR_SKELETON] = CREATURE_UNDEAD | CREATURE_MINDLESS, [CR_ZOMBIE] = CREATURE_UNDEAD | CREATURE_MINDLESS,
            [CR_SHADOW] = CREATURE_UNDEAD | CREATURE_MINDLESS, [CR_MUMMY] = CREATURE_UNDEAD | CREATURE_MINDLESS,
            [CR_SKELETON_ARCHER] = CREATURE_UNDEAD | CREATURE_MINDLESS,
            [CR_BLACK_PUDDING] = CREATURE_MINDLESS,
            [CR_OGRE] = CREATURE_GIANT, [CR_TROLL] = CREATURE_GIANT, [CR_HILL_GIANT] = CREATURE_GIANT,
            [CR_MINOTAUR] = CREATURE_GIANT,
            [CR_DRAGON] = CREATURE_DRAGON, [CR_WYVERN] = CREATURE_DRAGON, [CR_FROST_DRAKE] = CREATURE_DRAGON,
        };

        ok = 1;
        for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
            ok = ok && (gen_creature_flags(kind) & ~CREATURE_PLAYER) == family[kind];
        }
        CHECK(ok);
    }
    /*
     * The bands (docs/creature_extensions.md M3): real kinds; a member CR_NONE
     * exactly when it has none; at most LEVEL_MONSTERS_MAX creatures (R8); the
     * leader met at every depth of its band; members asleep with their leader
     */
    ok = 1;
    for (i = 0; i < GEN_BANDS; i++) {
        const gen_band_t *b = &gen_bands[i];
        const gen_creature_kind_t *l = &gen_creatures[b->leader];
        int size = 1;
        int m;

        ok = ok && b->leader > CR_NONE && b->leader < CR_COUNT && l->ai.brain != AI_BRAIN_NONE && b->where.rarity > 0 &&
             b->where.depth >= l->where.depth && b->where.depth_max <= l->where.depth_max &&
             b->where.depth <= b->where.depth_max;
        for (m = 0; m < GEN_BAND_MEMBERS; m++) {
            if (b->member[m] == CR_NONE) {
                ok = ok && b->min[m] == 0 && b->max[m] == 0;
            } else {
                ok = ok && b->member[m] < CR_COUNT && gen_creatures[b->member[m]].ai.brain != AI_BRAIN_NONE &&
                     b->min[m] <= b->max[m] && b->max[m] > 0 &&
                     gen_creatures[b->member[m]].ai.state == l->ai.state;
                size += b->max[m];
            }
        }
        ok = ok && size <= LEVEL_MONSTERS_MAX;
    }
    CHECK(ok);
    /* The pick: none for max 0; one kind for max 1; a band leader first and members in its bounds, about 1 draw in 4 */
    {
        uint8_t kinds[LEVEL_MONSTERS_MAX];
        uint8_t n;
        int bands = 0;
        int k;

        CHECK(gen_band_pick(2, kinds, 0) == 0);
        ok = 1;
        for (k = 0; k < 400; k++) {
            n = gen_band_pick(1, kinds, 1);
            ok = ok && n == 1 && kinds[0] > CR_NONE && kinds[0] < CR_COUNT &&
                 gen_creatures[kinds[0]].where.depth <= 1 && gen_creatures[kinds[0]].where.depth_max >= 1;
        }
        CHECK(ok);
        ok = 1;
        for (k = 0; k < 2000; k++) {
            n = gen_band_pick(2, kinds, LEVEL_MONSTERS_MAX);
            ok = ok && n >= 1 && n <= 5;   /* the largest band at depth 2: the goblin war band */
            if (n > 1) {
                bands++;
                ok = ok && (kinds[0] == CR_KOBOLD || kinds[0] == CR_JACKAL || kinds[0] == CR_GOBLIN);
                if (kinds[0] == CR_JACKAL) {
                    ok = ok && n == 3 && kinds[1] == CR_JACKAL && kinds[2] == CR_JACKAL;
                }
                if (kinds[0] == CR_GOBLIN) {
                    ok = ok && n >= 3 && kinds[1] == CR_GOBLIN && kinds[n - 1] != CR_KOBOLD;
                }
            }
        }
        CHECK(ok && bands > 2000 / 4 - 100 && bands < 2000 / 4 + 100);
        /* Cut short at max: a band of 3 jackals in 2 draws is 2 */
        ok = 1;
        for (k = 0; k < 400; k++) {
            ok = ok && gen_band_pick(1, kinds, 2) <= 2;
        }
        CHECK(ok);
    }
    /* The necromancer: its staff wielded and its robe worn, a caster who keeps its distance and drinks potions */
    id = gen_creature(CR_NECROMANCER, AREA);
    CHECK(inv_equipped(id, SLOT_WEAPON) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_WEAPON)] == IT_QUARTERSTAFF &&
          inv_equipped(id, SLOT_BODY) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_BODY)] == IT_ROBE);
    CHECK(spell_knows(id, SP_SUMMON_UNDEAD) && spell_knows(id, SP_HOLD) && mana_mp[id] == 14 &&
          (ai_can(id) & (AI_CAN_CAST | AI_CAN_USE | AI_CAN_SKIRMISH)) == (AI_CAN_CAST | AI_CAN_USE | AI_CAN_SKIRMISH));
    /* The orc warlord: its battle axe wielded and its chain mail worn; a war cry, no ally spell (M2.2) */
    id = gen_creature(CR_ORC_WARLORD, AREA);
    CHECK(inv_equipped(id, SLOT_WEAPON) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_WEAPON)] == IT_BATTLE_AXE &&
          inv_equipped(id, SLOT_BODY) != NGE_ENTITY_INVALID && kind_index[inv_equipped(id, SLOT_BODY)] == IT_CHAIN_MAIL &&
          spell_knows(id, SP_WAR_CRY) && !(ai_can(id) & AI_CAN_SUPPORT));
    id = gen_creature(CR_LICH, AREA);
    CHECK(spell_knows(id, SP_SUMMON_UNDEAD) && mana_mp[id] == 30 && GEN_HOOK_EF(gen_hit_of(id, ATTACK_MELEE)) == EF_PARALYSE &&
          !INV_HAS(id));
    reset();
    nge_event_init();
    id = gen_creature(CR_SHRIEKER, AREA);
    CHECK(ai_can(id) == AI_CAN_CAST && spell_knows(id, SP_SHRIEK));   /* Int 1, no attack, carries nothing: it shrieks */
    id = gen_creature(CR_DRAGON, AREA);
    CHECK(spell_knows(id, SP_FIRE_BREATH) && mana_mp[id] == 12 && (health_immune[id] & DMG_BIT(DMG_FIRE)));   /* R9 */
    id = gen_creature(CR_BAT, AREA);
    CHECK(STATUS_HAS(id, STATUS_CONFUSED));                    /* erratic flight (M1.4) */
    id = gen_creature(CR_OGRE, AREA);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP | AI_CAN_USE | AI_CAN_OPEN));   /* Int 6 */
    id = gen_creature(CR_SKELETON, AREA);
    CHECK(ai_can(id) == (AI_CAN_MOVE | AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP));   /* Int 3: mindless at doors */
    id = gen_creature(CR_PLAYER, AREA);
    CHECK(gen_creatures[CR_PLAYER].innate.ef == EF_REGEN && nge_owner_count(id) == 2);   /* its innate effect, and mana regeneration */
    CHECK(MANA_IS(id) && mana_mp[id] == 5 && mana_mp_max[id] == 5);                 /* full */
    CHECK(PC_IS(id) && pc_level == 1 && pc_xp == 0 && pc_gold == 0);                 /* a new character */
    CHECK(!AI_IS(id));
    CHECK(strcmp(pc_name, "Rogue") == 0 && pc_class == PC_CLASS_FIGHTER && pc_depth_max == 1);
    CHECK(gen_creature_xp(CR_RAT) == 5 && gen_creature_xp(CR_GOBLIN) == 5 && gen_creature_xp(CR_PLAYER) == 0);
    chest = nge_entity_create();                               /* no kind: no hooks */
    CHECK(GEN_HOOK_EF(gen_hit_of(chest, ATTACK_MELEE)) == EF_NONE);
    TEST_EXPECT_HALT(gen_hit_of(id, ATTACK_USE_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_item_hook(IT_COUNT, GEN_HOOK_USE));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(gen_creature_hit(CR_COUNT, ATTACK_MELEE));
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
        reset();                                               /* a lichen owns its roots too */
        nge_event_init();
        id = gen_creature_random(1, AREA);
        ok = id != NGE_ENTITY_INVALID && creature_ok(id, AREA, kind_index[id]);
        if (ok) {
            seen[kind_index[id]]++;
        }
    }
    CHECK(ok);
    CHECK(seen[CR_PLAYER] == 0 && seen[CR_RAT] > 0 && seen[CR_GOBLIN] > 0);   /* rarity 0: never */
    CHECK(seen[CR_FIGHTER] == 0 && seen[CR_MAGE] == 0);   /* the classes' players (game/chargen): never */
    CHECK((gen_creatures[CR_FIGHTER].flags & CREATURE_PLAYER) && (gen_creatures[CR_MAGE].flags & CREATURE_PLAYER));
    CHECK(gen_creatures[CR_PLAYER].stats.ability[ABIL_STR] == 12 && gen_creatures[CR_PLAYER].stats.ability[ABIL_CHA] == 12);
    CHECK(seen[CR_RAT] > seen[CR_GOBLIN]);                                      /* rarity 10 against 6 */

    /* ---- the creatures' bands: every depth has several kinds, and the weakest leave ---- */
    ok = 1;
    for (i = 1; i <= LEVEL_DEPTH_BOTTOM; i++) {
        int kinds = 0;

        for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
            const gen_where_t *w = &gen_creatures[kind].where;

            kinds += i >= w->depth && i <= w->depth_max && w->rarity != 0;
            ok = ok && w->depth_max <= LEVEL_DEPTH_BOTTOM && (gen_creatures[kind].flags & CREATURE_PLAYER || w->depth >= 1);
        }
        ok = ok && kinds >= 3;
    }
    CHECK(ok);
    CHECK(gen_creatures[CR_RAT].where.depth_max == 2 && gen_creatures[CR_GOBLIN].where.depth_max == 3);   /* narrowed (6.3) */
    memset(seen, 0, sizeof(seen));
    for (i = 0; i < 300; i++) {
        reset();                                               /* a troll owns its regeneration too */
        nge_event_init();                                      /* ...and posts its start */
        id = gen_creature_random(LEVEL_DEPTH_BOTTOM, AREA);
        if (id != NGE_ENTITY_INVALID) {
            seen[kind_index[id]]++;
        }
    }
    reset();
    nge_event_init();
    /* At the bottom, only the kinds whose depths reach it: the deep ones, and not the wraith (6.3) */
    ok = seen[CR_TROLL] > 0 && seen[CR_DRAGON] > 0 && seen[CR_WRAITH] == 0 && seen[CR_RAT] == 0;
    for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
        ok = ok && (seen[kind] == 0 || gen_creatures[kind].where.depth_max == LEVEL_DEPTH_BOTTOM);
    }
    CHECK(ok);

    /*
     * Every depth has 6 to 22 random kinds (docs/creature_extensions.md R1), and
     * from depth 2 one that attacks from afar: a ranged part, a ranged kit or a
     * spell cast at a target (R2)
     */
    ok = 1;
    for (i = 1; i <= LEVEL_DEPTH_BOTTOM; i++) {
        int kinds = 0;
        int far = 0;

        for (kind = CR_NONE + 1; kind < CR_COUNT; kind++) {
            const gen_creature_kind_t *k = &gen_creatures[kind];
            uint8_t j;

            if (k->where.rarity == 0 || i < k->where.depth || i > k->where.depth_max) {
                continue;
            }
            kinds++;
            far = far || k->ranged.dice != 0;
            for (j = 0; j < GEN_KIT_MAX; j++) {
                far = far || (k->kit[j] != IT_NONE && gen_items[k->kit[j]].subclass == ITEM_RANGED);
            }
            for (j = 0; j < GEN_SPELLS_MAX; j++) {
                far = far || (k->spells[j] != SP_NONE && spell_defs[k->spells[j]].ai == SPELL_AI_TARGET);
            }
        }
        ok = ok && kinds >= 6 && kinds <= 22 && (i == 1 || far);
    }
    CHECK(ok);

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
    /*
     * The quest item and keys: never at random, at any depth (docs/winning.md
     * R2, docs/action_unlock.md); keys come only with their locks
     */
    ok = 1;
    for (i = 0; i < 1000 && ok; i++) {
        id = gen_item_random((uint8_t)(1 + i % LEVEL_DEPTH_BOTTOM), AREA);
        ok = id != NGE_ENTITY_INVALID && kind_index[id] != IT_QUEST && gen_items[kind_index[id]].subclass != ITEM_KEY;
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    CHECK(ok && nge_entity_free_count() == NGE_ENTITY_MAX);

    /* ---- kind 0 of each class is none: a blank row, never generated (docs/creature_extensions.md D11) ---- */
    CHECK(gen_creatures[CR_NONE].name == NULL && gen_creatures[CR_NONE].where.rarity == 0);
    CHECK(gen_items[IT_NONE].name == NULL && gen_items[IT_NONE].where.rarity == 0);
    CHECK(gen_features[FT_NONE].name == NULL && gen_features[FT_NONE].where.rarity == 0);
    ok = 1;
    for (i = 0; i < 1000 && ok; i++) {
        lo = 1 + i % LEVEL_DEPTH_BOTTOM;
        reset();                                               /* a lichen owns its roots too */
        nge_event_init();
        id = gen_creature_random((uint8_t)lo, AREA);
        ok = id == NGE_ENTITY_INVALID || kind_index[id] != CR_NONE;
        id = gen_item_random((uint8_t)lo, AREA);
        ok = ok && (id == NGE_ENTITY_INVALID || kind_index[id] != IT_NONE);
        id = gen_feature_random((uint8_t)lo, AREA);
        ok = ok && (id == NGE_ENTITY_INVALID || kind_index[id] != FT_NONE);
    }
    CHECK(ok);
    reset();
    nge_event_init();

    /* ---- the catalogue: every kind well formed, every subclass as its mechanics will need ---- */
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        ok = ok && kind_ok(kind);
    }
    CHECK(ok);
    /* Each equipment slot has a kind that fits */
    ok = 1;
    for (i = 1; i < SLOT_COUNT; i++) {
        int fits = 0;

        for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
            fits = fits || (gen_items[kind].fits & SLOT_BIT(i)) != 0;
        }
        ok = ok && fits;
    }
    CHECK(ok);
    /* Each depth has kinds, and no subclass passes a quarter of the weight (docs/items.md R4) */
    ok = 1;
    for (i = 1; i <= LEVEL_DEPTH_BOTTOM; i++) {
        int share[ITEM_COUNT] = { 0 };
        int total = 0;
        int s;

        for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
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
    /* The spell books: each names its own list (docs/spells.md 10.5.3), and is made as an item */
    CHECK(gen_item_subclass(IT_BOOK_BASIC) == ITEM_BOOK && gen_item_book_spells(IT_BOOK_BASIC) == SPELL_BOOK_BASIC);
    CHECK(gen_item_subclass(IT_BOOK_ADEPT) == ITEM_BOOK && gen_item_book_spells(IT_BOOK_ADEPT) == SPELL_BOOK_ADEPT);
    CHECK(gen_item_subclass(IT_BOOK_MASTER) == ITEM_BOOK && gen_item_book_spells(IT_BOOK_MASTER) == SPELL_BOOK_MASTER);
    ok = 1;
    for (kind = IT_NONE + 1; kind < IT_COUNT; kind++) {
        ok = ok && (gen_items[kind].subclass == ITEM_BOOK) == (kind >= IT_BOOK_BASIC && kind <= IT_BOOK_MASTER);
    }
    CHECK(ok);
    CHECK(gen_item_hook(IT_BOOK_BASIC, GEN_HOOK_USE) == 0 && gen_item_fits(IT_BOOK_BASIC) == 0);
    id = gen_item(IT_BOOK_ADEPT, AREA);
    CHECK(item_ok(id, AREA, IT_BOOK_ADEPT) && !ATTACK_HAS(id, ATTACK_MELEE));
    nge_entity_destroy(id);
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
