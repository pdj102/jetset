/* game/components: the player character component, XP, levels and gold. */
#include "test.h"

#include <stdlib.h>
#include <string.h>

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"

/* The events posted since the last call */
static nge_event_t seen[16];
static int seen_count;

static void record(const nge_event_t *e)
{
    if (seen_count < (int)(sizeof(seen) / sizeof(seen[0]))) {
        seen[seen_count++] = *e;
    }
}

static int events(void)
{
    seen_count = 0;
    nge_event_process(record);
    return seen_count;
}

/* Events of id among those seen */
static int count(uint8_t id)
{
    int n = 0;
    int i;

    for (i = 0; i < seen_count; i++) {
        n += seen[i].id == id;
    }
    return n;
}

/* The index of the first event of id among those seen, or -1 */
static int first(uint8_t id)
{
    int i;

    for (i = 0; i < seen_count; i++) {
        if (seen[i].id == id) {
            return i;
        }
    }
    return -1;
}

/* A new game's player: entities, renderer and clock afresh */
static nge_entity_id_t new_player(void)
{
    nge_entity_id_t id;

    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    id = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    nge_event_flush();   /* its innate effect started */
    return id;
}

int main(void)
{
    nge_entity_id_t p;
    nge_entity_id_t rat;
    uint8_t hp_max;
    uint8_t gain;
    int ok;
    int i;

    test_init();
    srand(7);

    /* ---- a new character, from the generator ---- */
    p = new_player();
    CHECK(PC_IS(p) && pc_level == PC_LEVEL_MIN && pc_xp == 0 && pc_gold == 0 && pc_depth_max == 1);
    CHECK(pc_class == PC_CLASS_FIGHTER && strcmp(pc_name, "Rogue") == 0);
    rat = gen_creature(CR_RAT, NGE_OWNER_WORLD);
    CHECK(!PC_IS(rat));

    /* ---- the name: at most PC_NAME_MAX characters ---- */
    pc_set_name(p, "bo");
    CHECK(strcmp(pc_name, "bo") == 0);
    pc_set_name(p, "Bartholomew");
    CHECK(strcmp(pc_name, "Bartho") == 0);

    /* ---- the level table ---- */
    CHECK(pc_xp_needed(1) == 0 && pc_xp_needed(2) == 20 && pc_xp_needed(PC_LEVEL_MAX) == 3000);
    ok = 1;
    for (i = PC_LEVEL_MIN; i < PC_LEVEL_MAX; i++) {
        ok &= pc_xp_needed((uint8_t)i) < pc_xp_needed((uint8_t)(i + 1));
    }
    CHECK(ok);

    /* ---- XP below the next level: posted, no level ---- */
    pc_gain_xp(p, 19);
    CHECK(pc_xp == 19 && pc_level == 1);
    CHECK(events() == 1 && seen[0].id == EV_XP_GAINED && seen[0].target == p && seen[0].p1 == 19 &&
          seen[0].p2 == 0);
    pc_gain_xp(p, 0);                                  /* nothing: nothing posted */
    CHECK(pc_xp == 19 && events() == 0);

    /* ---- exactly the figure: a level, more hit points (1d8 + Con 14's +2), the XP first ---- */
    hp_max = health_hp_max[p];
    CHECK(health_hp[p] == hp_max);
    pc_gain_xp(p, 1);
    CHECK(pc_xp == 20 && pc_level == 2);
    gain = (uint8_t)(health_hp_max[p] - hp_max);
    CHECK(gain >= 3 && gain <= 10 && health_hp_max_base[p] == health_hp_max[p]);
    CHECK(health_hp[p] == health_hp_max[p]);           /* the current hit points up by the same */
    events();
    CHECK(first(EV_XP_GAINED) == 0 && count(EV_LEVEL_GAINED) == 1 && count(EV_HEALED) == 1);
    CHECK(seen[seen_count - 1].id == EV_LEVEL_GAINED && seen[seen_count - 1].p1 == 2);

    /* ---- hurt: a level heals the gain, not to full ---- */
    health_damage(p, NGE_ENTITY_INVALID, 10, DMG_BLUNT);
    hp_max = health_hp_max[p];
    pc_gain_xp(p, 20);                                 /* 40: level 3 */
    gain = (uint8_t)(health_hp_max[p] - hp_max);
    CHECK(pc_level == 3 && health_hp[p] == (uint8_t)(hp_max - 10 + gain));
    events();

    /* ---- one gain across three figures: three levels, three events, in order ---- */
    pc_gain_xp(p, 280);                                /* 320: level 6 */
    CHECK(pc_xp == 320 && pc_level == 6);
    events();
    CHECK(count(EV_LEVEL_GAINED) == 3 && first(EV_XP_GAINED) == 0);
    ok = 1;
    for (i = 0, gain = 4; i < seen_count; i++) {
        if (seen[i].id == EV_LEVEL_GAINED) {
            ok &= seen[i].p1 == gain++;
        }
    }
    CHECK(ok && gain == 7);

    /* ---- more than 255 XP: the amount in p1 and p2 ---- */
    pc_gain_xp(p, 300);
    events();
    CHECK(seen[0].id == EV_XP_GAINED && seen[0].p1 == (300 & 0xFF) && seen[0].p2 == (300 >> 8));

    /* ---- at the top level the XP still rise; at PC_XP_MAX they stop ---- */
    pc_gain_xp(p, 5000);
    CHECK(pc_level == PC_LEVEL_MAX && pc_xp == 5620);
    events();
    CHECK(count(EV_LEVEL_GAINED) == 4);
    pc_gain_xp(p, 100);
    CHECK(pc_level == PC_LEVEL_MAX && pc_xp == 5720 && events() == 1);
    pc_gain_xp(p, 65000);
    CHECK(pc_xp == PC_XP_MAX && events() == 1 && seen[0].p1 == ((PC_XP_MAX - 5720) & 0xFF) &&
          seen[0].p2 == ((PC_XP_MAX - 5720) >> 8));
    pc_gain_xp(p, 1);                                  /* full: nothing posted */
    CHECK(pc_xp == PC_XP_MAX && events() == 0);

    /* ---- the hit point gain: at least 1 at Con 1; the maximum stops at 255 ---- */
    p = new_player();
    effects_set_base(p, ATTR_CON, 1);                  /* modifier -5 */
    ok = 1;
    for (i = PC_LEVEL_MIN; i < PC_LEVEL_MAX; i++) {
        hp_max = health_hp_max[p];
        pc_gain_xp(p, (uint16_t)(pc_xp_needed((uint8_t)(i + 1)) - pc_xp));
        gain = (uint8_t)(health_hp_max[p] - hp_max);
        ok &= pc_level == i + 1 && gain >= 1 && gain <= 3;
        events();
    }
    CHECK(ok);
    p = new_player();
    effects_set_base(p, ATTR_HP_MAX, 254);
    pc_gain_xp(p, 20);
    CHECK(pc_level == 2 && health_hp_max_base[p] == 255 && health_hp_max[p] == 255);
    events();

    /* ---- gold: posted, then as many XP; spending leaves the XP ---- */
    p = new_player();
    pc_gain_gold(p, 15);
    CHECK(pc_gold == 15 && pc_xp == 15 && pc_level == 1);
    CHECK(events() == 2 && seen[0].id == EV_GOLD_CHANGED && seen[0].target == p && seen[1].id == EV_XP_GAINED &&
          seen[1].p1 == 15);
    CHECK(PC_HAS_GOLD(15) && !PC_HAS_GOLD(16));
    pc_gain_gold(p, 5);                                /* 20: crosses the figure, a level */
    CHECK(pc_gold == 20 && pc_xp == 20 && pc_level == 2);
    events();
    CHECK(first(EV_GOLD_CHANGED) == 0 && first(EV_XP_GAINED) == 1 && count(EV_LEVEL_GAINED) == 1);
    pc_spend_gold(p, 8);
    CHECK(pc_gold == 12 && pc_xp == 20 && events() == 1 && seen[0].id == EV_GOLD_CHANGED);
    pc_spend_gold(p, 0);                               /* nothing: nothing posted */
    CHECK(pc_gold == 12 && events() == 0);
    pc_spend_gold(p, 12);
    CHECK(pc_gold == 0 && events() == 1);
    pc_gain_gold(p, 0);
    CHECK(pc_gold == 0 && events() == 0);
    for (i = PC_LEVEL_MIN; i < PC_LEVEL_MAX; i++) {    /* a level at a time: the event queue holds 16 */
        pc_gain_xp(p, (uint16_t)(pc_xp_needed((uint8_t)(i + 1)) - pc_xp));
        events();
    }
    CHECK(pc_level == PC_LEVEL_MAX && pc_xp == 3000);
    pc_gain_gold(p, 65000);                            /* the XP stop at PC_XP_MAX */
    CHECK(pc_gold == 65000 && pc_xp == PC_XP_MAX && events() == 2);
    pc_gain_gold(p, 1000);                             /* only 535 more fit */
    CHECK(pc_gold == PC_GOLD_MAX && events() == 1 && seen[0].id == EV_GOLD_CHANGED);
    pc_gain_gold(p, 1);                                /* full: nothing */
    CHECK(pc_gold == PC_GOLD_MAX && events() == 0);

    /* ---- the deepest depth: the larger kept ---- */
    pc_reach_depth(p, 3);
    pc_reach_depth(p, 2);
    CHECK(pc_depth_max == 3);

    /* ---- asserts: spending too much, not the player ---- */
    pc_spend_gold(p, PC_GOLD_MAX);
    events();
    TEST_EXPECT_HALT(pc_spend_gold(p, 1));
    CHECK(test_halt_count == 1 && pc_gold == 0);       /* halted before any change */
    rat = gen_creature(CR_RAT, NGE_OWNER_WORLD);
    TEST_EXPECT_HALT(pc_gain_xp(rat, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(pc_gain_gold(rat, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(pc_xp_needed(PC_LEVEL_MAX + 1));
    CHECK(test_halt_count == 1);

    return test_summary("pc");
}
