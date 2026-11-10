/* game/spells: the spell definitions, known spells, learning, slots and books. */
#include "test.h"

#include <string.h>

#include "ecs/nge_entity.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/effects/effects_defs.h"
#include "game/events.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
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

/* The slot table: never falling as the character level rises (R35); spell level L first at 2L - 1 (R11) */
static int slots_table_ok(uint8_t cls)
{
    uint8_t lv;
    uint8_t sl;
    int ok = 1;

    for (lv = 0; lv < PC_LEVEL_MAX; lv++) {
        for (sl = 0; sl < SPELL_LEVEL_MAX; sl++) {
            uint8_t n = spell_slots_table[cls][lv][sl];

            ok = ok && (lv == 0 || n >= spell_slots_table[cls][lv - 1][sl]);
            ok = ok && (lv + 1 >= 2 * (sl + 1) - 1 || n == 0);     /* none before 2L - 1 */
            ok = ok && (lv + 1 != 2 * (sl + 1) - 1 || n != 0);     /* one at 2L - 1 */
        }
    }
    return ok;
}

/* Book book's spells, by the table, as a mask; 0 if one is invalid or listed twice */
static uint32_t book_mask(uint8_t book)
{
    uint32_t m = 0;
    uint8_t i;
    uint8_t sp;

    for (i = 0; i < SPELL_BOOK_MAX; i++) {
        sp = spell_books[book][i];
        if (sp == SP_NONE) {
            continue;
        }
        if (sp >= SPELL_COUNT || spell_defs[sp].level == 0 || (m & BIT(sp)) != 0) {
            return 0;
        }
        m |= BIT(sp);
    }
    return m;
}

/* The spell levels of mask's spells, as a mask of 1 << level */
static uint8_t levels_of(uint32_t mask)
{
    uint8_t m = 0;
    uint8_t sp;

    for (sp = 1; sp < SPELL_COUNT; sp++) {
        if (mask & BIT(sp)) {
            m |= (uint8_t)(1u << spell_defs[sp].level);
        }
    }
    return m;
}

int main(void)
{
    static const uint8_t none[GEN_SPELLS_MAX] = { SP_NONE };
    static const uint8_t two[GEN_SPELLS_MAX] = { SP_HASTE, SP_HOLD };
    spell_def_t def;
    char name[SPELL_NAME_MAX + 1];
    nge_entity_id_t a;
    nge_entity_id_t b;
    nge_entity_id_t mage;
    nge_entity_id_t shaman;
    uint8_t sp;
    uint8_t level;
    uint8_t kind;
    uint8_t i;
    uint8_t list[SPELL_BOOK_MAX];
    uint32_t all;
    int ok;

    test_init();
    reset();

    /* ---- the definitions: well formed ---- */
    CHECK(spell_defs[SP_NONE].name == NULL && spell_defs[SP_NONE].ef == EF_NONE);
    ok = 1;
    for (sp = 1; sp < SPELL_COUNT; sp++) {
        const spell_def_t *d = &spell_defs[sp];

        ok = ok && d->name != NULL && strlen(d->name) <= SPELL_NAME_MAX && d->level <= SPELL_LEVEL_MAX;
        ok = ok && d->shape < SPELL_SHAPE_COUNT && d->ai < SPELL_AI_COUNT;
        ok = ok && d->ef != EF_NONE && d->ef < EF_COUNT && effect_defs[d->ef].duration != EFFECT_WORN;   /* R2 */
        ok = ok && d->power > 0;                                    /* never a negative power (section 8) */
        ok = ok && (d->range != 0) == (d->shape != SPELL_SELF);     /* a range only on a line */
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
          def.ai == SPELL_AI_TARGET);
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

    /* ---- the kinds' lists: valid; the player's Magic Bolt, the shaman's three ---- */
    ok = 1;
    for (kind = 0; kind < CR_COUNT; kind++) {
        for (i = 0; i < GEN_SPELLS_MAX; i++) {
            ok = ok && gen_creatures[kind].spells[i] < SPELL_COUNT;
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

    /* ---- the slot table (D27): the Mage's rises, the Dev's is the Mage's, the Fighter has none (R36) ---- */
    CHECK(slots_table_ok(PC_CLASS_MAGE) && slots_table_ok(PC_CLASS_DEV));
    CHECK(memcmp(spell_slots_table[PC_CLASS_DEV], spell_slots_table[PC_CLASS_MAGE], sizeof(spell_slots_table[0])) == 0);
    ok = 1;
    for (level = 0; level < PC_LEVEL_MAX; level++) {
        for (i = 0; i < SPELL_LEVEL_MAX; i++) {
            ok = ok && spell_slots_table[PC_CLASS_FIGHTER][level][i] == 0;
        }
    }
    CHECK(ok);
    CHECK(spell_slots_table[PC_CLASS_MAGE][0][0] == 2 && spell_slots_table[PC_CLASS_MAGE][PC_LEVEL_MAX - 1][4] == 2);

    /* ---- spell_slots(): the table's for the player with mana, by class and level; 0 for the rest ---- */
    mage = gen_creature(CR_MAGE, AREA);
    shaman = gen_creature(CR_GOBLIN_SHAMAN, AREA);
    events();
    CHECK(PC_IS(mage) && MANA_IS(mage) && SPELLS_IS(mage) && !PC_IS(shaman) && MANA_IS(shaman) && SPELLS_IS(shaman));
    pc_set_class(mage, PC_CLASS_MAGE);
    ok = 1;
    for (level = PC_LEVEL_MIN; level <= PC_LEVEL_MAX; level++) {
        pc_level = level;
        for (i = 1; i <= SPELL_LEVEL_MAX; i++) {
            ok = ok && spell_slots(mage, i) == spell_slots_table[PC_CLASS_MAGE][level - 1][i - 1];
            ok = ok && spell_slots(shaman, i) == 0;                 /* a monster: none (R37) */
        }
    }
    CHECK(ok);
    pc_level = 3;
    CHECK(spell_slots(mage, 1) == 3 && spell_slots(mage, 2) == 1 && spell_slots(mage, 3) == 0);
    pc_set_class(mage, PC_CLASS_FIGHTER);
    CHECK(spell_slots(mage, 1) == 0);                               /* the class's row */
    pc_set_class(mage, PC_CLASS_MAGE);
    NGE_ENTITY_REMOVE_COMPONENT(mage, COMPONENT_MANA);
    CHECK(spell_slots(mage, 1) == 0);                               /* no mana: none */
    NGE_ENTITY_ADD_COMPONENT(mage, COMPONENT_MANA);

    /* ---- spell_free(): slots less the spells known of that level, never below 0 ---- */
    pc_level = 1;
    CHECK(known(mage) == BIT(SP_MAGIC_BOLT) && spell_free(mage, 1) == 1 && spell_free(mage, 2) == 0);
    spell_learn(mage, SP_CURE_WOUNDS);
    events();
    CHECK(spell_free(mage, 1) == 0);
    pc_level = 3;
    CHECK(spell_free(mage, 1) == 1 && spell_free(mage, 2) == 1);
    spell_learn(mage, SP_HOLD);
    spell_learn(mage, SP_ENFEEBLE);                                 /* past the slots, as only a test can */
    events();
    CHECK(spell_free(mage, 2) == 0 && spell_free(mage, 1) == 1);
    pc_level = 1;
    CHECK(spell_free(mage, 1) == 0 && spell_free(mage, 2) == 0);    /* more known than slots: 0 */
    CHECK(spell_free(shaman, 1) == 0);
    pc_level = PC_LEVEL_MIN;

    /* ---- spell_slots_new(): from the table, each level; never at level 1; never the Fighter ---- */
    ok = 1;
    for (level = PC_LEVEL_MIN + 1; level <= PC_LEVEL_MAX; level++) {
        int more = 0;

        for (i = 0; i < SPELL_LEVEL_MAX; i++) {
            more = more || spell_slots_table[PC_CLASS_MAGE][level - 1][i] > spell_slots_table[PC_CLASS_MAGE][level - 2][i];
        }
        ok = ok && spell_slots_new(PC_CLASS_MAGE, level) == more && more;   /* D27: every level adds one */
        ok = ok && spell_slots_new(PC_CLASS_DEV, level) == more && spell_slots_new(PC_CLASS_FIGHTER, level) == 0;
    }
    CHECK(ok && spell_slots_new(PC_CLASS_MAGE, PC_LEVEL_MIN) == 0);

    /* ---- the books: valid, no spell twice; every spell of level 1 or more in one (R42); their levels (R43) ---- */
    all = 0;
    for (i = 0; i < SPELL_BOOK_COUNT; i++) {
        CHECK(book_mask(i) != 0);
        all |= book_mask(i);
    }
    CHECK(all == BIT(SPELL_COUNT) - 2);
    CHECK(levels_of(book_mask(SPELL_BOOK_BASIC)) == ((1u << 1) | (1u << 2)));
    CHECK(levels_of(book_mask(SPELL_BOOK_ADEPT)) == ((1u << 3) | (1u << 4)));
    CHECK(levels_of(book_mask(SPELL_BOOK_MASTER)) == (1u << 5));

    /* ---- spell_book() and spell_in_book() ---- */
    memset(list, 0xAA, sizeof(list));
    CHECK(spell_book(SPELL_BOOK_BASIC, list) == 4);
    CHECK(list[0] == SP_MAGIC_BOLT && list[1] == SP_CURE_WOUNDS && list[2] == SP_HOLD && list[3] == SP_ENFEEBLE);
    ok = 1;
    for (i = 4; i < SPELL_BOOK_MAX; i++) {
        ok = ok && list[i] == SP_NONE;
    }
    CHECK(ok && spell_book(SPELL_BOOK_MASTER, list) == 2 && list[1] == SP_RENEWAL && list[2] == SP_NONE);
    CHECK(spell_in_book(SPELL_BOOK_BASIC, SP_HOLD) && !spell_in_book(SPELL_BOOK_BASIC, SP_HASTE));
    CHECK(spell_in_book(SPELL_BOOK_ADEPT, SP_HASTE) && !spell_in_book(SPELL_BOOK_ADEPT, SP_NONE));

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
    TEST_EXPECT_HALT(spell_slots(a, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_slots(a, SPELL_LEVEL_MAX + 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_slots_new(PC_CLASS_COUNT, 2));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_slots_new(PC_CLASS_MAGE, PC_LEVEL_MAX + 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_book(SPELL_BOOK_COUNT, list));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(spell_in_book(SPELL_BOOK_BASIC, SPELL_COUNT));
    CHECK(test_halt_count == 1);

    return test_summary("spell");
}
