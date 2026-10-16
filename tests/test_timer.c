/* nge_timer.c: the timer system, the clock, firing order, stopping and starting. */
#include "test.h"

#include <stdlib.h>

#include "ecs/nge_entity.h"
#include "ecs/systems/nge_timer.h"

/* Handler kinds */
#define K_PLAIN    1   /* does nothing */
#define K_STOP     2   /* stops the clock (the player) */
#define K_KILL     3   /* destroys victim */
#define K_UNTIME   4   /* removes victim's timer */
#define K_SUICIDE  5   /* destroys itself */
#define K_SPAWN    6   /* gives victim a timer of period 1 */
#define K_UNSELF   7   /* removes its own timer */
#define K_SPEED    8   /* sets its own period to new_period */
#define K_READD    9   /* adds itself a new timer: period 7, 2 cycles */
#define K_RANDOM  10   /* changes its period or removes its timer, from its state (randomised test) */

/* What dispatch saw */
typedef struct fired {
    nge_entity_id_t id;
    uint8_t kind;
    uint8_t cycles;
    uint32_t time;       /* clock in ticks when it fired */
} fired_t;

static fired_t seen[4000];
static int seen_count;
static nge_entity_id_t victim;
static uint8_t new_period;

static uint32_t now(void)
{
    return (uint32_t)nge_timer_turns * NGE_TIMER_TICKS_PER_TURN + nge_timer_tick;
}

/* The randomised handler's choice: 0 keep, 1-30 a new period, 0xFF remove */
static uint8_t random_choice(nge_entity_id_t id, uint8_t cycles, uint32_t time)
{
    uint32_t h = (id * 37u + time * 11u + cycles * 5u) % 29u;

    if (h == 0) {
        return 0xFF;
    }
    if (h < 20) {
        return 0;
    }
    return (uint8_t)(h - 15);   /* 5 to 13 */
}

static void dispatch(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    fired_t *f = &seen[seen_count++];
    uint8_t c;

    f->id = id;
    f->kind = kind;
    f->cycles = cycles;
    f->time = now();

    switch (kind) {
    case K_STOP:
        nge_timer_stop();
        break;
    case K_KILL:
        nge_entity_destroy(victim);
        break;
    case K_UNTIME:
        nge_timer_remove(victim);
        break;
    case K_SUICIDE:
        nge_entity_destroy(id);
        break;
    case K_SPAWN:
        nge_timer_add(victim, K_PLAIN, 1, 0);
        break;
    case K_UNSELF:
        nge_timer_remove(id);
        break;
    case K_SPEED:
        nge_timer_set_period(id, new_period);
        break;
    case K_READD:
        nge_timer_add(id, K_PLAIN, 7, 2);
        break;
    case K_RANDOM:
        c = random_choice(id, cycles, f->time);
        if (c == 0xFF) {
            nge_timer_remove(id);
        } else if (c != 0) {
            nge_timer_set_period(id, c);
        }
        break;
    default:
        break;
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
        uint8_t c;

        if (!ref_on[id] || ref_count[id] != 0) {
            continue;
        }
        ref_seen[ref_seen_count].id = (nge_entity_id_t)id;
        ref_seen[ref_seen_count].cycles = ref_cycles[id];
        ref_seen[ref_seen_count].time = ref_time;
        ref_seen_count++;
        c = random_choice((nge_entity_id_t)id, ref_cycles[id], ref_time);
        if (c == 0xFF) {
            ref_on[id] = 0;
            continue;
        }
        if (c != 0) {
            ref_period[id] = c;
        }
        ref_count[id] = ref_period[id];
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
    CHECK(nge_timer_turns == 0 && nge_timer_tick == 0 && !nge_timer_stopped());
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 0);

    /* ---- every period, the first one period after adding; jumps to the next due ---- */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    nge_timer_add(a, K_PLAIN, 5, 0);
    nge_timer_add(b, K_PLAIN, 3, 0);
    CHECK(has_timer(a) && has_timer(b) && nge_timer_count[a] == 5 && nge_timer_period[a] == 5);
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 1 && fired(0, b, 0, 3) && seen[0].kind == K_PLAIN);
    CHECK(nge_timer_count[a] == 2 && nge_timer_count[b] == 3);   /* b starts its next period */
    CHECK(nge_timer_advance(dispatch) == 2 && fired(1, a, 0, 5));
    CHECK(nge_timer_advance(dispatch) == 1 && fired(2, b, 0, 6));
    CHECK(nge_timer_advance(dispatch) == 3 && fired(3, b, 0, 9));
    CHECK(nge_timer_advance(dispatch) == 1 && fired(4, a, 0, 10));
    CHECK(nge_timer_turns == 1 && nge_timer_tick == 0);

    /* ---- the same tick: ID order, whatever order they were added in ---- */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    nge_timer_add(c, K_UNSELF, 4, 0);
    nge_timer_add(a, K_UNSELF, 4, 0);
    nge_timer_add(b, K_UNSELF, 6, 0);
    CHECK(nge_timer_advance(dispatch) == 4 && seen_count == 2 && fired(0, a, 0, 4) && fired(1, c, 0, 4));
    CHECK(!has_timer(a) && !has_timer(c) && has_timer(b));   /* removed themselves */
    CHECK(nge_timer_advance(dispatch) == 2 && fired(2, b, 0, 6));
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 3);

    /* ---- cycles count down to removal; 0 is no limit ---- */
    reset();
    a = nge_entity_create();
    nge_timer_add(a, K_PLAIN, 2, 3);
    CHECK(nge_timer_advance(dispatch) == 2 && fired(0, a, 3, 2) && nge_timer_cycles[a] == 2);
    CHECK(nge_timer_advance(dispatch) == 2 && fired(1, a, 2, 4));
    CHECK(nge_timer_advance(dispatch) == 2 && fired(2, a, 1, 6));   /* the last */
    CHECK(!has_timer(a));
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 3);
    nge_timer_add(a, K_PLAIN, 1, 0);
    for (i = 0; i < 300; i++) {
        nge_timer_advance(dispatch);
    }
    CHECK(has_timer(a) && nge_timer_cycles[a] == 0 && seen_count == 3 + 300);

    /* ---- the period: changed by the handler, it starts the next one ---- */
    reset();
    a = nge_entity_create();
    new_period = 2;
    nge_timer_add(a, K_SPEED, 4, 0);
    CHECK(nge_timer_advance(dispatch) == 4 && fired(0, a, 0, 4) && nge_timer_count[a] == 2);
    CHECK(nge_timer_advance(dispatch) == 2 && fired(1, a, 0, 6));
    /* Changed from outside, it applies from the next firing on */
    reset();
    a = nge_entity_create();
    nge_timer_add(a, K_PLAIN, 5, 0);
    CHECK(nge_timer_advance(dispatch) == 5);
    nge_timer_set_period(a, 2);
    CHECK(nge_timer_advance(dispatch) == 5 && fired(1, a, 0, 10));   /* the period under way */
    CHECK(nge_timer_advance(dispatch) == 2 && fired(2, a, 0, 12));
    /* A handler that adds itself a new timer keeps it as added: no cycle counted */
    reset();
    a = nge_entity_create();
    nge_timer_add(a, K_READD, 3, 1);
    CHECK(nge_timer_advance(dispatch) == 3 && fired(0, a, 1, 3));
    CHECK(has_timer(a) && nge_timer_count[a] == 7 && nge_timer_cycles[a] == 2 && nge_timer_kind[a] == K_PLAIN);

    /* ---- turns: every NGE_TIMER_TICKS_PER_TURN ticks, even in long steps ---- */
    reset();
    a = nge_entity_create();
    nge_timer_add(a, K_PLAIN, 255, 0);
    CHECK(nge_timer_advance(dispatch) == 255 && nge_timer_turns == 25 && nge_timer_tick == 5);
    CHECK(nge_timer_advance(dispatch) == 255 && nge_timer_turns == 51 && nge_timer_tick == 0);

    /* ---- stop and start: the tick finishes first ---- */
    reset();
    a = nge_entity_create();   /* the player */
    b = nge_entity_create();
    nge_timer_add(a, K_STOP, 10, 0);
    nge_timer_add(b, K_PLAIN, 10, 0);
    CHECK(nge_timer_advance(dispatch) == 10 && seen_count == 2 && fired(0, a, 0, 10) && fired(1, b, 0, 10));
    CHECK(nge_timer_stopped() && nge_timer_count[a] == 10 && nge_timer_count[b] == 10);
    CHECK(nge_timer_advance(dispatch) == 0 && nge_timer_advance(dispatch) == 0 && seen_count == 2);
    CHECK(nge_timer_turns == 1 && nge_timer_tick == 0);   /* no time passed while stopped */
    nge_timer_start();                                    /* the player's command */
    CHECK(!nge_timer_stopped());
    CHECK(nge_timer_advance(dispatch) == 10 && seen_count == 4 && fired(2, a, 0, 20) && fired(3, b, 0, 20));
    CHECK(nge_timer_stopped());
    /* Stopped from outside: nothing moves */
    nge_timer_start();
    nge_timer_stop();
    CHECK(nge_timer_advance(dispatch) == 0 && seen_count == 4);
    nge_timer_start();
    /* init: running again */
    nge_timer_stop();
    nge_timer_init();
    CHECK(!nge_timer_stopped() && nge_timer_turns == 0);

    /* ---- handlers changing other timers in the same tick ---- */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    victim = b;
    nge_timer_add(a, K_KILL, 3, 0);   /* kills b, due the same tick */
    nge_timer_add(b, K_PLAIN, 3, 0);
    nge_timer_add(c, K_PLAIN, 3, 0);
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 2 && fired(0, a, 0, 3) && fired(1, c, 0, 3));
    /* b, dying, is left out of the next step too */
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 4 && fired(2, a, 0, 6) && fired(3, c, 0, 6));

    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    victim = b;
    nge_timer_add(a, K_UNTIME, 3, 0);   /* removes b's timer, due the same tick */
    nge_timer_add(b, K_PLAIN, 3, 0);
    CHECK(nge_timer_advance(dispatch) == 3 && seen_count == 1 && fired(0, a, 0, 3) && !has_timer(b));

    /* A timer given to a later entity during the tick waits a whole period */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    victim = b;
    nge_timer_add(a, K_SPAWN, 2, 1);
    CHECK(nge_timer_advance(dispatch) == 2 && seen_count == 1 && has_timer(b) && !has_timer(a));
    CHECK(nge_timer_advance(dispatch) == 1 && fired(1, b, 0, 3));

    /* A handler destroying its own entity: its timer is left as it was */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    nge_timer_add(a, K_SUICIDE, 2, 3);
    nge_timer_add(b, K_PLAIN, 4, 0);
    CHECK(nge_timer_advance(dispatch) == 2 && fired(0, a, 3, 2));
    CHECK(nge_timer_cycles[a] == 3 && nge_timer_count[a] == 0);   /* untouched */
    CHECK(nge_timer_advance(dispatch) == 2 && fired(1, b, 0, 4));

    /* A dying entity's timer doesn't count, even if it is the soonest */
    reset();
    a = nge_entity_create();
    b = nge_entity_create();
    nge_timer_add(a, K_PLAIN, 1, 0);
    nge_timer_add(b, K_PLAIN, 4, 0);
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
                uint8_t period = (uint8_t)(1 + rand() % 30);
                uint8_t cycles = (uint8_t)(rand() % 4);

                nge_timer_add(id, K_RANDOM, period, cycles);
                ref_on[id] = 1;
                ref_count[id] = period;
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
            ok = (has_timer(id) != 0) == ref_on[id] &&
                 (!ref_on[id] || (nge_timer_count[id] == ref_count[id] && nge_timer_period[id] == ref_period[id]));
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
    TEST_EXPECT_HALT(nge_timer_add(a, K_PLAIN, 0, 0));          /* period 0 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_add(50, K_PLAIN, 1, 0));         /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_set_period(a, 1));               /* no timer */
    CHECK(test_halt_count == 1);
    nge_timer_add(a, K_PLAIN, 1, 0);
    TEST_EXPECT_HALT(nge_timer_set_period(a, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_remove(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_timer_advance(NULL));
    CHECK(test_halt_count == 1);

    return test_summary("timer");
}
