/* nge_event.c: the event queue and processing. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "event/nge_event.h"

#define EV_A  (NGE_EVENT_USER + 0)
#define EV_B  (NGE_EVENT_USER + 1)
#define EV_C  (NGE_EVENT_USER + 2)
#define EV_LOOP  (NGE_EVENT_USER + 3)

/* What the dispatch function saw */
static nge_event_t seen[200];
static int seen_count;

static void record(const nge_event_t *e)
{
    seen[seen_count++] = *e;
}

/* A cascade: A posts B for the same target, B posts C */
static void cascade(const nge_event_t *e)
{
    record(e);
    if (e->id == EV_A) {
        nge_event_post(e->target, EV_B, e->source, (uint8_t)(e->p1 + 1), 0);
    } else if (e->id == EV_B) {
        nge_event_post0(e->target, EV_C);
    }
}

/* Changes its copy: later events must not see the change */
static void vandal(const nge_event_t *e)
{
    record(e);
    ((nge_event_t *)e)->id = EV_C;
    ((nge_event_t *)e)->p2 = 99;
}

/* Posts first, then reads its event: posting must not overwrite it */
static void post_then_read(const nge_event_t *e)
{
    if (e->target != 77) {
        nge_event_post(77, EV_C, 77, 77, 77);
    }
    record(e);
}

/* Keeps posting: the runaway guard stops it */
static void runaway(const nge_event_t *e)
{
    record(e);
    nge_event_post0(e->target, EV_LOOP);
}

static int is(const nge_event_t *e, uint8_t target, uint8_t id, uint8_t source, uint8_t p1, uint8_t p2)
{
    return e->target == target && e->id == id && e->source == source && e->p1 == p1 && e->p2 == p2;
}

int main(void)
{
    int i;
    int ok;

    test_init();

    CHECK(sizeof(nge_event_t) == 5);
    CHECK(NGE_EVENT_NONE == NGE_ENTITY_INVALID);

    /* ---- empty ---- */
    nge_event_init();
    CHECK(nge_event_count() == 0);
    seen_count = 0;
    CHECK(nge_event_process(record) == 0 && seen_count == 0);

    /* ---- posting: every field, in order; post0 defaults ---- */
    nge_event_post(3, EV_A, 7, 1, 4);
    nge_event_post0(5, EV_B);
    nge_event_post(NGE_EVENT_NONE, EV_C, NGE_EVENT_NONE, 255, 128);
    CHECK(nge_event_count() == 3);
    seen_count = 0;
    CHECK(nge_event_process(record) == 3 && nge_event_count() == 0);
    CHECK(is(&seen[0], 3, EV_A, 7, 1, 4));
    CHECK(is(&seen[1], 5, EV_B, NGE_EVENT_NONE, 0, 0));
    CHECK(is(&seen[2], NGE_EVENT_NONE, EV_C, NGE_EVENT_NONE, 255, 128));
    /* Each event once */
    seen_count = 0;
    CHECK(nge_event_process(record) == 0 && seen_count == 0);

    /* ---- full queue: asserts at NGE_EVENT_QUEUE; order kept across the wrap ---- */
    for (i = 0; i < NGE_EVENT_QUEUE; i++) {
        nge_event_post((uint8_t)i, EV_A, 0, (uint8_t)i, 0);
    }
    CHECK(nge_event_count() == NGE_EVENT_QUEUE);
    TEST_EXPECT_HALT(nge_event_post(99, EV_B, 0, 0, 0));
    CHECK(test_halt_count == 1);
    seen_count = 0;
    nge_event_process(record);
    ok = seen_count == NGE_EVENT_QUEUE;
    for (i = 0; i < seen_count; i++) {
        ok = ok && is(&seen[i], (uint8_t)i, EV_A, 0, (uint8_t)i, 0);
    }
    CHECK(ok);

    /* ---- flush ---- */
    nge_event_post0(1, EV_A);
    nge_event_post0(2, EV_A);
    nge_event_flush();
    CHECK(nge_event_count() == 0);
    seen_count = 0;
    CHECK(nge_event_process(record) == 0);

    /* ---- cascade: events posted by handlers come after those waiting, in the same call ---- */
    nge_event_post(1, EV_A, 9, 10, 0);
    nge_event_post0(2, EV_C);
    seen_count = 0;
    CHECK(nge_event_process(cascade) == 4);
    CHECK(is(&seen[0], 1, EV_A, 9, 10, 0));
    CHECK(is(&seen[1], 2, EV_C, NGE_EVENT_NONE, 0, 0));
    CHECK(is(&seen[2], 1, EV_B, 9, 11, 0));
    CHECK(is(&seen[3], 1, EV_C, NGE_EVENT_NONE, 0, 0));
    CHECK(nge_event_count() == 0);

    /* ---- handlers get a copy ---- */
    nge_event_post(1, EV_A, 2, 3, 4);
    nge_event_post(1, EV_A, 2, 3, 4);
    seen_count = 0;
    nge_event_process(vandal);
    CHECK(seen_count == 2 && is(&seen[1], 1, EV_A, 2, 3, 4));

    /* Posting from a handler when the queue was full reuses the freed slot, not the event being handled */
    for (i = 0; i < NGE_EVENT_QUEUE; i++) {
        nge_event_post((uint8_t)i, EV_A, 1, 2, 3);
    }
    seen_count = 0;
    nge_event_process(post_then_read);
    CHECK(seen_count == 2 * NGE_EVENT_QUEUE);
    CHECK(is(&seen[0], 0, EV_A, 1, 2, 3) && is(&seen[NGE_EVENT_QUEUE - 1], NGE_EVENT_QUEUE - 1, EV_A, 1, 2, 3));
    CHECK(is(&seen[NGE_EVENT_QUEUE], 77, EV_C, 77, 77, 77));

    /* ---- runaway handlers: the guard asserts after NGE_EVENT_PROCESS_MAX ---- */
    nge_event_post0(1, EV_LOOP);
    seen_count = 0;
    TEST_EXPECT_HALT(nge_event_process(runaway));
    CHECK(test_halt_count == 1 && seen_count == NGE_EVENT_PROCESS_MAX);
    nge_event_flush();

    TEST_EXPECT_HALT(nge_event_process(NULL));
    CHECK(test_halt_count == 1);

    /* ---- init again: an empty queue, whatever was waiting ---- */
    nge_event_post0(1, EV_A);
    nge_event_post0(2, EV_B);
    nge_event_init();
    seen_count = 0;
    CHECK(nge_event_count() == 0 && nge_event_process(record) == 0 && seen_count == 0);
    nge_event_post0(3, EV_C);
    CHECK(nge_event_process(record) == 1 && is(&seen[0], 3, EV_C, NGE_EVENT_NONE, 0, 0));

    return test_summary("event");
}
