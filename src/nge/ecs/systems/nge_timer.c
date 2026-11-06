/*
 * Timer system: banked in page 54. Its tables and the clock are in slot 0
 * (nge_timer_cdata.c); it calls the game's dispatch function, in common
 * memory, which may call banked handlers in other pages.
 */
#ifdef __SDCC
#pragma codeseg PAGE_54
#pragma constseg PAGE_54
#endif

#include "ecs/systems/nge_timer.h"

#include "core/nge_util.h"

typedef char nge_timer_ticks_check[(NGE_TIMER_TICKS_PER_TURN >= 1 && NGE_TIMER_TICKS_PER_TURN <= 255) ? 1 : -1];

/* Alive, with a timer: the entities the clock drives */
#define HAS_TIMER(id) \
    (NGE_ENTITY_IS_ALIVE(id) && NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_TIMER))

/* An entity in use, with a timer unless adding one */
static void check_entity(nge_entity_id_t id, uint8_t with_timer)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    (void)with_timer;
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(!with_timer || NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_TIMER));
}

static void remove_timer(nge_entity_id_t id)
{
    NGE_ENTITY_REMOVE_COMPONENT(id, NGE_COMPONENT_TIMER);
}

void nge_timer_init(void) NGE_BANKED
{
    nge_timer_turns = 0;
    nge_timer_tick = 0;
    nge_timer_halted = 0;
}

void nge_timer_add(nge_entity_id_t id, uint8_t kind, uint8_t period, uint8_t cycles) NGE_BANKED
{
    check_entity(id, 0);
    nge_util_assert(period != 0);

    nge_timer_count[id] = period;
    nge_timer_period[id] = period;
    nge_timer_cycles[id] = cycles;
    nge_timer_kind[id] = kind;
    NGE_ENTITY_ADD_COMPONENT(id, NGE_COMPONENT_TIMER);
}

void nge_timer_set_period(nge_entity_id_t id, uint8_t period) NGE_BANKED
{
    check_entity(id, 1);
    nge_util_assert(period != 0);

    nge_timer_period[id] = period;
}

void nge_timer_remove(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id, 0);

    remove_timer(id);
}

void nge_timer_stop(void) NGE_BANKED
{
    nge_timer_halted = 1;
}

void nge_timer_start(void) NGE_BANKED
{
    nge_timer_halted = 0;
}

/* A timer has fired and its handler kept it: start its next period, count the cycle */
static void next_period(nge_entity_id_t id)
{
    uint8_t cycles = nge_timer_cycles[id];

    nge_timer_count[id] = nge_timer_period[id];
    if (cycles != 0) {
        if (--cycles == 0) {
            remove_timer(id);
        } else {
            nge_timer_cycles[id] = cycles;
        }
    }
}

uint8_t nge_timer_advance(nge_timer_dispatch_fn dispatch) NGE_BANKED
{
    nge_entity_id_t id;
    uint8_t step = 0xFF;
    uint8_t found = 0;
    uint16_t tick;

    nge_util_assert(dispatch != 0);

    if (nge_timer_halted) {
        return 0;
    }

    /*
     * Three passes, as handlers may change other timers: the step to the
     * next timer due, moving every timer on by it, then firing in ID order
     */
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (HAS_TIMER(id) && nge_timer_count[id] <= step) {
            step = nge_timer_count[id];
            found = 1;
        }
    }
    if (!found) {
        return 0;
    }
    nge_util_assert(step != 0);

    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (HAS_TIMER(id)) {
            nge_timer_count[id] -= step;
        }
    }

    /*
     * The game clock is kept in turns and ticks: nge_timer_turns whole
     * turns plus nge_timer_tick ticks into the current turn, with
     * NGE_TIMER_TICKS_PER_TURN ticks to a turn.
     *
     * Advance it by step ticks, as turns and ticks: add step to the ticks,
     * then take each whole turn out of them and add it to the turns,
     * leaving the ticks below NGE_TIMER_TICKS_PER_TURN. The sum is 16 bits,
     * as it can pass 255 (up to 254 + 255); the loop runs at most 25 times
     * at 10 ticks a turn, usually once or not at all: cheaper on the Z80
     * than a division.
     */
    tick = (uint16_t)nge_timer_tick + step;
    while (tick >= NGE_TIMER_TICKS_PER_TURN) {
        tick -= NGE_TIMER_TICKS_PER_TURN;
        nge_timer_turns++;
    }
    nge_timer_tick = (uint8_t)tick;

    /*
     * Fire the timers due now, those whose count has reached 0, in entity
     * ID order: call the game's handler for each through dispatch, then
     * start its next period (next_period(): with the period as the handler
     * left it, counting a cycle). A handler may change other timers, so
     * each timer is checked just before it fires, not from a list made
     * beforehand: one removed, or whose entity was destroyed, by an earlier
     * handler in this tick doesn't fire, and one added in this tick waits a
     * whole period. nge_timer_stop() in a handler takes effect after the
     * tick: the rest of its timers still fire.
     */
    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (!HAS_TIMER(id) || nge_timer_count[id] != 0) {
            continue;
        }
        dispatch(id, nge_timer_kind[id], nge_timer_cycles[id]);
        /*
         * Unless the handler destroyed its entity or removed its timer, or
         * added a new one (its count is a whole period already)
         */
        if (HAS_TIMER(id) && nge_timer_count[id] == 0) {
            next_period(id);
        }
    }
    return step;
}
