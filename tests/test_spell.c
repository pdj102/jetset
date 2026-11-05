/* game/spells: the spell definitions, known spells and learning. */
#include "test.h"

#include <string.h>

#include "ecs/nge_entity.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/effects/effects_defs.h"
#include "game/events.h"
#include "game/gen/gen_internal.h"
#include "game/spells/spells_internal.h"
#include "game/effects/effects.h"

#define AREA  NGE_OWNER_AREA(0)

/* The events posted since the last call */
static nge_event_t seen[NGE_EVENT_QUEUE];   /* a drain holds at most a full queue */
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

/* Event i is EV_SPELL_LEARNED of sp on target */
static int learned(int i, nge_entity_id_t target, uint8_t sp)
{
    return i < seen_count && seen[i].id == EV_SPELL_LEARNED && seen[i].target == target &&
           seen[i].source == NGE_EVENT_NONE && seen[i].p1 == sp;
}

/* Every entity free, the renderer and clock afresh */
static void reset(void)
{
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_effect_init(effects_hook);
    nge_event_init();
}

/* The spells id knows, as a mask of 1 << sp */
static uint32_t known(nge_entity_id_t id)
{
    uint32_t m = 0;
    uint8_t sp;

    for (sp = 0; sp < SPELL_COUNT; sp++) {
        if (spell_knows(id, sp)) {
            m |= (uint32_t)1 << sp;
        }
    }
    return m;
}

#define BIT(sp)  ((uint32_t)1 << (sp))

int main(void)
{
    static const uint8_t none[GEN_SPELLS_MAX] = { SP_NONE };
    static const uint8_t two[GEN_SPELLS_MAX] = { SP_HASTE, SP_HOLD };
    spell_def_t def;
    char name[SPELL_NAME_MAX + 1];
    nge_entity_id_t a;
    nge_entity_id_t b;
    uint8_t sp;
    uint8_t level;
    uint8_t kind;
    uint8_t i;
    int ok;

    test_init();
    reset();

    /* ---- the definitions: well formed ---- */
    CHECK(spell_defs[SP_NONE].name == NULL && spell_defs[SP_NONE].ef == EF_NONE);
    ok = 1;
    for (sp = 1; sp < SPELL_COUNT; sp++) {
        const spell_def_t *d = &spell_defs[sp];

        ok = ok && d->name != NULL && strlen(d->name) <= SPELL_NAME_MAX && d->level <= SPELL_LEVEL_MAX;
        ok = ok && d->shape < SPELL_SHAPE_COUNT && d->ai < SPELL_AI_COUNT && d->learn <= PC_LEVEL_MAX;
        ok = ok && d->ef != EF_NONE && d->ef < EF_COUNT && effect_defs[d->ef].duration != EFFECT_WORN;   /* R2 */
        ok = ok && d->power > 0;                                    /* never a negative power (section 8) */
        ok = ok && (d->range != 0) == (d->shape != SPELL_SELF);     /* a range only on a line */
        ok = ok && (d->learn == 0 || d->learn >= 2 * d->level - 1); /* R11: the pace of classic B/X */
    }
    CHECK(ok);
    CHECK(SPELL_COUNT <= SPELL_SET_BYTES * 8);

    /* ---- cost: by the level only, rising with every level above 0 (R5) ---- */
    CHECK(spell_cost(0) == 0 && spell_cost(1) == 1 && spell_cost(SPELL_LEVEL_MAX) == 9);
    ok = 1;
    for (level = 1; level <= SPELL_LEVEL_MAX; level++) {
        ok = ok && spell_cost(level) > spell_cost((uint8_t)(level - 1));
    }
    CHECK(ok);

    /* ---- get and name: copies ---- */
    spell_get(SP_FIRE_BOLT, &def);
    CHECK(def.level == 3 && def.shape == SPELL_BOLT && def.ef == EF_BOLT_FIRE && def.power == 8 && def.range == 8 &&
          def.learn == 5 && def.ai == SPELL_AI_TARGET);
    spell_name(SP_CURE_WOUNDS, name, sizeof(name));
    CHECK_STR(name, "Cure Wounds");
    spell_name(SP_CURE_WOUNDS, name, 5);
    CHECK_STR(name, "Cure");
    spell_name(SP_NONE, name, sizeof(name));
    CHECK_STR(name, "");

    /* ---- add: the list and no others, posting nothing ---- */
    a = nge_entity_create();
    b = nge_entity_create();
    CHECK(!SPELLS_IS(a) && known(a) == 0 && !spell_knows(a, SP_MAGIC_BOLT));
    spell_add(a, two, GEN_SPELLS_MAX);
    CHECK(SPELLS_IS(a) && known(a) == (BIT(SP_HASTE) | BIT(SP_HOLD)) && events() == 0);
    CHECK(!spell_knows(a, SP_NONE) && known(b) == 0);
    spell_add(b, none, GEN_SPELLS_MAX);                              /* a caster knowing nothing */
    CHECK(SPELLS_IS(b) && known(b) == 0);

    /* ---- learn: the event; a second learn changes nothing and posts nothing (R8) ---- */
    spell_learn(b, SP_FIRE_BOLT);
    CHECK(known(b) == BIT(SP_FIRE_BOLT) && events() == 1 && learned(0, b, SP_FIRE_BOLT));
    spell_learn(b, SP_FIRE_BOLT);
    CHECK(known(b) == BIT(SP_FIRE_BOLT) && events() == 0);
    /* Every spell, so every bit used: bit 7 of byte 0 (SP_STONE_SKIN) and bits of byte 1 */
    for (sp = 1; sp < SPELL_COUNT; sp++) {
        spell_learn(b, sp);
    }
    CHECK(known(b) == (BIT(SPELL_COUNT) - 2) && events() == SPELL_COUNT - 2);
    CHECK(spell_known[b][0] == 0xFE && spell_known[b][1] == 0x07 && spell_known[b][2] == 0 && spell_known[b][3] == 0);
    CHECK(known(a) == (BIT(SP_HASTE) | BIT(SP_HOLD)));               /* another caster's set untouched */

    /* ---- learn by level: each row with that level; 0 never ---- */
    ok = 1;
    for (level = 1; level <= PC_LEVEL_MAX; level++) {
        uint32_t want = 0;

        spell_add(a, none, GEN_SPELLS_MAX);
        for (sp = 1; sp < SPELL_COUNT; sp++) {
            if (spell_defs[sp].learn == level) {
                want |= BIT(sp);
            }
        }
        spell_learn_level(a, level);
        ok = ok && want != 0 && known(a) == want && events() == 1 && learned(0, a, seen[0].p1) &&
             want == BIT(seen[0].p1);                               /* one spell a level, for now */
    }
    CHECK(ok);
    spell_add(a, none, GEN_SPELLS_MAX);
    spell_learn_level(a, 0);
    CHECK(known(a) == 0 && events() == 0);

    /* ---- next: wrapping round to itself, filtered by when the AI casts it; none known ---- */
    spell_add(a, none, GEN_SPELLS_MAX);
    CHECK(spell_next(a, SP_NONE, SPELL_AI_ANY) == SP_NONE && spell_next(a, SP_HOLD, SPELL_AI_ANY) == SP_NONE);
    spell_learn(a, SP_MAGIC_BOLT);
    spell_learn(a, SP_CURE_WOUNDS);
    spell_learn(a, SP_HASTE);
    events();
    CHECK(spell_next(a, SP_NONE, SPELL_AI_ANY) == SP_MAGIC_BOLT);
    CHECK(spell_next(a, SP_MAGIC_BOLT, SPELL_AI_ANY) == SP_CURE_WOUNDS);
    CHECK(spell_next(a, SP_CURE_WOUNDS, SPELL_AI_ANY) == SP_HASTE);
    CHECK(spell_next(a, SP_HASTE, SPELL_AI_ANY) == SP_MAGIC_BOLT);                 /* wraps past SP_NONE */
    CHECK(spell_next(a, SP_RENEWAL, SPELL_AI_ANY) == SP_MAGIC_BOLT);
    CHECK(spell_next(a, SP_MAGIC_BOLT, SPELL_AI_TARGET) == SP_MAGIC_BOLT);         /* round to itself */
    CHECK(spell_next(a, SP_HASTE, SPELL_AI_HURT) == SP_CURE_WOUNDS);
    CHECK(spell_next(a, SP_NONE, SPELL_AI_NEVER) == SP_HASTE);

    /* ---- an entity ID reused: a new creature has no old spells ---- */
    reset();
    a = nge_entity_create();
    spell_add(a, two, GEN_SPELLS_MAX);
    nge_entity_destroy(a);
    NGE_ENTITY_RELEASE_EACH_DYING(b) {
    }
    b = nge_entity_create();
    CHECK(b == a && !SPELLS_IS(b) && known(b) == 0);
    spell_add(b, none, GEN_SPELLS_MAX);
    CHECK(known(b) == 0);

    /* ---- the kinds' lists: valid; the player's has every spell of learn 1 (R10), the shaman's three ---- */
    ok = 1;
    for (kind = 0; kind < CR_COUNT; kind++) {
        for (i = 0; i < GEN_SPELLS_MAX; i++) {
            ok = ok && gen_creatures[kind].spells[i] < SPELL_COUNT;
        }
    }
    CHECK(ok);
    ok = 1;
    for (sp = 1; sp < SPELL_COUNT; sp++) {
        if (spell_defs[sp].learn == 1) {
            ok = ok && memchr(gen_creatures[CR_PLAYER].spells, sp, GEN_SPELLS_MAX) != NULL;
        }
    }
    CHECK(ok);
    reset();
    a = gen_creature(CR_PLAYER, AREA);
    events();
    CHECK(SPELLS_IS(a) && known(a) == BIT(SP_MAGIC_BOLT));
    CHECK(seen_count > 0);
    ok = 1;
    for (i = 0; i < seen_count; i++) {
        ok = ok && seen[i].id != EV_SPELL_LEARNED;                   /* a new game learns nothing */
    }
    CHECK(ok);
    b = gen_creature(CR_GOBLIN_SHAMAN, AREA);
    CHECK(SPELLS_IS(b) && known(b) == (BIT(SP_MAGIC_BOLT) | BIT(SP_CURE_WOUNDS) | BIT(SP_ENFEEBLE)));
    b = gen_creature(CR_GOBLIN, AREA);
    CHECK(!SPELLS_IS(b) && !spell_knows(b, SP_MAGIC_BOLT));

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(spell_learn(b, SP_MAGIC_BOLT));                 /* not a caster */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_learn(a, SP_NONE));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_learn(a, SPELL_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_knows(a, SPELL_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_next(b, SP_NONE, SPELL_AI_ANY));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_cost(SPELL_LEVEL_MAX + 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_get(SPELL_COUNT, &def));
    CHECK(test_halt_count == 1);

    return test_summary("spell");
}
