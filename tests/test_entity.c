/* nge_entity.c: lifecycle, used list, stacks, flags and components. */
#include "test.h"

#include <stdlib.h>

#include "ecs/components/nge_components.h"
#include "ecs/nge_entity.h"

#define F_USER0  NGE_ENTITY_FLAG_USER
#define F_USER4  0x80

/* Each entity in use appears exactly once in the used list, and nothing else does */
static int used_list_ok(void)
{
    uint8_t seen[NGE_ENTITY_MAX] = {0};
    int used = 0;
    int i;

    for (i = 0; i < nge_entity_used_count; i++) {
        nge_entity_id_t id = nge_entity_used[i];
        if (id >= NGE_ENTITY_MAX || seen[id]++ || NGE_ENTITY_STATE(id) == NGE_ENTITY_FREE) {
            return 0;
        }
    }
    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        used += NGE_ENTITY_STATE(i) != NGE_ENTITY_FREE;
    }
    return used == nge_entity_used_count;
}

/* Entities the alive loop visits */
static int alive_visited(void)
{
    nge_entity_id_t id;
    uint8_t i;
    int n = 0;

    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        n++;
    }
    return n;
}

/* ---- Randomised comparison with a reference model ---- */

static uint8_t model_state[NGE_ENTITY_MAX];
static uint8_t model_flags[NGE_ENTITY_MAX];        /* bits 2-7 */
static uint32_t model_comp[NGE_ENTITY_MAX];   /* bit n: component n */
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

/* Entity i's component tables, as the model's bits */
static uint32_t comp_bits(int i)
{
    uint32_t bits = 0;
    int t;

    for (t = 0; t < NGE_COMPONENT_TABLES; t++) {
        bits |= (uint32_t)nge_entity_components[t][i] << (8 * t);
    }
    return bits;
}

static int matches_model(void)
{
    int i;

    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        if (NGE_ENTITY_STATE(i) != model_state[i] ||
            (nge_entity_flags[i] & NGE_ENTITY_FLAG_MASK) != model_flags[i] ||
            comp_bits(i) != model_comp[i]) {
            return 0;
        }
    }
    return used_list_ok() &&
           nge_entity_free_count() == model_count(NGE_ENTITY_FREE) &&
           nge_entity_dying_count() == model_dying_top &&
           alive_visited() == model_count(NGE_ENTITY_ALIVE);
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
        uint8_t n = (uint8_t)(rand() % NGE_COMPONENT_MAX);   /* a component number */

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
                nge_entity_set_component(id, NGE_COMPONENT(n));
                model_comp[id] |= (uint32_t)1 << n;
            }
            break;
        default:
            if (model_state[id] != NGE_ENTITY_FREE) {
                nge_entity_clear_component(id, NGE_COMPONENT(n));
                model_comp[id] &= ~((uint32_t)1 << n);
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
    uint8_t first;

    test_init();

    /* ---- init and create ---- */
    nge_entity_init();
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX);
    CHECK(nge_entity_used_count == 0 && nge_entity_dying_count() == 0);
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    CHECK(a == 0 && b == 1 && c == 2);                    /* handed out in order */
    CHECK(NGE_ENTITY_IS_ALIVE(a) && nge_entity_flags[a] == NGE_ENTITY_ALIVE);
    CHECK(comp_bits(a) == 0);
    CHECK(nge_entity_used_count == 3 && used_list_ok());
    CHECK(nge_entity_free_count() == NGE_ENTITY_MAX - 3);

    /* All used: INVALID */
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(nge_entity_free_count() == 0 && nge_entity_used_count == NGE_ENTITY_MAX);
    CHECK(nge_entity_create() == NGE_ENTITY_INVALID);
    CHECK(used_list_ok());

    /* ---- destroy: alive -> dying ---- */
    nge_entity_init();
    a = nge_entity_create();
    b = nge_entity_create();
    c = nge_entity_create();
    nge_entity_set_flags(a, F_USER0);
    CHECK(nge_entity_destroy(a) == 1);
    CHECK(NGE_ENTITY_STATE(a) == NGE_ENTITY_DYING);
    CHECK(NGE_ENTITY_HAS_FLAGS(a, F_USER0));               /* flags kept until release */
    CHECK(nge_entity_used_count == 3 && used_list_ok());   /* listed until released */
    CHECK(alive_visited() == 2);                           /* but not an alive one */
    CHECK(nge_entity_dying_count() == 1);
    CHECK(nge_entity_destroy(a) == 0);                     /* already dying: no-op */
    CHECK(nge_entity_dying_count() == 1 && alive_visited() == 2);

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
    CHECK(nge_entity_used_count == 1 && used_list_ok());   /* off the list once released */
    CHECK(nge_entity_create() == a);                        /* last released, first reused */
    CHECK(nge_entity_flags[a] == NGE_ENTITY_ALIVE && comp_bits(a) == 0);

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

    /* ---- components: the handle, and the first and last of each table ---- */
    CHECK(NGE_COMPONENT_MAX == 24);
    CHECK(NGE_COMPONENT(0) == 0x0001 && NGE_COMPONENT(7) == 0x0080);
    CHECK(NGE_COMPONENT(8) == 0x0101 && NGE_COMPONENT(15) == 0x0180);
    CHECK(NGE_COMPONENT(16) == 0x0201 && NGE_COMPONENT(23) == 0x0280);
    for (n = 0; n < NGE_COMPONENT_MAX; n += 7) {           /* 0, 7, 14, 21 */
        nge_entity_set_component(a, NGE_COMPONENT(n));
    }
    nge_entity_set_component(a, NGE_COMPONENT(8));
    NGE_ENTITY_ADD_COMPONENT(a, NGE_COMPONENT(23));
    CHECK(comp_bits(a) == 0xA04181UL);
    CHECK(NGE_ENTITY_HAS_COMPONENT(a, NGE_COMPONENT(23)) && nge_entity_has_component(a, NGE_COMPONENT(8)));
    CHECK(!NGE_ENTITY_HAS_COMPONENT(a, NGE_COMPONENT(15)) && !nge_entity_has_component(a, NGE_COMPONENT(16)));
    nge_entity_clear_component(a, NGE_COMPONENT(0));
    NGE_ENTITY_REMOVE_COMPONENT(a, NGE_COMPONENT(21));
    CHECK(comp_bits(a) == 0x804180UL);                     /* the others in the byte kept */
    CHECK(comp_bits(b) == 0);                              /* other entities untouched */
    nge_entity_destroy(a);
    nge_entity_release(nge_entity_next_dying());
    CHECK(comp_bits(a) == 0);                              /* cleared on release */

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
    CHECK(alive_visited() == 20 - 7 + 1 && used_list_ok());

    /* Destroying others, ahead of the loop and behind it: none is visited twice */
    for (first = 0; first < 20; first++) {
        nge_entity_init();
        for (n = 0; n < 20; n++) {
            nge_entity_create();
        }
        memset(visits, 0, sizeof(visits));
        NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
            CHECK(NGE_ENTITY_IS_ALIVE(id));
            visits[id]++;
            if (id == first) {
                for (n = 0; n < 20; n += 2) {
                    nge_entity_destroy((nge_entity_id_t)n);
                }
                nge_entity_destroy(first);                  /* itself too */
            }
        }
        ok = 1;
        for (n = 0; n < 20; n++) {
            ok = ok && visits[n] <= 1;                      /* never twice */
            ok = ok && (visits[n] == 1 || n % 2 == 0);      /* odd ones, alive, always */
        }
        CHECK(ok);
    }

    /* The in-use loop visits the dying as well, once each */
    nge_entity_init();
    for (n = 0; n < 6; n++) {
        nge_entity_create();
    }
    nge_entity_destroy(1);
    nge_entity_destroy(4);
    memset(visits, 0, sizeof(visits));
    NGE_ENTITY_FOR_EACH_IN_USE(i, id) {
        visits[id]++;
        nge_entity_destroy(2);                              /* seen, whenever it goes */
    }
    CHECK(visits[0] == 1 && visits[1] == 1 && visits[2] == 1 && visits[3] == 1);
    CHECK(visits[4] == 1 && visits[5] == 1);

    /* An if/else after the loop body is not captured by the macros */
    n = 0;
    if (1)
        NGE_ENTITY_FOR_EACH_ALIVE(i, id) n++;
    else
        n = 100;
    CHECK(n == 3);

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
    CHECK(nge_entity_used_count == 0 && nge_entity_dying_count() == 0);

    /* The same with the loop macro: nothing is lost, even with continue */
    nge_entity_init();
    for (n = 0; n < 10; n++) {
        nge_entity_create();
    }
    nge_entity_destroy(0);
    n = 0;
    NGE_ENTITY_RELEASE_EACH_DYING(id) {
        n++;
        CHECK(NGE_ENTITY_STATE(id) == NGE_ENTITY_DYING);    /* not released until the body ends */
        if (id + 1 < 10) {
            nge_entity_destroy(id + 1);
        }
        if (id % 2 == 0) {
            continue;
        }
        nge_entity_set_flags(id, F_USER0);
    }
    CHECK(n == 10 && nge_entity_free_count() == NGE_ENTITY_MAX);
    CHECK(nge_entity_used_count == 0 && nge_entity_dying_count() == 0);
    NGE_ENTITY_RELEASE_EACH_DYING(id) {                     /* nothing dying: no body */
        n++;
    }
    CHECK(n == 10);

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
    TEST_EXPECT_HALT(nge_entity_set_component(NGE_ENTITY_MAX, NGE_COMPONENT(0)));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_set_component(a, 0));                          /* no component */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_set_component(a, 0x0003));                     /* two */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_has_component(a, NGE_COMPONENT(NGE_COMPONENT_MAX)));   /* no table */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_entity_clear_component(a, NGE_COMPONENT(8) | NGE_COMPONENT(16)));   /* table 3 */
    CHECK(test_halt_count == 1);
    CHECK(NGE_ENTITY_IS_ALIVE(a) && used_list_ok());

    /* ---- randomised ---- */
    random_ops();

    return test_summary("entity");
}
