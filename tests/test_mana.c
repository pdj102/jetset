/* game/components: the mana component, spending and restoring MP. */
#include "test.h"

#include "ecs/nge_entity.h"
#include "event/nge_event.h"

#include "game/components/mana.h"
#include "game/events.h"

/* The events posted since the last call */
static nge_event_t seen[8];
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

/* Event i is id on target, p1 the amount, no source */
static int is(int i, uint8_t target, uint8_t id, uint8_t p1)
{
    return i < seen_count && seen[i].target == target && seen[i].id == id &&
           seen[i].source == NGE_EVENT_NONE && seen[i].p1 == p1;
}

int main(void)
{
    mana_base_t b;
    nge_entity_id_t a;
    nge_entity_id_t none;

    test_init();
    nge_entity_init();
    nge_event_init();

    /* ---- add: full, at the base's maximum ---- */
    b.mp = 5;
    CHECK(MANA_BASE_HAS(&b));
    a = nge_entity_create();
    none = nge_entity_create();
    CHECK(!MANA_IS(a));
    mana_add(a, &b);
    CHECK(MANA_IS(a) && !MANA_IS(none));
    CHECK(mana_mp[a] == 5 && mana_mp_max[a] == 5);
    CHECK(events() == 0);

    /* ---- MANA_HAS: at least n ---- */
    CHECK(MANA_HAS(a, 0) && MANA_HAS(a, 5) && !MANA_HAS(a, 6));

    /* ---- spend: down by the amount, posted ---- */
    mana_spend(a, 2);
    CHECK(mana_mp[a] == 3 && mana_mp_max[a] == 5);
    CHECK(events() == 1 && is(0, a, EV_MANA_SPENT, 2));
    CHECK(MANA_HAS(a, 3) && !MANA_HAS(a, 4));
    mana_spend(a, 0);                              /* nothing: nothing posted */
    CHECK(mana_mp[a] == 3 && events() == 0);
    mana_spend(a, 3);                              /* all of them */
    CHECK(mana_mp[a] == 0 && events() == 1 && is(0, a, EV_MANA_SPENT, 3));
    CHECK(MANA_HAS(a, 0) && !MANA_HAS(a, 1));

    /* ---- restore: up to the maximum, the amount returned and posted ---- */
    CHECK(mana_restore(a, 2) == 2 && mana_mp[a] == 2);
    CHECK(events() == 1 && is(0, a, EV_MANA_RESTORED, 2));
    CHECK(mana_restore(a, 255) == 3 && mana_mp[a] == 5);   /* only the room left */
    CHECK(events() == 1 && is(0, a, EV_MANA_RESTORED, 3));
    CHECK(mana_restore(a, 1) == 0 && mana_mp[a] == 5);     /* full: nothing */
    CHECK(mana_restore(a, 0) == 0 && events() == 0);

    /* ---- a new add starts afresh ---- */
    mana_spend(a, 4);
    b.mp = 9;
    mana_add(a, &b);
    CHECK(mana_mp[a] == 9 && mana_mp_max[a] == 9);
    events();

    /* ---- asserts: spending too much, no component, no MP in the base ---- */
    TEST_EXPECT_HALT(mana_spend(a, 10));
    CHECK(test_halt_count == 1 && mana_mp[a] == 9);         /* halted before any change */
    TEST_EXPECT_HALT(mana_spend(none, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(mana_restore(none, 1));
    CHECK(test_halt_count == 1);
    b.mp = 0;
    CHECK(!MANA_BASE_HAS(&b));
    TEST_EXPECT_HALT(mana_add(none, &b));
    CHECK(test_halt_count == 1);

    return test_summary("mana");
}
