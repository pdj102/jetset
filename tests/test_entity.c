/* nge_entity.c: lifecycle, alive list, stacks, flags and components. */
#include "test.h"

#include <stdlib.h>

#include "ecs/nge_entity.h"

#define F_USER0  NGE_ENTITY_FLAG_USER
#define F_USER4  0x80

/* Each alive entity appears exactly once in the alive list, and nothing else does */
static int alive_list_ok(void)
{
    uint8_t seen[NGE_ENTITY_MAX] = {0};
    int alive = 0;
    int i;

    for (i = 0; i < nge_entity_alive_count; i++) {
        nge_entity_id_t id = nge_entity_alive[i];
        if (id >= NGE_ENTITY_MAX || seen[id]++ || !NGE_ENTITY_IS_ALIVE(id)) {
            return 0;
        }
    }
    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        alive += NGE_ENTITY_IS_ALIVE(i);
    }
    return alive == nge_entity_alive_count;
}

/* ---- Randomised comparison with a reference model ---- */

static uint8_t model_state[NGE_ENTITY_MAX];
static uint8_t model_flags[NGE_ENTITY_MAX];        /* bits 2-7 */
static uint16_t model_comp[NGE_ENTITY_MAX];
static uint8_t model_dying[NGE_ENTITY_MAX];        /* stack */
static int model_dying_top;

static int model_count(uint8_t state)
{
    int i;
    int n = 0;

    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        n += model_state[i] == state;
    }
    return n;
}

static int matches_model(void)
{
    int i;

    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        if (NGE_ENTITY_STATE(i) != model_state[i] ||
            (nge_entity_flags[i] & NGE_ENTITY_FLAG_MASK) != model_flags[i] ||
            nge_entity_components[i] != model_comp[i]) {
            return 0;
        }
    }
    return alive_list_ok() &&
           nge_entity_free_count() == model_count(NGE_ENTITY_FREE) &&
           nge_entity_dying_count() == model_dying_top &&
           nge_entity_alive_count == model_count(NGE_ENTITY_ALIVE);
}

static void random_ops(void)
{
    int step;
    int ok = 1;

    srand(4242);
    nge_entity_init();
    memset(model_state, NGE_ENTITY_FREE, sizeof(model_state));
    memset(model_flags, 0, sizeof(model_flags));
    memset(model_comp, 0, sizeof(model_comp));
    model_dying_top = 0;

    for (step = 0; step < 20000 && ok; step++) {
        nge_entity_id_t id = (nge_entity_id_t)(rand() % NGE_ENTITY_MAX);
        uint8_t f = (uint8_t)(rand() & NGE_ENTITY_FLAG_MASK);
        uint16_t c = (uint16_t)rand();

        switch (rand() % 8) {
        case 0:
        case 1: {
            nge_entity_id_t got = nge_entity_create();
            if (model_count(NGE_ENTITY_FREE) == 0) {
                ok = got == NGE_ENTITY_INVALID;
            } else {
                ok = got < NGE_ENTITY_MAX && model_state[got] == NGE_ENTITY_FREE;
                model_state[got] = NGE_ENTITY_ALIVE;
                model_flags[got] = 0;
                model_comp[got] = 0;
            }
            break;
        }
        case 2:
            if (model_state[id] != NGE_ENTITY_FREE) {
                uint8_t r = nge_entity_destroy(id);
                if (model_state[id] == NGE_ENTITY_ALIVE && !(model_flags[id] & NGE_ENTITY_PERSISTENT)) {
                    ok = r == 1;
                    model_state[id] = NGE_ENTITY_DYING;
                    model_dying[model_dying_top++] = id;
                } else {
                    ok = r == 0;
                }
            }
            break;
        case 3: {
            nge_entity_id_t got = nge_entity_next_dying();
            if (model_dying_top == 0) {
                ok = got == NGE_ENTITY_INVALID;
            } else {
                ok = got == model_dying[--model_dying_top];
                nge_entity_release(got);
                model_state[got] = NGE_ENTITY_FREE;
                model_flags[got] = 0;
                model_comp[got] = 0;
            }
            break;
        }
        case 4:
            if (model_state[id] != NGE_ENTITY_FREE) {
                nge_entity_set_flags(id, f);
                model_flags[id] |= f;
            }
            break;
        case 5:
            if (model_state[id] != NGE_ENTITY_FREE) {
                nge_entity_clear_flags(id, f);
                model_flags[id] &= (uint8_t)~f;
            }
            break;
        case 6:
            if (model_state[id] != NGE_ENTITY_FREE) {
                nge_entity_set_components(id, c);
                model_comp[id] |= c;
            }
            break;
        default:
            if (model_state[id] != NGE_ENTITY_FREE) {
                nge_entity_clear_components(id, c);
                model_comp[id] &= (uint16_t)~c;
            }
            break;
        }
        ok = ok && matches_model();
    }
    CHECK(ok);
}

int main(void)
{
    nge_entity_id_t id;
    nge_entity_id_t a;
    nge_entity_id_t b;
    nge_entity_id_t c;
    uint8_t i;
    uint8_t visits[NGE_ENTITY_MAX];
    int n;
    int ok;

    test_init();

    /* ---- init and create ---- */
    nge_entity_init();
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX);
    CHECK(nge_entity_alive_count == 0 && nge_entity_dying_count() == 0);
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    CHECK(a == 0 && b == 1 && c == 2);                    /* handed out in order */
    CHECK(NGE_ENTITY_IS_ALIVE(a) && nge_entity_flags[a] == NGE_ENTITY_ALIVE);
    CHECK(nge_entity_components[a] == 0);
    CHECK(nge_entity_alive_count == 3 && alive_list_ok());
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX - 3);

    /* All used: INVALID */
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(nge_entity_free_count() == 0 && nge_entity_alive_count == NGE_ENTITY_MAX);
    CHECK(nge_entity_create() == NGE_ENTITY_INVALID);
    CHECK(alive_list_ok());

    /* ---- destroy: alive -> dying ---- */
    nge_entity_init();
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    nge_entity_set_flags(a, F_USER0);
    CHECK(nge_entity_destroy(a) == 1);
    CHECK(NGE_ENTITY_STATE(a) == NGE_ENTITY_DYING);
    CHECK(NGE_ENTITY_HAS_FLAGS(a, F_USER0));               /* flags kept until release */
    CHECK(nge_entity_alive_count == 2 && alive_list_ok());
    CHECK(nge_entity_dying_count() == 1);
    CHECK(nge_entity_destroy(a) == 0);                     /* already dying: no-op */
    CHECK(nge_entity_dying_count() == 1 && nge_entity_alive_count == 2);

    /* Persistent: not destroyed until the flag is cleared */
    nge_entity_set_flags(b, NGE_ENTITY_PERSISTENT);
    CHECK(nge_entity_destroy(b) == 0 && NGE_ENTITY_IS_ALIVE(b));
    nge_entity_clear_flags(b, NGE_ENTITY_PERSISTENT);
    CHECK(nge_entity_destroy(b) == 1);

    /* ---- next_dying and release: LIFO, then free ---- */
    CHECK(nge_entity_next_dying() == b);
    nge_entity_release(b);
    CHECK(nge_entity_next_dying() == a);
    nge_entity_release(a);
    CHECK(nge_entity_next_dying() == NGE_ENTITY_INVALID);
    CHECK(NGE_ENTITY_STATE(a) == NGE_ENTITY_FREE && nge_entity_flags[a] == 0);
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX - 1);
    CHECK(nge_entity_create() == a);                        /* last released, first reused */
    CHECK(nge_entity_flags[a] == NGE_ENTITY_ALIVE && nge_entity_components[a] == 0);

    /* ---- flags ---- */
    nge_entity_init();
    a = nge_entity_create();
    nge_entity_set_flags(a, F_USER0 | F_USER4);
    CHECK(NGE_ENTITY_IS_ALIVE(a));                         /* state untouched */
    CHECK(NGE_ENTITY_HAS_FLAGS(a, F_USER0 | F_USER4) && nge_entity_has_flags(a, F_USER4));
    CHECK(!NGE_ENTITY_HAS_FLAGS(a, F_USER0 | NGE_ENTITY_PERSISTENT));
    CHECK(NGE_ENTITY_HAS_ANY_FLAGS(a, F_USER0 | NGE_ENTITY_PERSISTENT));
    nge_entity_clear_flags(a, F_USER0);
    CHECK(!NGE_ENTITY_HAS_ANY_FLAGS(a, F_USER0) && NGE_ENTITY_HAS_FLAGS(a, F_USER4));
    CHECK(NGE_ENTITY_IS_ALIVE(a));

    /* ---- components ---- */
    nge_entity_set_components(a, NGE_COMPONENT(0) | NGE_COMPONENT(15));
    CHECK(nge_entity_components[a] == 0x8001);
    CHECK(NGE_ENTITY_HAS_COMPONENTS(a, NGE_COMPONENT(15)));
    CHECK(nge_entity_has_components(a, NGE_COMPONENT(0) | NGE_COMPONENT(15)));
    CHECK(!NGE_ENTITY_HAS_COMPONENTS(a, NGE_COMPONENT(0) | NGE_COMPONENT(3)));
    CHECK(NGE_ENTITY_HAS_ANY_COMPONENTS(a, NGE_COMPONENT(0) | NGE_COMPONENT(3)));
    nge_entity_clear_components(a, NGE_COMPONENT(0));
    CHECK(nge_entity_components[a] == 0x8000);
    nge_entity_destroy(a);
    nge_entity_release(nge_entity_next_dying());
    CHECK(nge_entity_components[a] == 0);                  /* cleared on release */

    /* ---- iteration: destroying and creating inside the loop ---- */
    nge_entity_init();
    for (n = 0; n < 20; n++) {
        nge_entity_create();
    }
    memset(visits, 0, sizeof(visits));
    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        visits[id]++;
        if (id % 3 == 0) {
            nge_entity_destroy(id);                         /* current entity */
        }
        if (id == 10) {
            nge_entity_create();                            /* new: not visited now */
        }
    }
    ok = 1;
    for (n = 0; n < 20; n++) {
        ok = ok && visits[n] == 1;
    }
    CHECK(ok);
    CHECK(visits[20] == 0 && NGE_ENTITY_IS_ALIVE(20));
    CHECK(nge_entity_alive_count == 20 - 7 + 1 && alive_list_ok());

    /* ---- clean-up that destroys more entities empties the dying stack ---- */
    nge_entity_init();
    for (n = 0; n < 10; n++) {
        nge_entity_create();
    }
    nge_entity_destroy(0);
    n = 0;
    while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
        if (id + 1 < 10) {
            nge_entity_destroy(id + 1);                     /* clean-up kills the next one */
        }
        nge_entity_release(id);
        n++;
    }
    CHECK(n == 10 && nge_entity_free_count() == NGE_ENTITY_MAX);
    CHECK(nge_entity_alive_count == 0 && nge_entity_dying_count() == 0);

    /* ---- asserts ---- */
    nge_entity_init();
    a = nge_entity_create();
    TEST_EXPECT_HALT(nge_entity_release(a));                /* alive, not dying */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_destroy(50));               /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_destroy(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_destroy(NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_set_flags(a, NGE_ENTITY_DYING));   /* state bits */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_set_flags(50, F_USER0));           /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_set_components(NGE_ENTITY_MAX, 1));
    CHECK(test_halt_count == 1);
    CHECK(NGE_ENTITY_IS_ALIVE(a) && alive_list_ok());

    /* ---- randomised ---- */
    random_ops();

    return test_summary("entity");
}
