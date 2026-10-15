/* nge_timer.c: the timer system, the clock and firing order. */
#include "test.h"

#include <stdlib.h>

#include "ecs/nge_entity.h"
#include "ecs/systems/nge_timer.h"

/* Handler kinds */
#define K_RETURN   1   /* returns ret[id] */
#define K_WAIT     2   /* returns NGE_TIMER_WAIT */
#define K_KILL     3   /* destroys victim, returns 5 */
#define K_UNTIME   4   /* removes victim's timer, returns 5 */
#define K_SUICIDE  5   /* destroys itself, returns 1 (ignored) */
#define K_SPAWN    6   /* gives victim a 1-tick timer, returns NGE_TIMER_STOP */
#define K_RANDOM   7   /* returns a value from its state (randomised test) */

/* What dispatch saw */
typedef struct fired {
    nge_entity_id_t id;
    uint8_t kind;
    uint8_t cycles;
    uint32_t time;       /* clock in ticks when it fired */
} fired_t;

static fired_t seen[4000];
static int seen_count;
static uint8_t ret[NGE_ENTITY_MAX];
static nge_entity_id_t victim;

static uint32_t now(void)
{
    return (uint32_t)nge_timer_turns * NGE_TIMER_TICKS_PER_TURN + nge_timer_tick;
}

/* The randomised handlers' result: depends only on what both models agree on */
static uint8_t random_result(nge_entity_id_t id, uint8_t cycles, uint32_t time)
{
    uint32_t h = (id * 37u + time * 11u + cycles * 5u) % 23u;

    if (h == 0) {
        return NGE_TIMER_STOP;
    }
    if (h < 6) {
        return NGE_TIMER_REPEAT;
    }
    return (uint8_t)(h - 5);   /* 1 to 17 */
}

static uint8_t dispatch(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    fired_t *f = &seen[seen_count++];

    f->id = id;
    f->kind = kind;
    f->cycles = cycles;
    f->time = now();

    switch (kind) {
    case K_RETURN:
        return ret[id];
    case K_WAIT:
        return NGE_TIMER_WAIT;
    case K_KILL:
        nge_entity_destroy(victim);
        return 5;
    case K_UNTIME:
        nge_timer_remove(victim);
        return 5;
    case K_SUICIDE:
        nge_entity_destroy(id);
        return 1;
    case K_SPAWN:
        nge_timer_add(victim, K_RETURN, 1, 0, 0);
        return NGE_TIMER_STOP;
    case K_RANDOM:
        return random_result(id, cycles, f->time);
    default:
        return NGE_TIMER_STOP;
    }
}

static int fired(int i, nge_entity_id_t id, uint8_t cycles, uint32_t time)
{
    return i < seen_count && seen[i].id == id && seen[i].cycles == cycles && seen[i].time == time;
}

static int has_timer(nge_entity_id_t id)
{
    return (nge_entity_components[id] & NGE_COMPONENT_TIMER) != 0;
}

/* A fresh start: every entity free, the clock at 0, nothing seen */
static void reset(void)
{
    nge_entity_init();
    nge_timer_init();
    seen_count = 0;
}

/* ---- The reference: a plain tick-by-tick clock, for the randomised test ---- */

static uint8_t ref_on[NGE_ENTITY_MAX];
static uint8_t ref_count[NGE_ENTITY_MAX];
static uint8_t ref_period[NGE_ENTITY_MAX];
static uint8_t ref_cycles[NGE_ENTITY_MAX];
static fired_t ref_seen[4000];
static int ref_seen_count;
static uint32_t ref_time;

/* One tick: every timer down by one, then those at 0 fire in ID order */
static void ref_tick(void)
{
    int id;

    ref_time++;
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (ref_on[id]) {
            ref_count[id]--;
        }
    }
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        uint8_t next;

        if (!ref_on[id] || ref_count[id] != 0) {
            continue;
        }
        ref_seen[ref_seen_count].id = (nge_entity_id_t)id;
        ref_seen[ref_seen_count].cycles = ref_cycles[id];
        ref_seen[ref_seen_count].time = ref_time;
        ref_seen_count++;
        next = random_result((nge_entity_id_t)id, ref_cycles[id], ref_time);
        if (next == NGE_TIMER_STOP) {
            ref_on[id] = 0;
            continue;
        }
        ref_count[id] = (next == NGE_TIMER_REPEAT) ? ref_period[id] : next;
        if (ref_cycles[id] != 0 && --ref_cycles[id] == 0) {
            ref_on[id] = 0;
        }
    }
}

int main(void)
{
    nge_entity_id_t a;
    nge_entity_id_t b;
    nge_entity_id_t c;
    nge_entity_id_t id;
    int i;
    int ok;

    test_init();

    CHECK(NGE_COMPONENT_TIMER == NGE_COMPONENT(3));
    CHECK(NGE_TIMER_TICKS_PER_TURN == 10);

    /* ---- the clock at the start; nothing to do ---- */
    reset();
    CHECK(nge_timer_turns == 0 && nge_timer_tick == 0);
    CHECK(nge_timer_waiting() == NGE_ENTITY_INVALID);
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 0);

    /* ---- jumps to the next timer due; the others move on by the same ---- */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    ret[a] = 7;
    ret[b] = 4;
    nge_timer_add(a, K_RETURN, 5, 0, 0);
    nge_timer_add(b, K_RETURN, 3, 0, 0);
    CHECK(has_timer(a) && has_timer(b));
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 1 && fired(0, b, 0, 3) && seen[0].kind == K_RETURN);
    CHECK(nge_timer_count[a] == 2 && nge_timer_count[b] == 4);
    CHECK(nge_timer_advance(dispatch) == 2 && seen_count == 2 && fired(1, a, 0, 5));
    CHECK(nge_timer_advance(dispatch) == 2 && fired(2, b, 0, 7));   /* b: 3 + 4 */
    CHECK(nge_timer_advance(dispatch) == 4 && fired(3, b, 0, 11));  /* b again before a (12) */
    CHECK(nge_timer_turns == 1 && nge_timer_tick == 1);
    CHECK(nge_timer_advance(dispatch) == 1 && fired(4, a, 0, 12));

    /* ---- the same tick: ID order, whatever order they were added in ---- */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    ret[a] = ret[b] = ret[c] = NGE_TIMER_STOP;
    nge_timer_add(c, K_RETURN, 4, 0, 0);
    nge_timer_add(a, K_RETURN, 4, 0, 0);
    nge_timer_add(b, K_RETURN, 6, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 4 && seen_count == 2 && fired(0, a, 0, 4) && fired(1, c, 0, 4));
    /* STOP removed them */
    CHECK(!has_timer(a) && !has_timer(c) && has_timer(b));
    CHECK(nge_timer_advance(dispatch) == 2 && fired(2, b, 0, 6));
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 3);

    /* ---- REPEAT uses the period; cycles count down to removal ---- */
    reset();
    a = nge_entity_create();
    ret[a] = NGE_TIMER_REPEAT;
    nge_timer_add(a, K_RETURN, 2, 3, 3);
    CHECK(nge_timer_advance(dispatch) == 2 && fired(0, a, 3, 2) && nge_timer_cycles[a] == 2);
    CHECK(nge_timer_advance(dispatch) == 3 && fired(1, a, 2, 5));
    CHECK(nge_timer_advance(dispatch) == 3 && fired(2, a, 1, 8));   /* the last */
    CHECK(!has_timer(a));
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 3);
    /* A delay instead of REPEAT also counts a cycle */
    ret[a] = 9;
    nge_timer_add(a, K_RETURN, 1, 0, 2);
    CHECK(nge_timer_advance(dispatch) == 1 && fired(3, a, 2, 9));
    CHECK(nge_timer_advance(dispatch) == 9 && fired(4, a, 1, 18) && !has_timer(a));
    /* No limit: cycles stay 0 */
    ret[a] = NGE_TIMER_REPEAT;
    nge_timer_add(a, K_RETURN, 1, 1, 0);
    for (i = 0; i < 300; i++) {
        nge_timer_advance(dispatch);
    }
    CHECK(has_timer(a) && nge_timer_cycles[a] == 0 && seen_count == 5 + 300);

    /* ---- turns: every NGE_TIMER_TICKS_PER_TURN ticks, even in long steps ---- */
    reset();
    a = nge_entity_create();
    ret[a] = NGE_TIMER_DELAY_MAX;
    nge_timer_add(a, K_RETURN, NGE_TIMER_DELAY_MAX, 0, 0);
    CHECK(nge_timer_advance(dispatch) == NGE_TIMER_DELAY_MAX);
    CHECK(nge_timer_turns == 25 && nge_timer_tick == 2);
    CHECK(nge_timer_advance(dispatch) == NGE_TIMER_DELAY_MAX);
    CHECK(nge_timer_turns == 50 && nge_timer_tick == 4);
    nge_timer_set_delay(a, 6);
    CHECK(nge_timer_advance(dispatch) == 6 && nge_timer_turns == 51 && nge_timer_tick == 0);

    /* ---- WAIT: the clock stops until set_delay; the tick finishes first ---- */
    reset();
    a = nge_entity_create();   /* the player */
    b = nge_entity_create();
    ret[b] = 10;
    nge_timer_add(a, K_WAIT, 10, 0, 0);
    nge_timer_add(b, K_RETURN, 10, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 10 && seen_count == 2 && fired(0, a, 0, 10) && fired(1, b, 0, 10));
    CHECK(nge_timer_waiting() == a && nge_timer_count[a] == 0);
    CHECK(nge_timer_advance(dispatch) == 0 && nge_timer_advance(dispatch) == 0 && seen_count == 2);
    nge_timer_set_delay(a, 15);   /* the player's command */
    CHECK(nge_timer_waiting() == NGE_ENTITY_INVALID);
    CHECK(nge_timer_advance(dispatch) == 10 && fired(2, b, 0, 20));
    CHECK(nge_timer_advance(dispatch) == 5 && fired(3, a, 0, 25) && nge_timer_waiting() == a);
    /* Removing the waiting entity's timer, or replacing it, starts the clock */
    nge_timer_remove(a);
    CHECK(nge_timer_waiting() == NGE_ENTITY_INVALID);
    nge_timer_add(a, K_WAIT, 1, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 1 && nge_timer_waiting() == a);
    nge_timer_add(a, K_WAIT, 2, 0, 0);
    CHECK(nge_timer_waiting() == NGE_ENTITY_INVALID);
    CHECK(nge_timer_advance(dispatch) == 2 && nge_timer_waiting() == a);
    /* The waiting entity dies: the clock runs on without it */
    nge_entity_destroy(a);
    CHECK(nge_timer_advance(dispatch) == 2 && seen[seen_count - 1].id == b && seen[seen_count - 1].time == 30);
    CHECK(nge_timer_waiting() == NGE_ENTITY_INVALID);

    /* ---- handlers changing other timers in the same tick ---- */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    ret[b] = 5;
    ret[c] = 5;
    victim = b;
    nge_timer_add(a, K_KILL, 3, 0, 0);   /* kills b, due the same tick */
    nge_timer_add(b, K_RETURN, 3, 0, 0);
    nge_timer_add(c, K_RETURN, 3, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 2 && fired(0, a, 0, 3) && fired(1, c, 0, 3));
    /* b, dying, is left out of the next step too */
    CHECK(nge_timer_advance(dispatch) == 5 && seen_count == 4 && fired(2, a, 0, 8) && fired(3, c, 0, 8));

    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    ret[b] = 5;
    victim = b;
    nge_timer_add(a, K_UNTIME, 3, 0, 0);   /* removes b's timer, due the same tick */
    nge_timer_add(b, K_RETURN, 3, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 1 && fired(0, a, 0, 3) && !has_timer(b));

    /* A timer given to a later entity during the tick waits for its own tick */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    ret[b] = NGE_TIMER_STOP;
    victim = b;
    nge_timer_add(a, K_SPAWN, 2, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 2 && seen_count == 1 && has_timer(b) && !has_timer(a));
    CHECK(nge_timer_advance(dispatch) == 1 && fired(1, b, 0, 3));

    /* A handler destroying its own entity: the return value is ignored */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    ret[b] = 4;
    nge_timer_add(a, K_SUICIDE, 2, 0, 3);
    nge_timer_add(b, K_RETURN, 4, 0, 0);
    CHECK(nge_timer_advance(dispatch) == 2 && fired(0, a, 3, 2));
    CHECK(nge_timer_cycles[a] == 3 && nge_timer_count[a] == 0);   /* untouched */
    CHECK(nge_timer_advance(dispatch) == 2 && fired(1, b, 0, 4));

    /* A dying entity's timer doesn't count, even if it is the soonest */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    ret[b] = 4;
    nge_timer_add(a, K_RETURN, 1, 0, 0);
    nge_timer_add(b, K_RETURN, 4, 0, 0);
    nge_entity_destroy(a);
    CHECK(nge_timer_advance(dispatch) == 4 && seen_count == 1 && fired(0, b, 0, 4));
    CHECK(nge_timer_count[a] == 1);   /* not moved on */

    /* ---- randomised: the same firings as a plain tick-by-tick clock ---- */
    srand(7);
    reset();
    ref_seen_count = 0;
    ref_time = 0;
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        ref_on[id] = 0;
    }
    ok = 1;
    for (i = 0; i < 2000 && ok && seen_count < 3500 && ref_seen_count < 3500; i++) {
        /* Sometimes a new timer, on an entity with none */
        if (rand() % 3 == 0) {
            id = (nge_entity_id_t)(rand() % 20);
            if (NGE_ENTITY_STATE(id) == NGE_ENTITY_FREE) {
                while (nge_entity_create() != id) {
                }
            }
            if (!has_timer(id)) {
                uint8_t delay = (uint8_t)(1 + rand() % 30);
                uint8_t period = (uint8_t)(1 + rand() % 30);
                uint8_t cycles = (uint8_t)(rand() % 4);

                nge_timer_add(id, K_RANDOM, delay, period, cycles);
                ref_on[id] = 1;
                ref_count[id] = delay;
                ref_period[id] = period;
                ref_cycles[id] = cycles;
            }
        }
        /* One step, and the reference ticks up to the same time */
        nge_timer_advance(dispatch);
        while (ref_time < now()) {
            ref_tick();
        }
        ok = seen_count == ref_seen_count;
        for (id = 0; id < NGE_ENTITY_MAX && ok; id++) {
            ok = (has_timer(id) != 0) == ref_on[id] && (!ref_on[id] || nge_timer_count[id] == ref_count[id]);
        }
    }
    for (i = 0; i < seen_count && ok; i++) {
        ok = seen[i].id == ref_seen[i].id && seen[i].cycles == ref_seen[i].cycles && seen[i].time == ref_seen[i].time;
    }
    CHECK(ok);
    CHECK(seen_count > 500);

    /* ---- asserts ---- */
    reset();
    a = nge_entity_create();
    TEST_EXPECT_HALT(nge_timer_add(a, K_RETURN, 0, 0, 0));                        /* delay 0 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_add(a, K_RETURN, NGE_TIMER_DELAY_MAX + 1, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_add(a, K_RETURN, 1, NGE_TIMER_DELAY_MAX + 1, 0));  /* period */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_add(50, K_RETURN, 1, 0, 0));                       /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_set_delay(a, 1));                                  /* no timer */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_remove(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);
    /* WAIT with a cycle limit; REPEAT with no period */
    nge_timer_add(a, K_WAIT, 1, 0, 2);
    TEST_EXPECT_HALT(nge_timer_advance(dispatch));
    CHECK(test_halt_count == 1);
    reset();
    a = nge_entity_create();
    ret[a] = NGE_TIMER_REPEAT;
    nge_timer_add(a, K_RETURN, 1, 0, 0);
    TEST_EXPECT_HALT(nge_timer_advance(dispatch));
    CHECK(test_halt_count == 1);
    /* Two entities waiting at once */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    nge_timer_add(a, K_WAIT, 1, 0, 0);
    nge_timer_add(b, K_WAIT, 1, 0, 0);
    TEST_EXPECT_HALT(nge_timer_advance(dispatch));
    CHECK(test_halt_count == 1);

    return test_summary("timer");
}
