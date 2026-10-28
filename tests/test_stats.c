/* game/components: the stats component, speed and the six abilities. */
#include "test.h"

#include "ecs/nge_entity.h"

#include "game/components/attrs.h"
#include "game/components/stats.h"

/* The reference: (score - 10) / 2 rounded down */
static int modifier(int score)
{
    int d = score - 10;

    return (d >= 0) ? d / 2 : -((-d + 1) / 2);
}

int main(void)
{
    stats_kind_t b = { 10, { 16, 14, 13, 9, 1, 30 } };
    nge_entity_id_t id;
    int s;
    int ok;
    uint8_t a;

    test_init();

    CHECK(ABIL_COUNT == 6 && ATTR_ABILITY(ABIL_STR) == ATTR_STR && ATTR_ABILITY(ABIL_CHA) == ATTR_CHA &&
          ATTR_CHA + 1 == ATTR_COUNT);

    /* ---- modifiers: every score ---- */
    ok = 1;
    for (s = STATS_ABILITY_MIN; s <= STATS_ABILITY_MAX; s++) {
        ok = ok && STATS_MOD(s) == modifier(s);
    }
    CHECK(ok);
    CHECK(STATS_MOD(1) == -5 && STATS_MOD(9) == -1 && STATS_MOD(10) == 0 && STATS_MOD(11) == 0 &&
          STATS_MOD(12) == 1 && STATS_MOD(30) == 10);

    /* ---- add: speed and abilities, as values and as bases; no statuses ---- */
    nge_entity_init();
    id = nge_entity_create();
    stats_status[id] = 0xFF;                                  /* left by an earlier entity */
    stats_add(id, &b);
    CHECK(NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS) && stats_status[id] == 0);
    CHECK(stats_speed[id] == 10 && stats_speed_base[id] == 10);
    ok = 1;
    for (a = 0; a < ABIL_COUNT; a++) {
        ok = ok && stats_ability[a][id] == b.ability[a] && stats_ability_base[a][id] == b.ability[a];
    }
    CHECK(ok);
    CHECK(stats_ability[ABIL_WIS][id] == 1 && stats_ability[ABIL_CHA][id] == 30);   /* the limits are allowed */
    stats_remove(id);
    CHECK(!NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS));

    /* ---- asserts: scores outside 1-30, speed 0 ---- */
    b.ability[ABIL_DEX] = 0;
    TEST_EXPECT_HALT(stats_add(id, &b));
    CHECK(test_halt_count == 1);
    b.ability[ABIL_DEX] = 31;
    TEST_EXPECT_HALT(stats_add(id, &b));
    CHECK(test_halt_count == 1);
    b.ability[ABIL_DEX] = 14;
    b.speed = 0;
    TEST_EXPECT_HALT(stats_add(id, &b));
    CHECK(test_halt_count == 1);

    return test_summary("stats");
}
