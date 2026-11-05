/* nge_effect.c, nge_effect_timer.c: active effects' lifecycle, its hooks, and firing through nge_timer. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_effect.h"
#include "ecs/systems/nge_effect_internal.h"
#include "ecs/systems/nge_timer.h"

#define AREA  NGE_OWNER_AREA(0)

#define K_POISON  3    /* kinds: any number */
#define K_REGEN   200

/* The game's reasons, from 1 */
#define END_CURED  1
#define END_SAVED  2

/* What the hook saw */
static nge_effect_hook_t seen[64];
static int seen_count;

/* What the hook does at a TICK: 0 nothing, or end it (a saving throw), or destroy the target */
#define ON_TICK_NOTHING  0
#define ON_TICK_SAVE     1
#define ON_TICK_KILL     2
static uint8_t on_tick;

/* A STARTED hook that starts another effect on the same target (re-entry) */
static uint8_t chain;

static void game_hook(const nge_effect_hook_t *h)
{
    if (seen_count < 64) {
        seen[seen_count++] = *h;
    }
    if (h->what == NGE_EFFECT_TICK) {
        if (on_tick == ON_TICK_SAVE) {
            nge_effect_end(h->effect, END_SAVED);
        } else if (on_tick == ON_TICK_KILL) {
            nge_entity_destroy(h->target);
        }
    }
    if (h->what == NGE_EFFECT_STARTED && chain && h->kind == K_POISON) {
        chain = 0;
        nge_effect_start(K_REGEN, 1, h->target, 5, 0, NGE_EFFECT_STACK);
    }
}

/* The game's dispatch: it must never see the effects' firings */
static int dispatched;

static void dispatch(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    (void)id;
    (void)kind;
    (void)cycles;
    dispatched++;
}

static int is(int i, uint8_t what, nge_entity_id_t effect, nge_entity_id_t target, uint8_t kind, int8_t power,
              uint8_t arg)
{
    return i < seen_count && seen[i].what == what && seen[i].effect == effect && seen[i].target == target &&
           seen[i].kind == kind && seen[i].power == power && seen[i].arg == arg;
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

static nge_entity_id_t reset(void)
{
    nge_entity_id_t t;

    nge_entity_init();
    nge_timer_init();
    nge_effect_init(game_hook);
    seen_count = 0;
    dispatched = 0;
    on_tick = ON_TICK_NOTHING;
    chain = 0;
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
    int n;

    test_init();

    CHECK(NGE_COMPONENT_EFFECT == NGE_COMPONENT(4) && NGE_COMPONENT_USER == 5);

    /* ---- init: the hook, and the timer handler for NGE_TIMER_EFFECT ---- */
    t = reset();
    CHECK(nge_effect_game_hook == game_hook && nge_timer_handler[NGE_TIMER_EFFECT] == nge_effect_timer);

    /* ---- starting: an entity owned by the target, with the component and a timer ---- */
    e = nge_effect_start(K_POISON, 2, t, 3, 4, 0);
    CHECK(e != NGE_ENTITY_INVALID && NGE_ENTITY_IS_ALIVE(e) && NGE_OWNER_OF(e) == t);
    CHECK(NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_EFFECT) && NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_TIMER));
    CHECK(nge_timer_kind[e] == NGE_TIMER_EFFECT && nge_timer_period[e] == 3 && nge_timer_cycles[e] == 4);
    CHECK(nge_effect_kind(e) == K_POISON && nge_effect_power(e) == 2);
    CHECK(seen_count == 1 && is(0, NGE_EFFECT_STARTED, e, t, K_POISON, 2, 0));
    CHECK(count(t) == 1);

    /* ---- refreshing: the timer again, the higher power kept ---- */
    nge_timer_advance(dispatch);                                   /* a firing: 3 cycles left */
    CHECK(nge_timer_cycles[e] == 3);
    seen_count = 0;
    CHECK(nge_effect_start(K_POISON, 1, t, 3, 4, 0) == e);         /* weaker */
    CHECK(nge_timer_cycles[e] == 4 && nge_timer_count[e] == 3 && nge_effect_power(e) == 2);
    CHECK(seen_count == 1 && is(0, NGE_EFFECT_REFRESHED, e, t, K_POISON, 2, 0));
    CHECK(nge_effect_start(K_POISON, 5, t, 3, 4, 0) == e);         /* stronger */
    CHECK(nge_effect_power(e) == 5 && is(1, NGE_EFFECT_REFRESHED, e, t, K_POISON, 5, 1));
    CHECK(count(t) == 1);
    /* Negative powers: higher is higher */
    t2 = nge_entity_create();
    nge_owner_set(t2, AREA);
    e2 = nge_effect_start(K_POISON, -3, t2, 3, 4, 0);
    CHECK(nge_effect_start(K_POISON, -1, t2, 3, 4, 0) == e2 && nge_effect_power(e2) == -1);

    /* ---- stacking: another instance each time ---- */
    t = reset();
    e = nge_effect_start(K_REGEN, 1, t, 5, 0, NGE_EFFECT_STACK);
    e2 = nge_effect_start(K_REGEN, 1, t, 5, 0, NGE_EFFECT_STACK);
    CHECK(e != e2 && count(t) == 2 && is(1, NGE_EFFECT_STARTED, e2, t, K_REGEN, 1, 0));
    /* Kinds apart: one of another kind isn't refreshed */
    other = nge_effect_start(K_POISON, 1, t, 3, 1, 0);
    CHECK(other != e && other != e2 && count(t) == 3);

    /* ---- firing: through nge_timer, never the game's dispatch ---- */
    t = reset();
    e = nge_effect_start(K_POISON, 1, t, 3, 2, 0);
    seen_count = 0;
    CHECK(nge_timer_advance(dispatch) == 3 && dispatched == 0);
    CHECK(seen_count == 1 && is(0, NGE_EFFECT_TICK, e, t, K_POISON, 1, 2));
    /* The last cycle: TICK with 1, then it expires */
    CHECK(nge_timer_advance(dispatch) == 3 && dispatched == 0);
    CHECK(seen_count == 3 && is(1, NGE_EFFECT_TICK, e, t, K_POISON, 1, 1) &&
          is(2, NGE_EFFECT_ENDED, e, t, K_POISON, 1, NGE_EFFECT_END_EXPIRED));
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && count(t) == 0);
    /* For ever: no expiry */
    t = reset();
    e = nge_effect_start(K_REGEN, 1, t, 2, 0, 0);
    for (n = 0; n < 20; n++) {
        nge_timer_advance(dispatch);
    }
    CHECK(NGE_ENTITY_IS_ALIVE(e) && seen_count == 21 && is(20, NGE_EFFECT_TICK, e, t, K_REGEN, 1, 0));
    /* The game's own timers still go to its dispatch */
    nge_timer_add(t, NGE_TIMER_KIND_USER, 1, 1);
    nge_timer_advance(dispatch);
    CHECK(dispatched == 1);

    /* ---- ending in a hook: a saving throw at its first firing ---- */
    t = reset();
    e = nge_effect_start(K_POISON, 1, t, 3, 1, 0);   /* its only firing: not expired after the save */
    on_tick = ON_TICK_SAVE;
    nge_timer_advance(dispatch);
    CHECK(seen_count == 3 && is(1, NGE_EFFECT_TICK, e, t, K_POISON, 1, 1) &&
          is(2, NGE_EFFECT_ENDED, e, t, K_POISON, 1, END_SAVED));
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && !NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_TIMER));

    /* ---- the hook destroying the target: the effect is left to the target's owner ---- */
    t = reset();
    e = nge_effect_start(K_POISON, 1, t, 3, 0, 0);
    on_tick = ON_TICK_KILL;
    nge_timer_advance(dispatch);
    CHECK(!NGE_ENTITY_IS_ALIVE(t) && NGE_ENTITY_IS_ALIVE(e) && seen_count == 2);   /* the game's clean-up destroys what it owned */
    /* Its last cycle all the same */
    t = reset();
    e = nge_effect_start(K_POISON, 1, t, 3, 1, 0);
    on_tick = ON_TICK_KILL;
    nge_timer_advance(dispatch);
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && is(2, NGE_EFFECT_ENDED, e, t, K_POISON, 1, NGE_EFFECT_END_EXPIRED));

    /* ---- ending by the game: destroyed before the hook ---- */
    t = reset();
    e = nge_effect_start(K_POISON, 4, t, 3, 0, 0);
    nge_effect_end(e, END_CURED);
    CHECK(is(1, NGE_EFFECT_ENDED, e, t, K_POISON, 4, END_CURED));
    CHECK(NGE_ENTITY_STATE(e) == NGE_ENTITY_DYING && !NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_TIMER));
    CHECK(count(t) == 0 && nge_effect_kind(e) == K_POISON);   /* dying: still readable */
    /* Ending each in the iterator */
    t = reset();
    nge_effect_start(K_REGEN, 1, t, 5, 0, NGE_EFFECT_STACK);
    nge_effect_start(K_REGEN, 1, t, 5, 0, NGE_EFFECT_STACK);
    nge_effect_start(K_POISON, 1, t, 3, 0, 0);
    {
        uint8_t i;

        NGE_EFFECT_FOR_EACH(i, e, t) {
            if (nge_effect_kind(e) == K_REGEN) {
                nge_effect_end(e, END_CURED);
            }
        }
    }
    CHECK(count(t) == 1);

    /* ---- re-entry: a STARTED hook starting another effect ---- */
    t = reset();
    chain = 1;
    e = nge_effect_start(K_POISON, 1, t, 3, 0, 0);
    CHECK(count(t) == 2 && seen_count == 2 && is(0, NGE_EFFECT_STARTED, e, t, K_POISON, 1, 0) &&
          seen[1].what == NGE_EFFECT_STARTED && seen[1].kind == K_REGEN);

    /* ---- no entity free: refused, nothing started ---- */
    t = reset();
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(nge_effect_start(K_POISON, 1, t, 3, 0, 0) == NGE_ENTITY_INVALID && seen_count == 0);
    /* ...but a refresh needs none */
    t = reset();
    e = nge_effect_start(K_POISON, 1, t, 3, 0, 0);
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(nge_effect_start(K_POISON, 2, t, 3, 0, 0) == e);

    /* ---- init again: the same state ---- */
    nge_timer_init();
    CHECK(nge_timer_handler[NGE_TIMER_EFFECT] == NULL);
    nge_effect_init(game_hook);
    CHECK(nge_effect_game_hook == game_hook && nge_timer_handler[NGE_TIMER_EFFECT] == nge_effect_timer);

    /* ---- asserts ---- */
    t = reset();
    e = nge_effect_start(K_POISON, 1, t, 3, 0, 0);
    TEST_EXPECT_HALT(nge_effect_init(NULL));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_start(K_POISON, 0, t, 3, 0, 0));          /* power 0 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_start(K_POISON, 1, t, 0, 0, 0));          /* period 0 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_start(K_POISON, 1, t, 3, 0, 0x02));       /* an unknown flag */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_start(K_POISON, 1, 90, 3, 0, 0));         /* a free target */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_start(K_POISON, 1, NGE_ENTITY_INVALID, 3, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_end(t, END_CURED));                       /* not an effect */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_kind(t));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_effect_power(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);
    nge_effect_end(e, END_CURED);
    TEST_EXPECT_HALT(nge_effect_end(e, END_CURED));                       /* ended already */
    CHECK(test_halt_count == 1);
    /* No hook: started before nge_effect_init() */
    nge_entity_init();
    nge_timer_init();
    nge_effect_game_hook = NULL;
    t = nge_entity_create();
    nge_owner_set(t, AREA);
    TEST_EXPECT_HALT(nge_effect_start(K_POISON, 1, t, 3, 0, 0));
    CHECK(test_halt_count == 1);

    return test_summary("nge_effect");
}
