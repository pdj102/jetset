/* nge_effect.c: creating and destroying active effects, and finding them. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_effect.h"
#include "ecs/systems/nge_timer.h"

#define AREA  NGE_OWNER_AREA(0)

#define K_EFFECT  4    /* timer kinds: the game's */
#define K_OTHER   9

/* What the dispatch saw */
static int dispatched;
static nge_entity_id_t last_id;
static uint8_t last_kind;
static uint8_t last_cycles;

static void dispatch(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    dispatched++;
    last_id = id;
    last_kind = kind;
    last_cycles = cycles;
}

/* target's active effects */
static int count(nge_entity_id_t target)
{
    nge_entity_id_t e;
    uint8_t i;
    int n = 0;

    NGE_EFFECT_FOR_EACH(i, e, target) {
        n++;
    }
    return n;
}

/* Every entity free, the clock at 0; a target placed in an area */
static nge_entity_id_t reset(void)
{
    nge_entity_id_t t;

    nge_entity_init();
    nge_timer_init();
    dispatched = 0;
    t = nge_entity_create();
    nge_owner_set(t, AREA);
    return t;
}

int main(void)
{
    nge_entity_id_t t;
    nge_entity_id_t t2;
    nge_entity_id_t e;
    nge_entity_id_t e2;
    nge_entity_id_t other;
    uint8_t i;

    test_init();

    CHECK(NGE_COMPONENT_EFFECT == NGE_COMPONENT(4) && NGE_COMPONENT_USER == 5);

    /* ---- creating: owned by the target, tagged, with a timer and its payload ---- */
    t = reset();
    e = nge_effect_create(t, K_EFFECT, 3, 4, 7, (uint8_t)-2);
    CHECK(e != NGE_ENTITY_INVALID && NGE_ENTITY_IS_ALIVE(e) && NGE_OWNER_OF(e) == t);
    CHECK(NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_EFFECT) && NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_TIMER));
    CHECK(nge_timer_kind[e] == K_EFFECT && nge_timer_period[e] == 3 && nge_timer_count[e] == 3 &&
          nge_timer_cycles[e] == 4);
    CHECK(nge_timer_payload[0][e] == 7 && (int8_t)nge_timer_payload[1][e] == -2);
    CHECK(count(t) == 1);

    /* ---- firing: the game's dispatch, with its kind ---- */
    CHECK(nge_timer_advance(dispatch) == 3 && dispatched == 1 && last_id == e && last_kind == K_EFFECT &&
          last_cycles == 4);
    /* A restart keeps the payload (a refresh) */
    nge_timer_add(e, K_EFFECT, 5, 2);
    CHECK(nge_timer_payload[0][e] == 7 && (int8_t)nge_timer_payload[1][e] == -2 && nge_timer_cycles[e] == 2);

    /* ---- the iterator: only the target's, only active effects, only alive ones ---- */
    t = reset();
    t2 = nge_entity_create();
    nge_owner_set(t2, AREA);
    e = nge_effect_create(t, K_EFFECT, 5, 0, 1, 1);
    e2 = nge_effect_create(t, K_OTHER, 5, 0, 2, 1);   /* any kind */
    other = nge_effect_create(t2, K_EFFECT, 5, 0, 1, 1);
    CHECK(e != e2 && count(t) == 2 && count(t2) == 1);
    /* An owned entity with a timer but no tag isn't an active effect */
    other = nge_entity_create();
    nge_owner_set(other, t);
    nge_timer_add(other, K_EFFECT, 5, 0);
    CHECK(count(t) == 2);
    /* One whose timer is gone still is */
    nge_timer_remove(e);
    CHECK(count(t) == 2);

    /* ---- destroying: dying, not found, not fired; owner and payload readable ---- */
    t = reset();
    e = nge_effect_create(t, K_EFFECT, 2, 0, 5, 6);
    e2 = nge_effect_create(t, K_EFFECT, 2, 0, 8, 9);
    nge_effect_destroy(e);
    CHECK(NGE_ENTITY_STATE(e) == NGE_ENTITY_DYING && count(t) == 1);
    CHECK(NGE_OWNER_OF(e) == t && nge_timer_payload[0][e] == 5 && nge_timer_payload[1][e] == 6);
    CHECK(nge_timer_advance(dispatch) == 2 && dispatched == 1 && last_id == e2);
    /* Destroying each in the iterator */
    t = reset();
    nge_effect_create(t, K_EFFECT, 5, 0, 1, 1);
    nge_effect_create(t, K_EFFECT, 5, 0, 2, 1);
    nge_effect_create(t, K_EFFECT, 5, 0, 1, 1);
    NGE_EFFECT_FOR_EACH(i, e, t) {
        if (nge_timer_payload[0][e] == 1) {
            nge_effect_destroy(e);
        }
    }
    CHECK(count(t) == 1);
    /* Destroyed in a firing (the game's handler), on its last cycle */
    t = reset();
    e = nge_effect_create(t, K_EFFECT, 3, 1, 1, 1);
    CHECK(nge_timer_advance(dispatch) == 3 && last_cycles == 1);
    nge_effect_destroy(e);
    CHECK(count(t) == 0 && nge_timer_advance(dispatch) == 0);

    /* ---- the target going: its effects with it ---- */
    t = reset();
    e = nge_effect_create(t, K_EFFECT, 3, 0, 1, 1);
    nge_entity_destroy(t);
    CHECK(NGE_ENTITY_IS_ALIVE(e) && count(t) == 1);   /* still its: the game's clean-up destroys what it owned */
    NGE_EFFECT_FOR_EACH(i, e, t) {
        nge_effect_destroy(e);                         /* a dying target's effects can be destroyed */
    }
    CHECK(count(t) == 0);

    /* ---- no entity free: refused, nothing created ---- */
    t = reset();
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(nge_effect_create(t, K_EFFECT, 3, 0, 1, 1) == NGE_ENTITY_INVALID && count(t) == 0);

    /* ---- asserts ---- */
    t = reset();
    e = nge_effect_create(t, K_EFFECT, 3, 0, 1, 1);
    TEST_EXPECT_HALT(nge_effect_create(t, K_EFFECT, 0, 0, 1, 1));            /* period 0 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_create(90, K_EFFECT, 3, 0, 1, 1));           /* a free target */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_create(NGE_ENTITY_INVALID, K_EFFECT, 3, 0, 1, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_destroy(t));                                 /* not an effect */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_destroy(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);
    nge_effect_destroy(e);
    TEST_EXPECT_HALT(nge_effect_destroy(e));                                 /* dying already */
    CHECK(test_halt_count == 1);
    nge_entity_destroy(t);
    TEST_EXPECT_HALT(nge_effect_create(t, K_EFFECT, 3, 0, 1, 1));            /* a dying target */
    CHECK(test_halt_count == 1);
    /* An effect owned by no entity (moved to an area) */
    t = reset();
    e = nge_effect_create(t, K_EFFECT, 3, 0, 1, 1);
    nge_owner_set(e, AREA);
    TEST_EXPECT_HALT(nge_effect_destroy(e));
    CHECK(test_halt_count == 1);

    return test_summary("nge_effect");
}
