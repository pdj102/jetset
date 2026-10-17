/* game/components: the health component, damage, healing and death. */
#include "test.h"

#include <stdlib.h>

#include "ecs/nge_entity.h"
#include "event/nge_event.h"

#include "game/components/health.h"
#include "game/events.h"

/* The events posted since the last call */
static nge_event_t seen[16];
static int seen_count;

static void record(const nge_event_t *e)
{
    seen[seen_count++] = *e;
}

static int events(void)
{
    seen_count = 0;
    nge_event_process(record);
    return seen_count;
}

static int is(int i, uint8_t target, uint8_t id, uint8_t source, uint8_t p1, uint8_t p2)
{
    return i < seen_count && seen[i].target == target && seen[i].id == id && seen[i].source == source &&
           seen[i].p1 == p1 && seen[i].p2 == p2;
}

/* An entity with health: hp hit points, these masks */
static nge_entity_id_t make(uint8_t hp, uint8_t immune, uint8_t resist, uint8_t vuln)
{
    health_base_t b = { 0, 0, 0, 0, 0, 0 };
    nge_entity_id_t id = nge_entity_create();

    b.hp_bonus = hp;
    b.immune = immune;
    b.resist = resist;
    b.vuln = vuln;
    health_add(id, &b);
    return id;
}

/* The reference: what the rules say an entity takes */
static unsigned int expected(unsigned int amount, int immune, int resist, int vuln, uint8_t *outcome)
{
    if (immune) {
        *outcome = DMG_IMMUNE;
        return 0;
    }
    if (resist && !vuln) {
        *outcome = DMG_RESISTED;
        return (amount + 1) / 2;
    }
    if (vuln && !resist) {
        *outcome = DMG_VULNERABLE;
        return amount * 2 > 255 ? 255 : amount * 2;
    }
    *outcome = DMG_NORMAL;
    return amount;
}

int main(void)
{
    static const uint8_t amounts[] = { 0, 1, 2, 3, 127, 128, 200, 255 };
    health_base_t b;
    nge_entity_id_t a;
    nge_entity_id_t player;
    nge_entity_id_t attacker;
    uint8_t outcome;
    uint8_t want_outcome;
    unsigned int m;
    unsigned int i;
    int lo;
    int hi;
    int ok;

    test_init();

    CHECK(COMPONENT_HEALTH == NGE_COMPONENT(NGE_COMPONENT_USER + 4));
    CHECK(EV_DMG_PACK(DMG_RESISTED, DMG_FIRE) == 0x13);
    CHECK(EV_DMG_OUTCOME(0x13) == DMG_RESISTED && EV_DMG_TYPE(0x13) == DMG_FIRE);
    CHECK(DMG_BIT(DMG_MAGIC) == 0x80);

    nge_entity_init();
    nge_event_init();

    /* ---- add: rolled from the base, unhurt, the masks ---- */
    b.hp_dice = 2;
    b.hp_sides = 6;
    b.hp_bonus = 3;
    b.immune = 0x01;
    b.resist = 0x02;
    b.vuln = 0x04;
    srand(5);
    lo = 255;
    hi = 0;
    for (i = 0; i < 300; i++) {
        a = nge_entity_create();
        health_add(a, &b);
        lo = health_hp[a] < lo ? health_hp[a] : lo;
        hi = health_hp[a] > hi ? health_hp[a] : hi;
        ok = health_hp_max[a] == health_hp[a] && health_immune[a] == 0x01 && health_resist[a] == 0x02 &&
             health_vuln[a] == 0x04 && NGE_ENTITY_HAS_COMPONENTS(a, COMPONENT_HEALTH);
        nge_entity_destroy(a);
        nge_entity_release(nge_entity_next_dying());
        if (!ok) {
            break;
        }
    }
    CHECK(ok && lo == 5 && hi == 15);                         /* 2d6 + 3 */
    b.hp_dice = 1;                                            /* 1d10 + 250: past 255, capped */
    b.hp_sides = 10;
    b.hp_bonus = 250;
    a = nge_entity_create();
    ok = 1;
    for (i = 0; i < 50 && ok; i++) {
        health_add(a, &b);
        ok = health_hp[a] >= 251;                             /* never wrapped round */
    }
    CHECK(ok);
    b.hp_dice = 0;
    b.hp_bonus = 0;                                           /* nothing: at least 1 */
    health_add(a, &b);
    CHECK(health_hp[a] == 1 && health_hp_max[a] == 1);
    CHECK(HEALTH_BASE_HAS(&b) == 0);
    b.hp_bonus = 1;
    CHECK(HEALTH_BASE_HAS(&b) == 1);
    nge_entity_init();

    /* ---- dealt: every amount against every combination of masks ---- */
    ok = 1;
    for (m = 0; m < 8 && ok; m++) {
        int imm = (m & 1) != 0;
        int res = (m & 2) != 0;
        int vul = (m & 4) != 0;

        a = make(100, imm ? DMG_BIT(DMG_FIRE) : 0, res ? DMG_BIT(DMG_FIRE) : 0, vul ? DMG_BIT(DMG_FIRE) : 0);
        for (i = 0; i < sizeof(amounts) && ok; i++) {
            unsigned int want = expected(amounts[i], imm, res, vul, &want_outcome);

            outcome = 0x77;
            ok = health_dealt(a, amounts[i], DMG_FIRE, &outcome) == want && outcome == want_outcome;
            /* Another type: no mask applies */
            ok = ok && health_dealt(a, amounts[i], DMG_COLD, &outcome) == amounts[i] && outcome == DMG_NORMAL;
        }
        ok = ok && health_hp[a] == 100;                       /* dealt changes nothing */
    }
    CHECK(ok);
    CHECK(health_dealt(a, 9, DMG_FIRE, NULL) == 0);           /* the outcome may be left out */

    /* ---- damage: hp down, the event, never below 0, death once ---- */
    nge_entity_init();
    nge_event_init();
    attacker = nge_entity_create();
    a = make(10, DMG_BIT(DMG_POISON), DMG_BIT(DMG_FIRE), DMG_BIT(DMG_COLD));
    CHECK(health_damage(a, attacker, 3, DMG_SLASH) == 3 && health_hp[a] == 7);
    CHECK(events() == 1 && is(0, a, EV_DAMAGED, attacker, 3, EV_DMG_PACK(DMG_NORMAL, DMG_SLASH)));
    CHECK(health_damage(a, attacker, 3, DMG_FIRE) == 2 && health_hp[a] == 5);   /* resisted, rounded up */
    CHECK(events() == 1 && is(0, a, EV_DAMAGED, attacker, 2, EV_DMG_PACK(DMG_RESISTED, DMG_FIRE)));
    /* Immune: 0 taken, the event all the same */
    CHECK(health_damage(a, NGE_ENTITY_INVALID, 50, DMG_POISON) == 0 && health_hp[a] == 5);
    CHECK(events() == 1 && is(0, a, EV_DAMAGED, NGE_EVENT_NONE, 0, EV_DMG_PACK(DMG_IMMUNE, DMG_POISON)));
    /* Vulnerable, more than is left: 0, not below; then death */
    CHECK(health_damage(a, attacker, 4, DMG_COLD) == 8 && health_hp[a] == 0);
    CHECK(events() == 2 && is(0, a, EV_DAMAGED, attacker, 8, EV_DMG_PACK(DMG_VULNERABLE, DMG_COLD)) &&
          is(1, a, EV_DIED, attacker, 0, 0));
    CHECK(NGE_ENTITY_STATE(a) == NGE_ENTITY_DYING);
    /* Dead: no more damage, no events, no healing */
    CHECK(health_damage(a, attacker, 5, DMG_SLASH) == 0 && health_heal(a, 5) == 0);
    CHECK(health_hp[a] == 0 && events() == 0);
    /* 0 damage of a normal type: still an event, no death */
    a = make(4, 0, 0, 0);
    CHECK(health_damage(a, attacker, 0, DMG_BLUNT) == 0 && health_hp[a] == 4);
    CHECK(events() == 1 && is(0, a, EV_DAMAGED, attacker, 0, EV_DMG_PACK(DMG_NORMAL, DMG_BLUNT)));
    /* Exactly the hp left: dead */
    CHECK(health_damage(a, attacker, 4, DMG_BLUNT) == 4 && health_hp[a] == 0);
    CHECK(events() == 2 && seen[1].id == EV_DIED);

    /* ---- a persistent entity (the player) isn't destroyed at 0 ---- */
    player = make(5, 0, 0, 0);
    nge_entity_set_flags(player, NGE_ENTITY_PERSISTENT);
    CHECK(health_damage(player, NGE_ENTITY_INVALID, 9, DMG_ACID) == 9 && health_hp[player] == 0);
    CHECK(events() == 2 && is(1, player, EV_DIED, NGE_EVENT_NONE, 0, 0));
    CHECK(NGE_ENTITY_IS_ALIVE(player));
    CHECK(health_damage(player, NGE_ENTITY_INVALID, 9, DMG_ACID) == 0 && events() == 0);   /* died once */

    /* ---- heal: up to the maximum, the hp restored in the event ---- */
    a = make(10, 0, 0, 0);
    health_damage(a, NGE_ENTITY_INVALID, 6, DMG_BLUNT);
    events();
    CHECK(health_heal(a, 2) == 2 && health_hp[a] == 6);
    CHECK(events() == 1 && is(0, a, EV_HEALED, NGE_EVENT_NONE, 2, 0));
    CHECK(health_heal(a, 200) == 4 && health_hp[a] == 10);   /* only what was missing */
    CHECK(events() == 1 && seen[0].p1 == 4);
    CHECK(health_heal(a, 3) == 0 && events() == 0);           /* unhurt: nothing, no event */

    /* ---- set max and flags ---- */
    health_set_max(a, 4);
    CHECK(health_hp_max[a] == 4 && health_hp[a] == 4);        /* hp kept within it */
    health_set_max(a, 20);
    CHECK(health_hp_max[a] == 20 && health_hp[a] == 4);       /* raising it doesn't heal */
    health_set_flags(a, DMG_BIT(DMG_MAGIC), 0, 0);
    CHECK(health_dealt(a, 7, DMG_MAGIC, &outcome) == 0 && outcome == DMG_IMMUNE);

    /* ---- asserts ---- */
    a = nge_entity_create();                                  /* no health */
    TEST_EXPECT_HALT(health_damage(a, NGE_ENTITY_INVALID, 1, DMG_FIRE));
    CHECK(test_halt_count == 1);
    a = make(5, 0, 0, 0);
    TEST_EXPECT_HALT(health_damage(a, NGE_ENTITY_INVALID, 1, DMG_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(health_set_max(a, 0));
    CHECK(test_halt_count == 1);
    b.hp_dice = 1;
    b.hp_sides = 0;
    TEST_EXPECT_HALT(health_add(a, &b));
    CHECK(test_halt_count == 1);

    return test_summary("health");
}
