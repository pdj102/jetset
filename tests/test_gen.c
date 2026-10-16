/* game/gen: the entity generators, every kind against its base record. */
#include "test.h"

#include <stdlib.h>
#include <string.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"

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

static int creature_ok(nge_entity_id_t id, nge_owner_t owner, uint8_t kind)
{
    const gen_creature_base_t *b = &gen_creatures[kind];
    unsigned int lo = b->stats.hp_bonus + b->stats.hp_dice;
    unsigned int hi = b->stats.hp_bonus + b->stats.hp_dice * b->stats.hp_sides;

    if (!made(id, owner, CLASS_CREATURE, kind) || !render_is(id, &b->render)) {
        return 0;
    }
    if (!NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_BLOCK | COMPONENT_STATS | NGE_COMPONENT_TIMER) ||
        nge_block_of[id] != b->block.block || nge_block_pass[id] != b->block.pass) {
        return 0;
    }
    if (stats_hp[id] < lo || stats_hp[id] > hi || stats_hp_max[id] != stats_hp[id] ||
        stats_speed[id] != b->stats.speed) {
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
           !NGE_ENTITY_HAS_ANY_COMPONENTS(id, NGE_COMPONENT_BLOCK | COMPONENT_STATS | NGE_COMPONENT_TIMER);
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
    int hi;
    uint8_t kind;

    test_init();
    srand(3);

    CHECK(COMPONENT_KIND == NGE_COMPONENT(NGE_COMPONENT_USER + 0));
    CHECK(COMPONENT_STATS == NGE_COMPONENT(NGE_COMPONENT_USER + 1));

    /* ---- every kind: its components match its base record ---- */
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
        ok = ok && item_ok(id, AREA, kind);
    }
    CHECK(ok);

    /* Any owner: an item in a chest, a creature not yet placed */
    chest = gen_item(IT_SWORD, AREA);
    id = gen_item(IT_POTION, chest);
    CHECK(item_ok(id, chest, IT_POTION));
    id = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    CHECK(creature_ok(id, NGE_OWNER_WORLD, CR_PLAYER));

    /* Hit points are rolled across the whole range: a rat has 1d4 */
    reset();
    lo = 255;
    hi = 0;
    for (i = 0; i < 200; i++) {
        id = gen_creature(CR_RAT, AREA);
        lo = stats_hp[id] < lo ? stats_hp[id] : lo;
        hi = stats_hp[id] > hi ? stats_hp[id] : hi;
        nge_entity_destroy(id);
        nge_entity_release(nge_entity_next_dying());
    }
    CHECK(lo == 1 && hi == 4);

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
    CHECK(seen[IT_POTION] + seen[IT_SWORD] == 300 && seen[IT_POTION] > seen[IT_SWORD] && seen[IT_SWORD] > 0);
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
    CHECK(gen_pick(where, 3, 0) == 0xFF);
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
