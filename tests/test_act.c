/* game/actions: the action pattern, and the move action. */
#include "test.h"

#include <stdlib.h>

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "map/nge_map.h"

#include "data/tile_data.h"

#include "game/actions/act.h"
#include "game/actions/act_internal.h"
#include "game/components/attack.h"
#include "game/attrs/attrs.h"
#include "game/components/health.h"
#include "game/components/inventory_internal.h"
#include "game/components/feature.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/effects/effects_defs.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/quest/quest.h"
#include "game/spells/spells.h"

#define AREA   NGE_OWNER_AREA(0)
#define FLOOR  '.'
#define WALL   '#'

static const nge_rect_t window = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 19 };   /* a view of 10 x 10 tiles */

/* The player, for the AI linked with the actions (game/actors.c isn't linked) */
nge_entity_id_t actor_player = NGE_ENTITY_INVALID;

/* The screen's map view, for the level module (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

/* The events posted since the last call */
static nge_event_t seen[NGE_EVENT_QUEUE];   /* a drain holds at most a full queue */
static int seen_count;

static void record(const nge_event_t *e)
{
    seen[seen_count++] = *e;
}

static int events(void)
{
    seen_count = 0;
    nge_event_process(record);
    return seen_count;
}

static int is(int i, uint8_t target, uint8_t id, uint8_t source)
{
    return i < seen_count && seen[i].target == target && seen[i].id == id && seen[i].source == source;
}

/* A creature of kind in the area at (x, y) */
static nge_entity_id_t creature(uint8_t kind, uint8_t x, uint8_t y)
{
    nge_entity_id_t id = gen_creature(kind, AREA);

    nge_location_set(id, x, y);
    return id;
}

static int at(nge_entity_id_t id, uint8_t x, uint8_t y)
{
    return NGE_LOCATION_X(id) == x && NGE_LOCATION_Y(id) == y;
}

/* A move context: actor, direction, the outputs set to junk */
static act_t move(nge_entity_id_t actor, int8_t dx, int8_t dy)
{
    act_t a = { 0 };

    a.actor = actor;
    a.dx = dx;
    a.dy = dy;
    a.target = 0x77;
    a.reason = 0x77;
    a.used = 0x77;
    return a;
}

/* A context for actor with no inputs, the outputs set to junk */
static act_t ctx(nge_entity_id_t actor)
{
    act_t a = move(actor, 1, 0);

    a.dx = 0;
    a.target = NGE_ENTITY_INVALID;
    a.item = NGE_ENTITY_INVALID;
    return a;
}

/* A seed whose first rand() gives the d20 roll r: act_melee()'s first draw is its roll */
static unsigned int seed_for_roll(int r)
{
    unsigned int s;

    for (s = 1; s < 100000; s++) {
        srand(s);
        if (1 + rand() % 20 == r) {
            return s;
        }
    }
    return 0;
}

/* act_line() from actor towards (x, y) in mode how: its reason, and the target in line_target */
static nge_entity_id_t line_target;

static uint8_t line_how(nge_entity_id_t actor, uint8_t x, uint8_t y, uint8_t range, uint8_t how)
{
    act_t a = move(actor, 0, 0);
    uint8_t reason;

    a.x = x;
    a.y = y;
    reason = act_line(&a, range, how);
    line_target = a.target;
    return reason;
}

/* act_line() as a bolt */
static uint8_t line(nge_entity_id_t actor, uint8_t x, uint8_t y, uint8_t range)
{
    return line_how(actor, x, y, range, ACT_LINE_BOLT);
}

/* act_fire() from actor at (x, y): its result, and the context in last */
static act_t last;

static uint8_t fire(nge_entity_id_t actor, uint8_t x, uint8_t y, uint8_t mode)
{
    uint8_t result;

    last = move(actor, 0, 0);
    last.x = x;
    last.y = y;
    last.item = NGE_ENTITY_INVALID;
    result = act_fire(&last, mode);
    return result;
}

/* act_aim() from actor with item at (x, y): its result, and the context in last */
static uint8_t aim(nge_entity_id_t actor, nge_entity_id_t item, uint8_t x, uint8_t y, uint8_t mode)
{
    last = move(actor, 0, 0);
    last.item = item;
    last.x = x;
    last.y = y;
    return act_aim(&last, mode);
}

/* act_cast() of spell sp by actor at (x, y): its result, and the context in last */
static uint8_t cast(nge_entity_id_t actor, uint8_t sp, uint8_t x, uint8_t y, uint8_t mode)
{
    last = move(actor, 0, 0);
    last.spell = sp;
    last.x = x;
    last.y = y;
    return act_cast(&last, mode);
}

/* A stack of this kind on the floor of the area at (x, y) */
static nge_entity_id_t floor_stack(uint8_t kind, uint8_t count, uint8_t x, uint8_t y)
{
    nge_entity_id_t id = gen_item(kind, AREA);

    item_count[id] = count;
    nge_location_set(id, x, y);
    return id;
}

/* An item of this kind made for holder and put straight in slot */
static nge_entity_id_t equipped(uint8_t kind, nge_entity_id_t holder, uint8_t slot)
{
    nge_entity_id_t id = gen_item(kind, holder);

    item_slot[id] = slot;
    return id;
}

static int stowed(nge_entity_id_t id, nge_entity_id_t holder)
{
    return NGE_OWNER_OF(id) == holder && !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION);
}

/* act_exchange() by actor of item into slot (SLOT_NONE for its choice): its result, and the context in last */
static uint8_t exchange(nge_entity_id_t actor, nge_entity_id_t item, uint8_t slot, uint8_t mode)
{
    last = ctx(actor);
    last.item = item;
    last.slot = slot;
    return act_exchange(&last, mode);
}

/* Where event id about target is among those seen, or -1 */
static int seen_at(nge_entity_id_t target, uint8_t id)
{
    int i;

    for (i = 0; i < seen_count; i++) {
        if (seen[i].target == target && seen[i].id == id) {
            return i;
        }
    }
    return -1;
}

/* Equip item on actor, by the action */
static void wear(nge_entity_id_t actor, nge_entity_id_t item, uint8_t slot)
{
    act_t a = ctx(actor);

    a.item = item;
    a.slot = slot;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS);
}

/* act_learn() of spell sp from book by actor: its result, and the context in *a */
static uint8_t learn(act_t *a, nge_entity_id_t actor, nge_entity_id_t book, uint8_t sp, uint8_t mode)
{
    *a = ctx(actor);
    a->item = book;
    a->spell = sp;
    return act_learn(a, mode);
}

/* Refused with reason, the turn not used, nothing posted */
static int refused(const act_t *a, uint8_t result, uint8_t reason)
{
    return result == ACT_FAIL && a->reason == reason && a->used == 0 && events() == 0;
}

/* Learn (docs/spells.md 10.5.4): a Mage at level 1 with a basic spellbook; a Fighter; a shaman */
static void test_learn(void)
{
    nge_entity_id_t fighter;
    nge_entity_id_t mage;
    nge_entity_id_t shaman;
    nge_entity_id_t book;
    nge_entity_id_t potion;
    nge_entity_id_t floor;
    nge_entity_id_t adept;
    act_t a;
    int r1;

    events();
    nge_entity_init();
    fighter = creature(CR_FIGHTER, 2, 2);
    mage = creature(CR_MAGE, 3, 3);   /* the last PC made: level 1, its class set below */
    shaman = creature(CR_GOBLIN_SHAMAN, 5, 5);
    pc_set_class(mage, PC_CLASS_MAGE);
    book = gen_item(IT_BOOK_BASIC, mage);
    adept = gen_item(IT_BOOK_ADEPT, mage);
    potion = gen_item(IT_HEAL_POTION, mage);
    floor = gen_item(IT_BOOK_BASIC, AREA);
    nge_location_set(floor, 3, 3);
    events();
    CHECK(pc_level == 1 && spell_knows(mage, SP_MAGIC_BOLT) && !MANA_IS(fighter) && !PC_IS(shaman));

    /* ---- each refusal, in order: nothing changed, nothing posted, the turn not used ---- */
    CHECK(refused(&a, learn(&a, mage, floor, SP_CURE_WOUNDS, ACT_DO), ACT_R_NOT_CARRIED));   /* on the floor (R51) */
    CHECK(refused(&a, learn(&a, mage, potion, SP_CURE_WOUNDS, ACT_DO), ACT_R_NOT_BOOK));
    nge_owner_set(book, fighter);
    CHECK(refused(&a, learn(&a, mage, book, SP_CURE_WOUNDS, ACT_DO), ACT_R_NOT_CARRIED));    /* another's */
    CHECK(refused(&a, learn(&a, fighter, book, SP_CURE_WOUNDS, ACT_DO), ACT_R_CANT_LEARN));  /* no mana */
    CHECK(refused(&a, learn(&a, fighter, book, SP_NONE, ACT_CHECK), ACT_R_CANT_LEARN));      /* the book alone */
    nge_owner_set(book, shaman);
    CHECK(refused(&a, learn(&a, shaman, book, SP_CURE_WOUNDS, ACT_DO), ACT_R_CANT_LEARN));   /* a monster (R37) */
    nge_owner_set(book, mage);
    CHECK(refused(&a, learn(&a, mage, book, SP_HASTE, ACT_DO), ACT_R_NOT_IN_BOOK));
    CHECK(refused(&a, learn(&a, mage, book, SP_MAGIC_BOLT, ACT_DO), ACT_R_KNOWN));
    CHECK(refused(&a, learn(&a, mage, book, SP_HOLD, ACT_DO), ACT_R_BEYOND));                /* no level 2 slot yet */
    CHECK(refused(&a, learn(&a, mage, adept, SP_FIRE_BOLT, ACT_DO), ACT_R_BEYOND));
    CHECK(!spell_knows(mage, SP_CURE_WOUNDS) && !spell_knows(mage, SP_HOLD));

    /* ---- the book alone, in check mode: success (D24); in do mode, an assert ---- */
    CHECK(learn(&a, mage, book, SP_NONE, ACT_CHECK) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 0);
    TEST_EXPECT_HALT(learn(&a, mage, book, SP_NONE, ACT_DO));
    CHECK(test_halt_count == 1);

    /* ---- check mode: the answer, and nothing changes, not even rand() ---- */
    srand(5);
    r1 = rand();
    srand(5);
    CHECK(learn(&a, mage, book, SP_CURE_WOUNDS, ACT_CHECK) == ACT_SUCCESS && a.used == 0);
    CHECK(rand() == r1 && events() == 0 && !spell_knows(mage, SP_CURE_WOUNDS));

    /* ---- a learn: the spell known, its event, the turn used; the book kept (R50) ---- */
    CHECK(spell_free(mage, 1) == 1);
    CHECK(learn(&a, mage, book, SP_CURE_WOUNDS, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(spell_knows(mage, SP_CURE_WOUNDS) && events() == 1 && is(0, mage, EV_SPELL_LEARNED, NGE_EVENT_NONE) &&
          seen[0].p1 == SP_CURE_WOUNDS);
    CHECK(NGE_ENTITY_IS_ALIVE(book) && NGE_OWNER_OF(book) == mage && spell_free(mage, 1) == 0);

    /* ---- level 3: one level 2 slot; the last filled, then the next refused ---- */
    pc_level = 3;
    a = ctx(mage);
    a.item = book;
    a.spell = SP_HOLD;
    CHECK(act_do(ACT_LEARN, &a, ACT_DO) == ACT_SUCCESS);                                      /* by number */
    CHECK(events() == 1 && spell_knows(mage, SP_HOLD) && spell_free(mage, 2) == 0);
    CHECK(refused(&a, learn(&a, mage, book, SP_ENFEEBLE, ACT_DO), ACT_R_SLOTS_FULL));
    CHECK(refused(&a, learn(&a, mage, book, SP_ENFEEBLE, ACT_CHECK), ACT_R_SLOTS_FULL));
    pc_level = 1;
}

/* ---- exchange (docs/action_exchange.md): a bandit, capacity 6 ---- */
/* Operate (docs/features.md 4.6): a bandit at (7, 7), a healing shrine east, a foul fountain south, a door west */
static void test_operate(void)
{
    nge_entity_id_t h;
    nge_entity_id_t shrine;
    nge_entity_id_t foul;
    nge_entity_id_t door;
    nge_entity_id_t far;
    nge_entity_id_t hidden;
    act_t a;
    int healed;

    /* Run last, with every entity free again: the tests before it use them all */
    events();
    nge_entity_init();
    h = gen_creature(CR_BANDIT, AREA);
    shrine = gen_feature(FT_SHRINE_HEALING, AREA);
    foul = gen_feature(FT_FOUNTAIN_FOUL, AREA);
    door = gen_feature(FT_DOOR, AREA);
    far = gen_feature(FT_ALTAR_MIGHT, AREA);
    hidden = gen_feature(FT_SHRINE_CLEANSING, AREA);
    nge_location_set(h, 7, 7);
    feature_place(shrine, 8, 7);
    feature_place(foul, 7, 8);
    feature_place(door, 6, 7);
    feature_place(far, 9, 9);                                       /* two moves away */
    feature_state[hidden] = FEAT_HIDDEN;
    feature_place(hidden, 6, 6);
    health_hp[h] = 1;
    healed = health_hp_max[h] < 16 ? health_hp_max[h] : 16;        /* 15 at power 15, up to its maximum */
    events();

    /* Refused, no turn: nothing there, a hidden one (unknown is not there), a door, too far */
    a = ctx(h);
    a.dy = -1;
    CHECK(act_operate(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_THERE && a.used == 0);
    a = ctx(h);
    a.dx = -1;
    a.dy = -1;
    CHECK(act_operate(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_THERE && a.used == 0);
    a = ctx(h);
    a.dx = -1;
    CHECK(act_operate(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_OPERATE && a.target == door && a.used == 0);
    a = ctx(h);
    a.target = far;
    CHECK(act_operate(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_OUT_OF_RANGE && a.used == 0);
    CHECK(events() == 0);

    /* Check: possible, its effect said, nothing changes */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_operate(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.target == shrine && a.effect == EF_HEALING);
    CHECK(health_hp[h] == 1 && !FEATURE_IS_SPENT(shrine) && events() == 0);

    /* Done: EV_OPERATED first, then the healing; spent, drawn spent, still blocking */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_operate(&a, ACT_DO) == ACT_SUCCESS && a.used == 1 && health_hp[h] == healed);
    CHECK(events() >= 2 && is(0, shrine, EV_OPERATED, h) && seen[0].p1 == EF_HEALING && seen_at(h, EV_HEALED) > 0);
    CHECK(FEATURE_IS_SPENT(shrine) && NGE_MAP_TILE(8, 7).tile_id == LOOK_SHRINE_SPENT && NGE_BLOCK_OF(shrine) == NGE_BLOCK_SOLID);

    /* Again: spent, refused, no turn */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_operate(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SPENT && a.used == 0 && events() == 0);

    /* Held fast, it still operates (it doesn't leave its tile): the foul fountain, by target */
    effect_apply(EF_HOLD, 1, h, NGE_ENTITY_INVALID);
    CHECK(STATUS_HAS(h, STATUS_ROOTED));
    events();
    a = ctx(h);
    a.target = foul;
    CHECK(act_do(ACT_OPERATE, &a, ACT_DO) == ACT_SUCCESS && a.effect == EF_POISON && FEATURE_IS_SPENT(foul));
    CHECK(events() >= 1 && is(0, foul, EV_OPERATED, h) && seen[0].p1 == EF_POISON &&
          NGE_MAP_TILE(7, 8).tile_id == LOOK_FOUNTAIN_DRY);
}

/* Containers (docs/features.md 4.9): a bandit at (2, 2), a locked chest east of it holding two items */
static void test_container(void)
{
    nge_entity_id_t h;
    nge_entity_id_t chest;
    nge_entity_id_t statue;
    nge_entity_id_t i1;
    nge_entity_id_t i2;
    nge_entity_id_t s1;
    nge_entity_id_t key;
    act_t a;
    int n;
    int i;

    events();
    nge_entity_init();
    h = gen_creature(CR_BANDIT, AREA);
    nge_location_set(h, 2, 2);
    chest = gen_feature(FT_CHEST, AREA);
    feature_place(chest, 3, 2);
    i1 = gen_item(IT_DAGGER, chest);                                /* held off the map, as a pack's */
    i2 = gen_item(IT_SWORD, chest);
    feature_lock(chest, KEY_IRON);
    key = gen_item(IT_KEY_IRON, h);
    events();

    /* Locked: it won't open; its key unlocks it, as a door's */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_open(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_LOCKED && a.used == 0);
    a = ctx(h);
    a.dx = 1;
    a.item = key;
    CHECK(act_unlock(&a, ACT_DO) == ACT_SUCCESS && !FEATURE_IS_LOCKED(chest));
    events();

    /* Opened: EV_OPENED, and its reaction spills what it holds on its tile, open and drawn open */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_open(&a, ACT_DO) == ACT_SUCCESS && FEATURE_IS_OPEN(chest));
    CHECK(NGE_OWNER_OF(i1) == chest && !NGE_ENTITY_HAS_COMPONENT(i1, NGE_COMPONENT_LOCATION));   /* not until the event */
    n = events();
    for (i = 0; i < n; i++) {
        feature_on_event(&seen[i]);
    }
    CHECK(NGE_OWNER_OF(i1) == AREA && at(i1, 3, 2) && NGE_OWNER_OF(i2) == AREA && at(i2, 3, 2));
    CHECK(NGE_MAP_TILE(3, 2).tile_id == LOOK_CHEST_OPEN && NGE_BLOCK_OF(chest) == NGE_BLOCK_NONE);
    /* Spilled: it doesn't close again */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_OPEN && a.used == 0);

    /* A cracked statue: broken, its item dropped on its tile by the clean-up */
    statue = gen_feature(FT_CRACKED_STATUE, AREA);
    feature_place(statue, 2, 3);
    s1 = gen_item(IT_DAGGER, statue);
    health_damage(statue, h, 100, DMG_BLUNT);
    CHECK(NGE_ENTITY_STATE(statue) == NGE_ENTITY_DYING && NGE_OWNER_OF(s1) == statue);
    events();
    level_release_dying();
    CHECK(NGE_ENTITY_STATE(statue) == NGE_ENTITY_FREE && NGE_OWNER_OF(s1) == AREA && at(s1, 2, 3));
}

/* Traps (docs/features.md 4.8): act_spring(), and the reaction to a creature moving onto one */
static void test_traps(void)
{
    nge_entity_id_t h;
    nge_entity_id_t bat;
    nge_entity_id_t dart;
    nge_entity_id_t rune;
    nge_entity_id_t pit;
    act_t a;
    int hp;
    int n;
    int i;

    uint8_t x;
    uint8_t y;

    events();
    nge_entity_init();
    for (y = 0; y < 10; y++) {                                      /* the earlier tests' tiles gone: all floor */
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    nge_map_set_block(LOOK_TRAP, NGE_BLOCK_NONE);                  /* as the level sets it: a found trap's tile is walked on */
    h = gen_creature(CR_BANDIT, AREA);
    nge_location_set(h, 2, 2);
    effects_set_base(h, ATTR_HP_MAX, 200);
    health_hp[h] = 200;
    dart = gen_feature(FT_DART_TRAP, AREA);
    feature_place(dart, 3, 6);
    rune = gen_feature(FT_FIRE_RUNE, AREA);
    feature_place(rune, 4, 6);
    events();

    /* Hidden, as its kind starts: drawn as the floor, blocking nothing */
    CHECK(FEATURE_IS_HIDDEN(dart) && NGE_MAP_TILE(3, 6).tile_id == FLOOR && NGE_BLOCK_OF(dart) == NGE_BLOCK_NONE);

    /* A 1: it goes off and is found, then misses; no hit hook */
    srand(seed_for_roll(1));
    act_spring(dart, h);
    n = events();
    CHECK(n >= 3 && is(0, dart, EV_TRIGGERED, h) && is(1, dart, EV_FOUND, NGE_EVENT_NONE) && is(2, h, EV_MISSED, dart));
    CHECK(!FEATURE_IS_HIDDEN(dart) && NGE_MAP_TILE(3, 6).tile_id == LOOK_TRAP && health_hp[h] == 200 &&
          seen_at(h, EV_EFFECT_STARTED) < 0);
    /* A 20: it hits, 1d4 piercing, then its hit hook (poison) on what it left alive; found already */
    srand(seed_for_roll(20));
    act_spring(dart, h);
    n = events();
    CHECK(is(0, dart, EV_TRIGGERED, h) && seen_at(dart, EV_FOUND) < 0 && is(1, h, EV_HIT, dart));
    CHECK(health_hp[h] >= 196 && health_hp[h] <= 199 && seen_at(h, EV_EFFECT_STARTED) > 0);

    /* No attack: it always goes off, its effect at its power (6 fire) */
    hp = health_hp[h];
    act_spring(rune, h);
    n = events();
    CHECK(is(0, rune, EV_TRIGGERED, h) && health_hp[h] == hp - 6 && seen_at(h, EV_DAMAGED) > 0);

    /* Moved onto: the reaction to EV_STOOD_ON sets it off at a walker... */
    pit = gen_feature(FT_PIT, AREA);
    feature_place(pit, 2, 3);                                       /* south of the bandit */
    events();
    srand(seed_for_roll(20));
    a = move(h, 0, 1);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(h, 2, 3));
    n = events();
    for (i = 0; i < n; i++) {
        feature_on_event(&seen[i]);
    }
    n = events();
    CHECK(is(0, pit, EV_TRIGGERED, h) && STATUS_HAS(h, STATUS_ROOTED));   /* in the pit: held */
    effect_cure(h, STATUS_ROOTED);
    events();
    /* ...and not at one that flies (it passes holes) */
    bat = gen_creature(CR_BAT, AREA);
    nge_location_set(bat, 1, 4);
    nge_block_add(bat, NGE_BLOCK_OCCUPIED, NGE_BLOCK_NO_FLOOR);
    a = move(bat, 1, -1);
    CHECK(act_move(&a, ACT_DO) == ACT_FAIL);                        /* the bandit is there: step aside first */
    nge_location_set(h, 5, 5);
    events();
    a = move(bat, 1, -1);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(bat, 2, 3));
    n = events();
    for (i = 0; i < n; i++) {
        feature_on_event(&seen[i]);
    }
    CHECK(events() == 0);
    /* ...nor at one levitating (STATUS_FLYING), which the fire rune's tile shows (docs/effects_extensions.md R19) */
    effect_apply(EF_LEVITATE, 1, h, NGE_ENTITY_INVALID);
    nge_location_set(h, 4, 5);
    events();
    hp = health_hp[h];
    a = move(h, 0, 1);
    CHECK(STATUS_HAS(h, STATUS_FLYING) && act_move(&a, ACT_DO) == ACT_SUCCESS && at(h, 4, 6));
    n = events();
    for (i = 0; i < n; i++) {
        feature_on_event(&seen[i]);
    }
    CHECK(events() == 0 && health_hp[h] == hp);
    /* Not a condition: curing them all leaves it; worn boots of levitation do the same */
    effect_apply(EF_CURE_ALL, 1, h, NGE_ENTITY_INVALID);
    CHECK(STATUS_HAS(h, STATUS_FLYING));
    effect_cure(h, STATUS_FLYING);
    CHECK(!STATUS_HAS(h, STATUS_FLYING));
    wear(h, gen_item(IT_BOOTS_LEVITATE, h), SLOT_FEET);
    events();
    CHECK(STATUS_HAS(h, STATUS_FLYING));
    nge_location_set(h, 4, 5);
    a = move(h, 0, 1);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS);
    n = events();
    for (i = 0; i < n; i++) {
        feature_on_event(&seen[i]);
    }
    CHECK(events() == 0 && health_hp[h] == hp);
}

/* Search and disarm (docs/features.md 4.6): a bandit at (2, 2), Wisdom and Dexterity 10 (no modifier) */
static void test_search_disarm(void)
{
    nge_entity_id_t h;
    nge_entity_id_t dart;
    nge_entity_id_t pit;
    nge_entity_id_t far;
    nge_entity_id_t door;
    act_t a;
    int r1;
    uint8_t x;
    uint8_t y;

    events();
    nge_entity_init();
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    h = gen_creature(CR_BANDIT, AREA);
    nge_location_set(h, 2, 2);
    effects_set_base(h, ATTR_ABILITY(ABIL_WIS), 10);               /* bases: the scores are derived from them */
    effects_set_base(h, ATTR_ABILITY(ABIL_DEX), 10);
    effects_set_base(h, ATTR_HP_MAX, 200);
    health_hp[h] = 200;
    dart = gen_feature(FT_DART_TRAP, AREA);                         /* hidden, DC 12 */
    feature_place(dart, 3, 2);
    far = gen_feature(FT_DART_TRAP, AREA);                          /* hidden, out of reach */
    feature_place(far, 5, 5);
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 2, 1);
    events();

    /* Disarming what isn't known: nothing there; a door: not a trap */
    a = ctx(h);
    a.dx = 1;
    CHECK(act_disarm(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_THERE && a.used == 0);
    a = ctx(h);
    a.dy = -1;
    CHECK(act_disarm(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_DISARM && a.target == door && a.used == 0);

    /* Search, check mode: possible, no roll, nothing found */
    srand(9);
    r1 = rand();
    srand(9);
    a = ctx(h);
    CHECK(act_search(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && rand() == r1 && FEATURE_IS_HIDDEN(dart));
    /* A 1: nothing found, still a success, the turn used, EV_SEARCHED with 0 */
    srand(seed_for_roll(1));
    a = ctx(h);
    CHECK(act_do(ACT_SEARCH, &a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.amount == 0 && FEATURE_IS_HIDDEN(dart));
    CHECK(events() == 1 && is(0, h, EV_SEARCHED, NGE_EVENT_NONE) && seen[0].p1 == 0);
    /* A 20: the one within a tile found, drawn; the far one not */
    srand(seed_for_roll(20));
    a = ctx(h);
    CHECK(act_search(&a, ACT_DO) == ACT_SUCCESS && a.amount == 1 && !FEATURE_IS_HIDDEN(dart) && FEATURE_IS_HIDDEN(far));
    CHECK(NGE_MAP_TILE(3, 2).tile_id == LOOK_TRAP && events() == 2 && is(0, dart, EV_FOUND, h) &&
          is(1, h, EV_SEARCHED, NGE_EVENT_NONE) && seen[1].p1 == 1);
    /* The roll: the DC reached with Wisdom's modifier */
    CHECK(!feature_found_by(h, far, 11) && feature_found_by(h, far, 12));
    effects_set_base(h, ATTR_ABILITY(ABIL_WIS), 14);               /* +2 */
    CHECK(feature_found_by(h, far, 10));
    effects_set_base(h, ATTR_ABILITY(ABIL_WIS), 10);
    events();

    /* Disarm, check mode: possible, no roll */
    srand(9);
    a = ctx(h);
    a.dx = 1;
    CHECK(act_disarm(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.target == dart && rand() == r1);
    /* Short by less than 5 (an 8 against 12): it holds, the turn used */
    srand(seed_for_roll(8));
    a = ctx(h);
    a.dx = 1;
    CHECK(act_disarm(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_DISARM_FAILED && a.used == 1 &&
          NGE_ENTITY_IS_ALIVE(dart) && events() == 0);
    /* Short by 5 (a 7): it goes off at the disarmer, next to it */
    srand(seed_for_roll(7));
    a = ctx(h);
    a.dx = 1;
    CHECK(act_disarm(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SET_OFF && a.used == 1 && NGE_ENTITY_IS_ALIVE(dart));
    CHECK(events() >= 2 && is(0, dart, EV_TRIGGERED, h));
    /* Held fast, it can still disarm; a 12 does it: EV_DISARMED, the trap dying */
    effect_apply(EF_HOLD, 1, h, NGE_ENTITY_INVALID);
    events();
    srand(seed_for_roll(12));
    a = ctx(h);
    a.target = dart;
    CHECK(act_disarm(&a, ACT_DO) == ACT_SUCCESS && a.used == 1 && NGE_ENTITY_STATE(dart) == NGE_ENTITY_DYING);
    CHECK(events() == 1 && is(0, dart, EV_DISARMED, h));
    /* A web can't be disarmed */
    pit = gen_feature(FT_WEB, AREA);
    feature_place(pit, 1, 2);
    a = ctx(h);
    a.dx = -1;
    CHECK(act_disarm(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_DISARM);
    /* The margin: the roll and Dexterity's modifier, less the DC */
    CHECK(feature_disarm_margin(h, far, 12) == 0 && feature_disarm_margin(h, far, 7) == FEAT_DISARM_SET_OFF);
}

/* The trapdoor (docs/features.md, step 7): at depth 0 here, so the area is the level's */
static void test_trapdoor(void)
{
    nge_entity_id_t pc;
    nge_entity_id_t gob;
    nge_entity_id_t td;
    nge_entity_id_t td2;
    act_t a;
    uint8_t depth;
    uint8_t arrival;
    uint8_t x;
    uint8_t y;

    events();
    nge_entity_init();
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    nge_map_set_block(LOOK_TRAPDOOR, NGE_BLOCK_NONE);
    level_clear_request();
    CHECK(level_depth == 0 && LEVEL_AREA == AREA);
    pc = gen_creature(CR_PLAYER, AREA);
    nge_location_set(pc, 2, 2);
    td = gen_feature(FT_TRAPDOOR, AREA);
    feature_place(td, 2, 2);
    events();

    /* Hidden, '>' doesn't find it */
    a = ctx(pc);
    CHECK(act_descend(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_STAIRS && !level_pending());

    /* Gone off at the player: found, and the player asks for the next depth, anywhere on it */
    act_spring(td, pc);
    CHECK(events() == 2 && is(0, td, EV_TRIGGERED, pc) && is(1, td, EV_FOUND, NGE_EVENT_NONE) && !FEATURE_IS_HIDDEN(td));
    CHECK(level_pending() && NGE_ENTITY_IS_ALIVE(pc));
    level_take_request(&depth, &arrival);
    CHECK(depth == 1 && arrival == LEVEL_ARRIVE_ANY);

    /* Known, '>' takes it down, arriving anywhere: no stairs at the bottom of it */
    a = ctx(pc);
    CHECK(act_descend(&a, ACT_DO) == ACT_SUCCESS && a.target == td && level_pending());
    level_take_request(&depth, &arrival);
    CHECK(depth == 1 && arrival == LEVEL_ARRIVE_ANY);

    /* Held fast, nobody falls */
    effect_apply(EF_HOLD, 1, pc, NGE_ENTITY_INVALID);
    events();
    act_spring(td, pc);
    CHECK(!level_pending());
    effect_cure(pc, STATUS_ROOTED);
    events();

    /* Levitating: a failed disarm sets it off at a flier, which doesn't fall (docs/effects_extensions.md R19) */
    effect_apply(EF_LEVITATE, 1, pc, NGE_ENTITY_INVALID);
    events();
    act_spring(td, pc);
    CHECK(!level_pending());
    effect_cure(pc, STATUS_FLYING);
    events();

    /* From the bottom level, nowhere to fall */
    level_depth = LEVEL_DEPTH_BOTTOM;
    act_spring(td, pc);
    level_depth = 0;
    CHECK(!level_pending());
    events();

    /* A monster leaves the level for the next depth now */
    gob = gen_creature(CR_GOBLIN, AREA);
    nge_location_set(gob, 5, 5);
    td2 = gen_feature(FT_TRAPDOOR, AREA);
    feature_place(td2, 5, 5);
    events();
    act_spring(td2, gob);
    CHECK(NGE_ENTITY_STATE(gob) == NGE_ENTITY_DYING && !level_pending() && events() >= 2);
    CHECK(seen_at(gob, EV_LEFT_LEVEL) >= 0 && seen[seen_at(gob, EV_LEFT_LEVEL)].p1 == 1);
}

static void test_exchange(void)
{
    nge_entity_id_t h = gen_creature(CR_BANDIT, AREA);
    nge_entity_id_t sword = gen_item(IT_SWORD, h);
    nge_entity_id_t sword2 = gen_item(IT_SWORD, h);
    nge_entity_id_t staff = gen_item(IT_QUARTERSTAFF, h);
    nge_entity_id_t shield = gen_item(IT_SHIELD, h);
    nge_entity_id_t potion = gen_item(IT_HEAL_POTION, h);
    nge_entity_id_t ring1;
    nge_entity_id_t ring2;
    nge_entity_id_t ring3;
    nge_entity_id_t cursed;
    nge_entity_id_t frail;
    act_t a;
    nge_entity_id_t id;
    unsigned int r1;
    unsigned int r2;
    int hp;
    int n;
    int i;

    nge_location_set(h, 1, 1);
    events();

    /* A free slot: equipped, as equip would (R4); no target */
    CHECK(exchange(h, sword, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.used == 1 && last.slot == SLOT_WEAPON &&
          last.target == NGE_ENTITY_INVALID && item_slot[sword] == SLOT_WEAPON);
    CHECK(events() >= 1 && seen_at(sword, EV_EQUIPPED) >= 0 && seen_at(sword, EV_UNEQUIPPED) < 0);

    /* One for one, with a full pack (R2): the check first, changing nothing and drawing no numbers */
    inv_capacity[h] = inv_count(h);
    srand(5);
    r1 = (unsigned int)rand();
    srand(5);
    CHECK(exchange(h, sword2, SLOT_NONE, ACT_CHECK) == ACT_SUCCESS && last.used == 0 && last.slot == SLOT_WEAPON &&
          last.target == sword);
    r2 = (unsigned int)rand();
    CHECK(r1 == r2 && item_slot[sword] == SLOT_WEAPON && item_slot[sword2] == SLOT_NONE && events() == 0);
    CHECK(act_do(ACT_EXCHANGE, &last, ACT_DO) == ACT_SUCCESS && last.used == 1 && last.target == sword &&
          item_slot[sword2] == SLOT_WEAPON && item_slot[sword] == SLOT_NONE && stowed(sword, h));
    CHECK(inv_count(h) == inv_capacity[h]);
    /* Off, then on */
    CHECK(events() >= 2 && seen_at(sword, EV_UNEQUIPPED) >= 0 &&
          seen_at(sword, EV_UNEQUIPPED) < seen_at(sword2, EV_EQUIPPED) &&
          seen[seen_at(sword2, EV_EQUIPPED)].p1 == SLOT_WEAPON && seen[seen_at(sword, EV_UNEQUIPPED)].source == h);
    /* Equip still refuses a taken slot (D1) */
    a = ctx(h);
    a.item = sword;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == sword2);

    /* The shared refusals: in use already, can't be equipped, not carried, no inventory */
    CHECK(exchange(h, sword2, SLOT_NONE, ACT_DO) == ACT_FAIL && last.reason == ACT_R_ALREADY_EQUIPPED && last.used == 0);
    CHECK(exchange(h, potion, SLOT_NONE, ACT_DO) == ACT_FAIL && last.reason == ACT_R_CANT_EQUIP);
    id = gen_item(IT_SWORD, AREA);   /* on the floor */
    nge_location_set(id, 2, 2);
    CHECK(exchange(h, id, SLOT_NONE, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NOT_CARRIED);
    nge_entity_destroy(id);
    nge_entity_release(nge_entity_next_dying());
    CHECK(exchange(h, sword, SLOT_HEAD, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_SLOT);
    CHECK(item_slot[sword] == SLOT_NONE && item_slot[sword2] == SLOT_WEAPON && events() == 0);
    inv_capacity[h] = 6;

    /* A shield with a one-handed sword: the shield slot is free, so just equipped */
    CHECK(exchange(h, shield, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.slot == SLOT_SHIELD &&
          last.target == NGE_ENTITY_INVALID && item_slot[sword2] == SLOT_WEAPON);
    events();

    /* A two-hander for a sword and a shield, two for one (R6): a full pack refuses, target the shield */
    inv_capacity[h] = inv_count(h);
    CHECK(exchange(h, staff, SLOT_NONE, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_FULL && last.target == shield);
    CHECK(exchange(h, staff, SLOT_NONE, ACT_DO) == ACT_FAIL && last.reason == ACT_R_FULL && last.used == 0 &&
          item_slot[sword2] == SLOT_WEAPON && item_slot[shield] == SLOT_SHIELD && item_slot[staff] == SLOT_NONE &&
          events() == 0);
    /* With a free place both come off, the weapon first */
    inv_capacity[h] = (uint8_t)(inv_count(h) + 1);
    CHECK(exchange(h, staff, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.slot == SLOT_WEAPON && last.target == sword2 &&
          item_slot[staff] == SLOT_WEAPON && item_slot[sword2] == SLOT_NONE && item_slot[shield] == SLOT_NONE);
    CHECK(inv_count(h) == inv_capacity[h]);
    CHECK(events() >= 3 && seen_at(sword2, EV_UNEQUIPPED) >= 0 &&
          seen_at(sword2, EV_UNEQUIPPED) < seen_at(shield, EV_UNEQUIPPED) &&
          seen_at(shield, EV_UNEQUIPPED) < seen_at(staff, EV_EQUIPPED));

    /* A shield with a two-hander wielded, full pack: the two-hander is exchanged (R7) */
    inv_capacity[h] = inv_count(h);
    CHECK(exchange(h, shield, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.slot == SLOT_SHIELD && last.target == staff &&
          item_slot[shield] == SLOT_SHIELD && item_slot[staff] == SLOT_NONE);
    events();
    /* A two-hander with only a shield worn: the shield is exchanged (R7) */
    CHECK(exchange(h, staff, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.slot == SLOT_WEAPON && last.target == shield &&
          item_slot[staff] == SLOT_WEAPON && item_slot[shield] == SLOT_NONE);
    events();
    inv_capacity[h] = 6;

    /* A cursed weapon in the slot: refused, target it, nothing changes (R3) */
    nge_entity_destroy(potion);
    nge_entity_release(nge_entity_next_dying());
    cursed = gen_item(IT_RUSTY_SWORD, h);
    CHECK(exchange(h, cursed, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.target == staff);
    events();
    CHECK(exchange(h, sword, SLOT_NONE, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_CURSED && last.target == cursed);
    CHECK(exchange(h, sword, SLOT_NONE, ACT_DO) == ACT_FAIL && last.reason == ACT_R_CURSED && last.used == 0 &&
          last.target == cursed && item_slot[cursed] == SLOT_WEAPON && item_slot[sword] == SLOT_NONE && events() == 0);
    /* A cursed sword in the way of a two-hander: refused the same way */
    CHECK(exchange(h, staff, SLOT_NONE, ACT_DO) == ACT_FAIL && last.reason == ACT_R_CURSED && last.target == cursed);
    inv_drop_all(h);   /* the curse doesn't stop a death's drop: start again with an empty pack */
    events();

    /* Rings: the one in use goes, not the free slot (R5); a slot asked for; worn effects counted */
    ring1 = gen_item(IT_RING_HEALTH_2, h);
    ring2 = gen_item(IT_RING_HEALTH_2, h);
    ring3 = gen_item(IT_RING_HEALTH_2, h);
    frail = gen_item(IT_RING_FRAILTY, h);
    wear(h, ring1, SLOT_RING_L);
    events();
    inv_capacity[h] = inv_count(h);
    CHECK(exchange(h, ring2, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.slot == SLOT_RING_L && last.target == ring1 &&
          item_slot[ring2] == SLOT_RING_L && item_slot[ring1] == SLOT_NONE &&
          inv_equipped(h, SLOT_RING_R) == NGE_ENTITY_INVALID);
    events();
    inv_capacity[h] = 6;
    wear(h, ring3, SLOT_RING_R);
    events();
    /* Both worn: asked for the right one; and with none asked, the left */
    CHECK(exchange(h, ring1, SLOT_RING_R, ACT_DO) == ACT_SUCCESS && last.target == ring3 && item_slot[ring1] == SLOT_RING_R &&
          item_slot[ring2] == SLOT_RING_L && item_slot[ring3] == SLOT_NONE);
    events();
    CHECK(exchange(h, ring3, SLOT_NONE, ACT_DO) == ACT_SUCCESS && last.target == ring2 && item_slot[ring3] == SLOT_RING_L);
    events();
    /* Frailty, cursed, for the left ring: its worn effect replaces health's, recomputed once */
    hp = health_hp_max[h];
    CHECK(exchange(h, frail, SLOT_RING_L, ACT_DO) == ACT_SUCCESS && last.target == ring3 && effects_check(h));
    CHECK(health_hp_max[h] < hp);   /* ring3's +2 gone, and frailty's own penalty */
    CHECK(events() >= 3 && seen_at(h, EV_ATTRS_CHANGED) >= 0);
    for (n = 0, i = 0; i < seen_count; i++) {
        n += seen[i].id == EV_ATTRS_CHANGED;
    }
    CHECK(n == 1);
    /* The cursed ring stays: the left can't be exchanged, the right can */
    CHECK(exchange(h, ring2, SLOT_RING_L, ACT_DO) == ACT_FAIL && last.reason == ACT_R_CURSED && last.target == frail);
    CHECK(exchange(h, ring2, SLOT_RING_R, ACT_DO) == ACT_SUCCESS && last.target == ring1);
    events();

    /* Gone, so the tests after find the tiles empty */
    inv_drop_all(h);
    NGE_ENTITY_RELEASE_EACH_DYING(id) {
    }
    events();
    nge_entity_destroy(sword);
    nge_entity_destroy(sword2);
    nge_entity_destroy(staff);
    nge_entity_destroy(shield);
    nge_entity_destroy(cursed);
    nge_entity_destroy(ring1);
    nge_entity_destroy(ring2);
    nge_entity_destroy(ring3);
    nge_entity_destroy(frail);
    nge_entity_destroy(h);
    NGE_ENTITY_RELEASE_EACH_DYING(id) {
    }
    events();
}

/* Confused moves stray, frightened actors won't melee (docs/effects_extensions.md R13, R14) */
static void test_statuses(void)
{
    nge_entity_id_t h;
    nge_entity_id_t rat;
    act_t a;
    unsigned int seed;
    int straight = 0;
    int strayed = 0;
    int stumbled = 0;
    int ok = 1;
    int r;
    int d;
    uint8_t x;
    uint8_t y;

    events();
    nge_entity_init();
    for (y = 0; y < 10; y++) {                                      /* walls round (5, 5), but to the east */
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = (x >= 4 && x <= 6 && y >= 4 && y <= 6 && !(x == 6 && y == 5)) ? WALL : FLOOR;
        }
    }
    NGE_MAP_TILE(5, 5).tile_id = FLOOR;
    h = creature(CR_BANDIT, 5, 5);
    effects_set_base(h, ATTR_HP_MAX, 200);
    health_hp[h] = 200;
    effect_apply(EF_CONFUSE, 1, h, NGE_ENTITY_INVALID);
    events();
    CHECK(STATUS_HAS(h, STATUS_CONFUSED));
    /*
     * East, the one way out, with the same rand() draws as the action: 1 in
     * 3 goes a random way of the 8, which only east lets through
     */
    for (seed = 1; seed <= 60 && ok; seed++) {
        nge_location_set(h, 5, 5);
        srand(seed);
        r = rand();
        d = -1;
        if ((unsigned int)r % 3 == 0) {
            d = (int)((unsigned int)rand() % 8);
            d += (d >= 4) ? 1 : 0;
        }
        srand(seed);
        a = move(h, 1, 0);
        ok = act_move(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0;   /* a check rolls nothing */
        ok = ok && rand() == r;
        srand(seed);
        a = move(h, 1, 0);
        r = act_move(&a, ACT_DO);
        if (d < 0) {
            ok = ok && r == ACT_SUCCESS && at(h, 6, 5);
            straight++;
        } else if (d == 5) {                                     /* the stray way is east too */
            ok = ok && r == ACT_SUCCESS && at(h, 6, 5) && a.dx == 1 && a.dy == 0;
            strayed++;
        } else {
            ok = ok && r == ACT_FAIL && a.reason == ACT_R_STUMBLED && a.used == 1 && at(h, 5, 5) &&
                 a.dx == d % 3 - 1 && a.dy == d / 3 - 1;
            stumbled++;
        }
        events();
    }
    CHECK(ok && straight > 0 && stumbled > 0);
    (void)strayed;
    /* Not confused: always straight on */
    effect_cure(h, STATUS_CONFUSED);
    nge_location_set(h, 5, 5);
    a = move(h, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(h, 6, 5));

    /* Frightened: melee refused, the turn kept */
    rat = creature(CR_RAT, 7, 5);
    effect_apply(EF_FEAR, 1, h, NGE_ENTITY_INVALID);
    events();
    a = ctx(h);
    a.target = rat;
    CHECK(act_melee(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_AFRAID && a.used == 0);
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_AFRAID && a.used == 0 && events() == 0);
    effect_cure(h, STATUS_FRIGHTENED);
    a = ctx(h);
    a.target = rat;
    CHECK(act_melee(&a, ACT_CHECK) == ACT_SUCCESS);
    events();
}

/* Blink (a SPELL_TILE spell, effect_apply_at()) and detect traps (docs/effects_extensions.md R21, R22) */
static void test_blink_detect(void)
{
    nge_entity_id_t pc;
    nge_entity_id_t rat;
    nge_entity_id_t near;
    nge_entity_id_t far;
    uint8_t mp;
    uint8_t x;
    uint8_t y;

    events();
    nge_entity_init();
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    NGE_MAP_TILE(2, 5).tile_id = WALL;
    level_clear_request();
    CHECK(level_depth == 0 && LEVEL_AREA == AREA);
    pc = creature(CR_PLAYER, 2, 2);
    spell_learn(pc, SP_BLINK);
    rat = creature(CR_RAT, 4, 2);
    events();
    mp = mana_mp[pc];

    /* Refused: its own tile, out of range, no line, a creature there; a check moves nothing */
    CHECK(cast(pc, SP_BLINK, 2, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_NO_TARGET);
    CHECK(cast(pc, SP_BLINK, 9, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_OUT_OF_RANGE);
    CHECK(cast(pc, SP_BLINK, 2, 7, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_LINE);
    CHECK(cast(pc, SP_BLINK, 4, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_BLOCKED_ENTITY && last.target == rat);
    CHECK(cast(pc, SP_BLINK, 6, 2, ACT_CHECK) == ACT_SUCCESS && at(pc, 2, 2) && mana_mp[pc] == mp && events() == 0);
    /* Cast: past the rat to the tile chosen, 2 MP, EV_CAST at the caster, then the move */
    CHECK(cast(pc, SP_BLINK, 6, 2, ACT_DO) == ACT_SUCCESS && last.target == pc && at(pc, 6, 2) &&
          mana_mp[pc] == mp - 2);
    CHECK(events() >= 3 && is(0, pc, EV_CAST, pc) && seen_at(pc, EV_MOVED) >= 0 &&
          seen[seen_at(pc, EV_MOVED)].p1 == 0 && seen[seen_at(pc, EV_MOVED)].p2 == 0);
    /* Applied with no tile (not effect_apply_at()), or to a tile it can't enter: it fizzles */
    effect_apply(EF_BLINK, 1, pc, NGE_ENTITY_INVALID);
    CHECK(at(pc, 6, 2) && events() == 1 && is(0, pc, EV_FIZZLED, NGE_EVENT_NONE) && seen[0].p1 == OP_BLINK);
    effect_apply_at(EF_BLINK, 1, pc, NGE_ENTITY_INVALID, 4, 2);   /* the rat's */
    CHECK(at(pc, 6, 2) && events() == 1 && is(0, pc, EV_FIZZLED, NGE_EVENT_NONE));
    effect_apply_at(EF_BLINK, 1, pc, NGE_ENTITY_INVALID, 2, 5);   /* the wall */
    CHECK(at(pc, 6, 2) && events() == 1 && is(0, pc, EV_FIZZLED, NGE_EVENT_NONE));
    effect_apply_at(EF_BLINK, 1, pc, NGE_ENTITY_INVALID, 7, 7);
    CHECK(at(pc, 7, 7));
    events();

    /* Detect traps: hidden features within 1 tile a power (Chebyshev), found by the target */
    near = gen_feature(FT_DART_TRAP, AREA);
    feature_place(near, 5, 5);                                      /* 2 away */
    far = gen_feature(FT_PIT, AREA);
    feature_place(far, 7, 3);                                       /* 4 away */
    events();
    CHECK(FEATURE_IS_HIDDEN(near) && FEATURE_IS_HIDDEN(far));
    effect_apply(EF_DETECT_TRAPS, 1, pc, NGE_ENTITY_INVALID);     /* none within 1 */
    CHECK(FEATURE_IS_HIDDEN(near) && events() == 1 && is(0, pc, EV_FIZZLED, NGE_EVENT_NONE) && seen[0].p1 == OP_DETECT);
    effect_apply(EF_DETECT_TRAPS, 2, pc, NGE_ENTITY_INVALID);
    CHECK(!FEATURE_IS_HIDDEN(near) && FEATURE_IS_HIDDEN(far));
    CHECK(events() == 2 && is(0, near, EV_FOUND, pc) && is(1, pc, EV_EFFECT_STARTED, NGE_EVENT_NONE));
    effect_apply(EF_DETECT_TRAPS, 4, pc, NGE_ENTITY_INVALID);     /* found already: only the pit */
    CHECK(!FEATURE_IS_HIDDEN(far) && events() == 2 && is(0, far, EV_FOUND, pc));
}

int main(void)
{
    nge_entity_id_t sword;
    nge_entity_id_t i1;
    nge_entity_id_t i2;
    nge_entity_id_t sw2;
    nge_entity_id_t ring1;
    nge_entity_id_t ring2;
    nge_entity_id_t ring3;
    nge_entity_id_t goblin;
    nge_entity_id_t gob;
    nge_entity_id_t far;
    nge_entity_id_t blob;
    nge_entity_id_t biter;
    nge_entity_id_t thing;
    nge_entity_id_t door;
    nge_entity_id_t thing2;
    nge_entity_id_t opener;
    nge_entity_id_t stairs;
    nge_entity_id_t climber;
    nge_entity_id_t quest;
    uint8_t depth;
    uint8_t arrival;
    unsigned int s;
    int roll;
    int hp;
    nge_entity_id_t player;
    nge_entity_id_t rat;
    nge_entity_id_t potion;
    act_t a;
    uint8_t x;
    uint8_t y;
    uint8_t str;   /* the player's Strength, put back after the melee checks */
    int r1;
    int r2;
    int ok;

    test_init();

    /* A 10 x 10 room of floor, a wall at (5, 4) */
    nge_map_init(10, 10, &window);
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    nge_map_set_block(FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_SHUT, NGE_BLOCK_NONE);   /* as the level sets them: the door blocks */
    nge_map_set_block(LOOK_DOOR_OPEN, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_STAIRS_UP, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_STAIRS_DOWN, NGE_BLOCK_NONE);
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    NGE_MAP_TILE(5, 4).tile_id = WALL;

    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    player = creature(CR_PLAYER, 4, 5);
    rat = creature(CR_RAT, 6, 5);
    potion = gen_item(IT_HEAL_POTION, AREA);
    nge_location_set(potion, 5, 5);
    events();   /* the player's innate effect started */

    /* ---- a move: the location, the result, the turn used, the events ---- */
    a = move(player, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS);
    CHECK(a.reason == ACT_R_NONE && a.used == 1 && a.target == NGE_ENTITY_INVALID);
    CHECK(at(player, 5, 5));
    CHECK(events() == 2 && is(0, player, EV_MOVED, NGE_EVENT_NONE) && is(1, potion, EV_STOOD_ON, player));
    CHECK(seen[0].p1 == 1 && seen[0].p2 == 0);                 /* the direction: dx, dy */
    /* Diagonally, onto empty floor: EV_MOVED only */
    a = move(player, -1, 1);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(player, 4, 6));
    CHECK(events() == 1 && is(0, player, EV_MOVED, NGE_EVENT_NONE));
    CHECK((int8_t)seen[0].p1 == -1 && (int8_t)seen[0].p2 == 1);
    a = move(player, 1, -1);
    act_move(&a, ACT_DO);
    events();
    CHECK(at(player, 5, 5));

    /* ---- refused: a wall, the map's edge, a creature: nothing changes ---- */
    a = move(player, 0, -1);                                  /* the wall at (5, 4) */
    CHECK(act_move(&a, ACT_DO) == ACT_FAIL);
    CHECK(a.reason == ACT_R_BLOCKED && a.used == 0 && a.target == NGE_ENTITY_INVALID);
    CHECK(at(player, 5, 5) && events() == 0);
    a = move(rat, 1, 0);
    nge_location_set(rat, 9, 5);
    CHECK(act_move(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_BLOCKED && at(rat, 9, 5));   /* the edge */
    nge_location_set(rat, 6, 5);
    a = move(player, 1, 0);                                   /* the rat at (6, 5) */
    CHECK(act_move(&a, ACT_DO) == ACT_FAIL);
    CHECK(a.reason == ACT_R_BLOCKED_ENTITY && a.target == rat && a.used == 0);
    CHECK(at(player, 5, 5) && events() == 0);
    /* A creature doesn't block itself: the rat moves away from the potion's tile... */
    a = move(rat, 0, 1);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(rat, 6, 6));
    events();
    /* ...and an item doesn't block */
    a = move(rat, -1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(rat, 5, 6));
    events();

    /* ---- check mode: the same answers, and nothing changes, not even rand() ---- */
    ok = 1;
    srand(9);
    r1 = rand();
    srand(9);
    a = move(player, 1, 0);                                   /* free */
    ok = ok && act_move(&a, ACT_CHECK) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 0;
    a = move(player, 0, -1);                                  /* the wall */
    ok = ok && act_move(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_BLOCKED && a.used == 0;
    a = move(player, 0, 1);                                   /* the rat, now at (5, 6) */
    ok = ok && act_move(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_BLOCKED_ENTITY && a.target == rat;
    r2 = rand();
    CHECK(ok);
    CHECK(at(player, 5, 5) && events() == 0 && r1 == r2);

    /* ---- act_do: the same as calling the action ---- */
    a = move(player, 0, 1);
    CHECK(act_do(ACT_MOVE, &a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_BLOCKED_ENTITY && a.target == rat);
    a = move(player, -1, 0);
    CHECK(act_do(ACT_MOVE, &a, ACT_DO) == ACT_SUCCESS && at(player, 4, 5) && a.used == 1);
    events();

    /* ---- pick up: player at (4, 5), the potion at (5, 5), the rat at (5, 6) ---- */
    sword = gen_item(IT_SWORD, AREA);
    nge_location_set(sword, 4, 5);
    /* Check mode: possible, nothing changes, no random numbers */
    srand(9);
    r1 = rand();
    srand(9);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.target == sword);
    CHECK(rand() == r1 && at(sword, 4, 5) && events() == 0);
    /* The first item here */
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1 && a.target == sword);
    CHECK(stowed(sword, player) && inv_count(player) == 1);
    CHECK(events() == 1 && is(0, sword, EV_PICKED_UP, player));
    /* Nothing left here */
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_HERE && a.used == 0 && events() == 0);
    /* No inventory: the rat */
    a = ctx(rat);
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_INVENTORY && a.used == 0);
    /* A target given: not on the tile; not an item */
    a = ctx(player);
    a.target = potion;
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_HERE && at(potion, 5, 5));
    a = ctx(player);
    a.target = rat;
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_ITEM);
    /* Two items on a tile: the lowest ID first, or the one asked for */
    i1 = gen_item(IT_HEAL_POTION, AREA);
    i2 = gen_item(IT_SWORD, AREA);
    nge_location_set(i2, 4, 5);
    nge_location_set(i1, 4, 5);
    a = ctx(player);
    a.target = i2;
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && stowed(i2, player) && at(i1, 4, 5));
    events();
    /* Full: refused, nothing changes */
    inv_capacity[player] = 2;
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_FULL && a.target == i1 && a.used == 0);
    CHECK(at(i1, 4, 5) && events() == 0);
    inv_capacity[player] = 20;
    a = ctx(player);
    CHECK(act_do(ACT_PICKUP, &a, ACT_DO) == ACT_SUCCESS && stowed(i1, player) && inv_count(player) == 3);
    events();

    /* ---- drop: on the actor's tile, in its area ---- */
    a = ctx(player);
    a.item = sword;
    CHECK(act_drop(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && stowed(sword, player) && events() == 0);
    CHECK(act_drop(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(at(sword, 4, 5) && NGE_OWNER_OF(sword) == AREA && inv_count(player) == 2);
    CHECK(events() == 1 && is(0, sword, EV_DROPPED, player));
    /* Not carried: on the floor; not an item; someone else's */
    CHECK(act_drop(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED && a.used == 0);
    a = ctx(player);
    a.item = rat;
    CHECK(act_drop(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED);
    a = ctx(rat);
    a.item = i1;
    CHECK(act_drop(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED && stowed(i1, player));
    /* Equipped: take it off first */
    item_slot[i1] = SLOT_WEAPON;
    a = ctx(player);
    a.item = i1;
    CHECK(act_do(ACT_DROP, &a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_EQUIPPED && stowed(i1, player));
    item_slot[i1] = SLOT_NONE;
    CHECK(act_do(ACT_DROP, &a, ACT_DO) == ACT_SUCCESS && at(i1, 4, 5));
    CHECK(events() == 1);

    /* ---- equip: the player has the sword i2 stowed ---- */
    CHECK(stowed(i2, player) && item_slot[i2] == SLOT_NONE && inv_count(player) == 1);
    a = ctx(player);
    a.item = i2;
    a.slot = SLOT_NONE;
    CHECK(act_equip(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.slot == SLOT_WEAPON);
    CHECK(item_slot[i2] == SLOT_NONE && events() == 0);
    a.slot = SLOT_NONE;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1 && a.slot == SLOT_WEAPON);
    CHECK(item_slot[i2] == SLOT_WEAPON && inv_equipped(player, SLOT_WEAPON) == i2);
    CHECK(inv_count(player) == 0);                            /* equipped items don't count */
    CHECK(events() == 1 && is(0, i2, EV_EQUIPPED, player) && seen[0].p1 == SLOT_WEAPON);
    /* Already equipped; the slot taken by another; can't be equipped; not carried */
    a = ctx(player);
    a.item = i2;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ALREADY_EQUIPPED && a.used == 0);
    sw2 = gen_item(IT_SWORD, AREA);
    inv_stow(sw2, player);
    a = ctx(player);
    a.item = sw2;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == i2 && a.used == 0);
    CHECK(item_slot[sw2] == SLOT_NONE && events() == 0);
    i1 = gen_item(IT_HEAL_POTION, AREA);
    inv_stow(i1, player);
    a = ctx(player);
    a.item = i1;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_EQUIP);
    a = ctx(player);
    a.item = sword;                                           /* on the floor */
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED);
    /* Rings: the left hand, then the right, then both are taken */
    ring1 = gen_item(IT_RING_HEALTH_2, AREA);
    ring2 = gen_item(IT_RING_HEALTH_2, AREA);
    ring3 = gen_item(IT_RING_HEALTH_2, AREA);
    inv_stow(ring1, player);
    inv_stow(ring2, player);
    inv_stow(ring3, player);
    a = ctx(player);
    a.item = ring1;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.slot == SLOT_RING_L && item_slot[ring1] == SLOT_RING_L);
    a = ctx(player);
    a.item = ring2;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.slot == SLOT_RING_R && item_slot[ring2] == SLOT_RING_R);
    /* Rings of health: each one worn counts, recomputed by the action; hit points not raised */
    CHECK(health_hp_max[player] == 12 + 4 && health_hp[player] == 12 && effects_check(player));
    a = ctx(player);
    a.item = ring3;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == ring2);
    /* A slot asked for: taken, not one it fits, or free */
    a = ctx(player);
    a.item = ring3;
    a.slot = SLOT_RING_L;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == ring1);
    a = ctx(player);
    a.item = ring3;
    a.slot = SLOT_HEAD;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_SLOT);
    events();

    /* ---- unequip ---- */
    a = ctx(player);
    a.item = ring1;
    CHECK(act_unequip(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && item_slot[ring1] == SLOT_RING_L && events() == 0);
    CHECK(act_unequip(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(item_slot[ring1] == SLOT_NONE && stowed(ring1, player));
    CHECK(events() == 2 && is(0, player, EV_ATTRS_CHANGED, NGE_EVENT_NONE) && is(1, ring1, EV_UNEQUIPPED, player));
    CHECK(health_hp_max[player] == 12 + 2);                  /* ring2's worn effect only */
    CHECK(act_unequip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_EQUIPPED && a.used == 0);
    /* Now the left hand is free: asking for it works */
    a = ctx(player);
    a.item = ring3;
    a.slot = SLOT_RING_L;
    CHECK(act_do(ACT_EQUIP, &a, ACT_DO) == ACT_SUCCESS && item_slot[ring3] == SLOT_RING_L);
    /* No room in the pack: refused, still equipped */
    inv_capacity[player] = inv_count(player);
    a = ctx(player);
    a.item = ring3;
    CHECK(act_do(ACT_UNEQUIP, &a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_FULL && item_slot[ring3] == SLOT_RING_L);
    inv_capacity[player] = 20;
    a = ctx(rat);
    a.item = ring3;
    CHECK(act_unequip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED);
    events();

    /* ---- two hands: a two-handed item and a shield, in either order; the check refuses as the action does ---- */
    {
        nge_entity_id_t holder = gen_creature(CR_BANDIT, AREA);
        nge_entity_id_t staff = gen_item(IT_QUARTERSTAFF, holder);
        nge_entity_id_t shield = gen_item(IT_SHIELD, holder);
        nge_entity_id_t sword = gen_item(IT_SWORD, holder);
        nge_entity_id_t ring = gen_item(IT_RING_FRAILTY, holder);
        nge_entity_id_t cursed = gen_item(IT_RUSTY_SWORD, holder);
        nge_entity_id_t page = gen_item(IT_SP_UNCURSE, holder);

        nge_location_set(holder, 1, 1);
        a = ctx(holder);
        a.item = shield;
        CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.slot == SLOT_SHIELD);
        a = ctx(holder);
        a.item = staff;
        CHECK(act_equip(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == shield);
        a = ctx(holder);
        a.item = staff;
        CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == shield &&
              a.used == 0 && item_slot[staff] == SLOT_NONE);
        a = ctx(holder);
        a.item = sword;
        CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS);            /* one hand: goes with a shield */
        a = ctx(holder);
        a.item = sword;
        CHECK(act_unequip(&a, ACT_DO) == ACT_SUCCESS);
        a = ctx(holder);
        a.item = shield;
        CHECK(act_unequip(&a, ACT_DO) == ACT_SUCCESS);
        a = ctx(holder);
        a.item = staff;
        CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.slot == SLOT_WEAPON);
        a = ctx(holder);
        a.item = shield;
        CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == staff &&
              item_slot[shield] == SLOT_NONE);

        /* ---- curses: on, it stays on, without using the turn, until an uncurse parchment ---- */
        a = ctx(holder);
        a.item = ring;
        CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS);
        events();
        a = ctx(holder);
        a.item = ring;
        CHECK(act_unequip(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_CURSED);
        a = ctx(holder);
        a.item = ring;
        CHECK(act_do(ACT_UNEQUIP, &a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CURSED && a.used == 0 &&
              item_slot[ring] != SLOT_NONE && events() == 0);
        /* Refused before room is asked about */
        inv_capacity[holder] = inv_count(holder);
        a = ctx(holder);
        a.item = ring;
        CHECK(act_unequip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CURSED);
        inv_capacity[holder] = 6;
        /* Read: what is worn is freed, and what is stowed is still cursed */
        a = ctx(holder);
        a.item = page;
        CHECK(act_do(ACT_USE, &a, ACT_DO) == ACT_SUCCESS && !NGE_ENTITY_IS_ALIVE(page));
        CHECK(!inv_cursed(ring) && inv_cursed(cursed));
        events();
        a = ctx(holder);
        a.item = ring;
        CHECK(act_unequip(&a, ACT_DO) == ACT_SUCCESS && item_slot[ring] == SLOT_NONE);
        /* Off once, on again: the curse doesn't come back */
        a = ctx(holder);
        a.item = ring;
        CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS);
        a = ctx(holder);
        a.item = ring;
        CHECK(act_unequip(&a, ACT_DO) == ACT_SUCCESS);
        /* A cursed weapon, and a holder's death: its gear drops, curse or no curse (no action, so no refusal) */
        a = ctx(holder);
        a.item = staff;
        CHECK(act_unequip(&a, ACT_DO) == ACT_SUCCESS);
        a = ctx(holder);
        a.item = cursed;
        CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && item_slot[cursed] == SLOT_WEAPON);
        CHECK(inv_drop_all(holder) >= 1 && item_slot[cursed] == SLOT_NONE && NGE_OWNER_OF(cursed) == AREA);
        /* Gone, so the tests below find the tile empty */
        nge_entity_destroy(staff);
        nge_entity_destroy(shield);
        nge_entity_destroy(sword);
        nge_entity_destroy(ring);
        nge_entity_destroy(cursed);
        NGE_ENTITY_RELEASE_EACH_DYING(door) {
        }
        nge_entity_destroy(holder);
        nge_entity_release(nge_entity_next_dying());
        events();
    }

    test_exchange();
    a = ctx(rat);
    a.item = ring1;
    CHECK(act_exchange(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_INVENTORY);

    /* ---- who can equip: the rat has no inventory, the goblin no ring slots ---- */
    a = ctx(rat);
    a.item = ring1;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_INVENTORY);
    goblin = gen_creature(CR_GOBLIN, AREA);
    nge_location_set(goblin, 1, 1);
    inv_stow(ring1, goblin);
    a = ctx(goblin);
    a.item = ring1;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_SLOT);
    inv_stow(sw2, goblin);
    a = ctx(goblin);
    a.item = sw2;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.slot == SLOT_WEAPON);
    events();

    /* ---- melee: the player (sword i2 wielded) at (4, 5), a goblin west of it ---- */
    CHECK(at(player, 4, 5) && item_slot[i2] == SLOT_WEAPON);
    str = stats_ability[ABIL_STR][player];
    effects_set_base(player, ATTR_ABILITY(ABIL_STR), 10);   /* Strength +0 until its own checks below */
    events();
    CHECK(attack_actor_to_hit(player, ATTACK_MELEE) == 0 && attack_actor_to_damage(player, ATTACK_MELEE) == 0);
    gob = creature(CR_GOBLIN, 3, 5);
    effects_set_base(gob, ATTR_HP_MAX, 200);
    health_hp[gob] = 200;
    events();
    /* Check mode: possible, the weapon and target found, nothing changes, no random numbers */
    srand(9);
    r1 = rand();
    srand(9);
    a = ctx(player);
    a.dx = -1;
    CHECK(act_melee(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.target == gob && a.item == i2);
    CHECK(rand() == r1 && health_hp[gob] == 200 && events() == 0);
    /* A 20 always hits: EV_HIT, then the damage (the sword: slashing), the turn used */
    srand(seed_for_roll(20));
    a = ctx(player);
    a.dx = -1;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(a.target == gob && a.item == i2 && a.damage == DMG_SLASH && a.amount >= 1 && a.amount <= 8);
    CHECK(health_hp[gob] == 200 - a.amount);
    CHECK(events() == 2 && is(0, gob, EV_HIT, player) && seen[0].p1 == 20 && seen[0].p2 == i2 &&
          is(1, gob, EV_DAMAGED, player) && seen[1].p1 == a.amount &&
          seen[1].p2 == EV_DMG_PACK(DMG_NORMAL, DMG_SLASH));
    /* A 1 always misses: EV_MISSED, the turn still used, no damage */
    hp = health_hp[gob];
    srand(seed_for_roll(1));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_MISSED && a.used == 1);
    CHECK(health_hp[gob] == hp && events() == 1 && is(0, gob, EV_MISSED, player) && seen[0].p1 == 1);
    CHECK(act_attempted(ACT_R_MISSED));
    /* Every other roll: hits if roll + 1 (the sword) >= 13 (the goblin's AC) */
    ok = 1;
    for (roll = 2; roll <= 19 && ok; roll++) {
        s = seed_for_roll(roll);
        srand(s);
        a = ctx(player);
        a.target = gob;
        r1 = act_melee(&a, ACT_DO);
        ok = (r1 == ACT_SUCCESS) == (roll + 1 >= 13) && a.used == 1;
        events();
        health_hp[gob] = 200;
    }
    CHECK(ok);
    /* Strength 14 (+2) adds to the sword's to hit: hits if roll + 1 + 2 >= 13 */
    effects_set_base(player, ATTR_ABILITY(ABIL_STR), 14);
    events();
    CHECK(attack_actor_to_hit(player, ATTACK_MELEE) == 2 && attack_actor_to_damage(player, ATTACK_MELEE) == 2);
    CHECK(attack_actor_to_hit(i2, ATTACK_MELEE) == 0 && attack_actor_to_damage(i2, ATTACK_MELEE) == 0);   /* no stats */
    ok = 1;
    for (roll = 2; roll <= 19 && ok; roll++) {
        srand(seed_for_roll(roll));
        a = ctx(player);
        a.target = gob;
        r1 = act_melee(&a, ACT_DO);
        ok = (r1 == ACT_SUCCESS) == (roll + 3 >= 13) && a.used == 1;
        events();
        health_hp[gob] = 200;
    }
    CHECK(ok);
    /* ... and to its damage: 1d8 + 2 */
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.amount >= 3 && a.amount <= 10);
    events();
    health_hp[gob] = 200;
    /* Strength 6 (-2): hits if roll + 1 - 2 >= 13 */
    effects_set_base(player, ATTR_ABILITY(ABIL_STR), 6);
    events();
    ok = 1;
    for (roll = 2; roll <= 19 && ok; roll++) {
        srand(seed_for_roll(roll));
        a = ctx(player);
        a.target = gob;
        r1 = act_melee(&a, ACT_DO);
        ok = (r1 == ACT_SUCCESS) == (roll - 1 >= 13) && a.used == 1;
        events();
        health_hp[gob] = 200;
    }
    CHECK(ok);
    effects_set_base(player, ATTR_ABILITY(ABIL_STR), 10);
    events();
    /* Blinded: -4 to hit, EF_BLIND's modifier (a 15 misses, a 16 hits: roll + 1 - 4 >= 13), the damage as ever */
    effect_apply(EF_BLIND, 1, player, NGE_ENTITY_INVALID);
    events();
    CHECK(attack_actor_to_hit(player, ATTACK_MELEE) == -4 &&
          attack_actor_to_damage(player, ATTACK_MELEE) == 0);
    srand(seed_for_roll(15));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_MISSED);
    srand(seed_for_roll(16));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.amount >= 1 && a.amount <= 8);
    effect_cure(player, STATUS_BLINDED);
    events();
    events();
    health_hp[gob] = 200;
    /* Its own attack without a weapon: fists, blunt, which the goblin resists */
    item_slot[i2] = SLOT_NONE;
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.item == NGE_ENTITY_INVALID && a.damage == DMG_BLUNT);
    CHECK(a.amount == 1 && health_hp[gob] == 199);            /* 1d2 resisted: rounded up to 1 */
    CHECK(events() == 2 && seen[0].p2 == NGE_EVENT_NONE && EV_DMG_OUTCOME(seen[1].p2) == DMG_RESISTED);
    /* Strength adds to its own attack too: 1d2 + 2, resisted: 2 */
    effects_set_base(player, ATTR_ABILITY(ABIL_STR), 14);
    events();
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.amount == 2 && health_hp[gob] == 197);
    events();
    effects_set_base(player, ATTR_ABILITY(ABIL_STR), str);
    events();
    /* A kill: EV_DIED, the goblin destroyed */
    health_hp[gob] = 1;
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = gob;
    CHECK(act_do(ACT_MELEE, &a, ACT_DO) == ACT_SUCCESS && health_hp[gob] == 0);
    CHECK(events() == 3 && seen[2].id == EV_DIED && NGE_ENTITY_STATE(gob) == NGE_ENTITY_DYING);
    /* A boulder north (docs/features.md 4.10): found in that direction by its health, struck, destroyed at 0 hp */
    {
        nge_entity_id_t boulder = gen_feature(FT_BOULDER, NGE_OWNER_OF(player));

        feature_place(boulder, 4, 4);
        health_hp[boulder] = 1;
        srand(seed_for_roll(20));
        a = ctx(player);
        a.dy = -1;
        CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.target == boulder && health_hp[boulder] == 0);
        CHECK(events() == 3 && seen[0].id == EV_HIT && seen[2].id == EV_DIED && NGE_ENTITY_STATE(boulder) == NGE_ENTITY_DYING);
        NGE_MAP_TILE(4, 4).tile_id = FLOOR;   /* as the game's clean-up would redraw it */
    }
    /* Refused: nothing changes, no turn */
    a = ctx(player);
    a.target = gob;                                           /* dying */
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_DAMAGE && a.used == 0);
    a = ctx(player);
    a.dx = -1;                                                /* nothing with health there now */
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_TARGET);
    a = ctx(player);
    a.dx = 0;
    a.dy = -1;                                                /* (4, 4): empty */
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_TARGET);
    a = ctx(player);
    a.target = player;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_TARGET);
    a = ctx(player);
    a.target = sword;                                         /* an item: no health */
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_DAMAGE);
    far = creature(CR_RAT, 8, 8);
    a = ctx(player);
    a.target = far;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_OUT_OF_RANGE);
    nge_owner_set(far, NGE_OWNER_AREA(1));
    nge_location_set(far, 4, 4);                              /* next to it, but in another area */
    a = ctx(player);
    a.target = far;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_OUT_OF_RANGE);
    /* Reach: the 8 neighbours and nothing further, and not the actor's own tile */
    nge_owner_set(far, AREA);
    for (x = 2; x <= 6; x++) {
        for (y = 3; y <= 7; y++) {
            nge_location_set(far, (uint8_t)x, (uint8_t)y);
            a = ctx(player);
            a.target = far;
            r1 = abs(x - 4) <= 1 && abs(y - 5) <= 1 && !(x == 4 && y == 5);
            CHECK((act_melee(&a, ACT_CHECK) == ACT_SUCCESS) == r1);
            CHECK(r1 || a.reason == ACT_R_OUT_OF_RANGE);
        }
    }
    nge_owner_set(far, NGE_OWNER_AREA(1));
    nge_location_set(far, 4, 4);
    far = gen_item(IT_HEAL_POTION, AREA);
    nge_location_set(far, 4, 4);                              /* only an item there: no target */
    a = ctx(player);
    a.dx = 0;
    a.dy = -1;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_TARGET);
    far = creature(CR_RAT, 3, 4);                             /* at 0 hp but not destroyed (persistent) */
    nge_entity_set_flags(far, NGE_ENTITY_PERSISTENT);
    health_hp[far] = 0;
    a = ctx(player);
    a.target = far;
    CHECK(act_melee(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_CANT_DAMAGE);
    /* Something in the weapon slot with no melee attack (a ring): its own attack is used */
    item_slot[ring3] = SLOT_WEAPON;
    health_hp[far] = 3;
    a = ctx(player);
    a.target = far;
    CHECK(act_melee(&a, ACT_CHECK) == ACT_SUCCESS && a.item == NGE_ENTITY_INVALID);
    item_slot[ring3] = SLOT_NONE;
    blob = nge_entity_create();                               /* no attack of its own */
    nge_owner_set(blob, AREA);
    nge_location_set(blob, 2, 2);
    a = ctx(blob);
    a.dx = 1;
    CHECK(act_melee(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_ATTACK);
    CHECK(events() == 0);

    /* ---- a hit's effect: the rat's bite poisons what it leaves alive ---- */
    biter = creature(CR_RAT, 5, 6);
    events();
    srand(seed_for_roll(20));
    a = ctx(biter);
    a.target = player;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.item == NGE_ENTITY_INVALID);
    CHECK(STATUS_HAS(player, STATUS_POISONED) && effects_check(player));
    CHECK(events() == 4 && is(0, player, EV_HIT, biter) && is(1, player, EV_DAMAGED, biter) &&
          seen[2].id == EV_ATTRS_CHANGED && seen[3].id == EV_EFFECT_STARTED && seen[3].p1 == EF_POISON);
    thing = creature(CR_GOBLIN, 6, 6);                         /* killed by the bite: not poisoned */
    health_hp[thing] = 1;
    events();
    srand(seed_for_roll(20));
    a = ctx(biter);
    a.target = thing;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && health_hp[thing] == 0);
    CHECK(events() == 3 && seen[2].id == EV_DIED && !STATUS_HAS(thing, STATUS_POISONED));
    /* A weapon with no hit effect (the sword): just the damage */
    item_slot[i2] = SLOT_WEAPON;
    thing = creature(CR_GOBLIN, 5, 4);
    events();
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = thing;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.item == i2 && events() == 2);

    /* ---- rooted: a step is refused at no cost, but a blocker still gives bump to attack ---- */
    {
        nge_entity_id_t tile = gen_item(IT_CLUB, AREA);       /* no stats: no statuses, stale table or not */
        uint8_t x;
        nge_entity_id_t blocker = creature(CR_RAT, NGE_LOCATION_X(player), (uint8_t)(NGE_LOCATION_Y(player) + 1));

        effect_apply(EF_HOLD, 1, player, NGE_ENTITY_INVALID);
        events();
        CHECK(STATUS_HAS(player, STATUS_ROOTED));
        a = ctx(player);
        a.dx = -1;
        x = NGE_LOCATION_X(player);
        CHECK(act_move(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0);
        a = ctx(player);
        a.dx = -1;
        /* It struggles and the turn is used: a refused step would stop the clock, and a hold could never end */
        CHECK(act_move(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ROOTED && a.used == 1 && events() == 0);
        CHECK(NGE_LOCATION_X(player) == x);
        a = ctx(player);
        a.dy = 1;
        CHECK(act_move(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_BLOCKED_ENTITY && a.target == blocker);
        effect_cure(player, STATUS_ROOTED);
        events();
        a = ctx(player);
        a.dx = -1;
        CHECK(act_move(&a, ACT_CHECK) == ACT_SUCCESS);
        nge_location_set(tile, 8, 1);
        stats_status[tile] = STATUS_ROOTED;
        a = ctx(tile);
        a.dx = -1;
        CHECK(act_move(&a, ACT_CHECK) == ACT_SUCCESS);
        stats_status[tile] = 0;
        nge_entity_destroy(tile);
        nge_entity_destroy(blocker);
        events();
    }

    /* ---- use: a stowed item with a use effect, used up ---- */
    effect_cure(player, STATUS_POISONED);
    health_hp[player] = 3;
    thing = gen_item(IT_HEAL_POTION, AREA);
    inv_stow(thing, player);
    events();
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.effect == EF_HEALING);
    CHECK(NGE_ENTITY_IS_ALIVE(thing) && health_hp[player] == 3 && events() == 0);
    CHECK(act_use(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(NGE_ENTITY_STATE(thing) == NGE_ENTITY_DYING && health_hp[player] == 3 + 6);
    CHECK(events() == 3 && is(0, thing, EV_USED, player) && is(1, player, EV_HEALED, NGE_EVENT_NONE) &&
          seen[1].p1 == 6 && is(2, player, EV_EFFECT_STARTED, NGE_EVENT_NONE) && seen[2].p1 == EF_HEALING);
    /* Used up already: not carried */
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED && a.used == 0);
    /* The hook's power goes to the effect: a draught heals half what a potion does */
    health_hp[player] = 3;
    thing = gen_item(IT_HEAL_DRAUGHT, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_SUCCESS && a.effect == EF_HEALING && health_hp[player] == 3 + 3);
    events();
    /* The new slots: a launcher, its ammo and more armour go where they fit */
    thing = gen_item(IT_SHORT_BOW, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && item_slot[thing] == SLOT_RANGED && inv_equipped(player, SLOT_RANGED) == thing);
    thing = gen_item(IT_ARROWS, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && item_slot[thing] == SLOT_AMMO);
    thing = gen_item(IT_CLOAK, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && item_slot[thing] == SLOT_CLOAK);   /* the last slot: bit 15 */
    /* A wand aims: it isn't used */
    thing = gen_item(IT_WAND_FIRE, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_USE);
    events();
    /* Food, by number: poisoned */
    thing = gen_item(IT_DEAD_RAT, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_do(ACT_USE, &a, ACT_DO) == ACT_SUCCESS && a.effect == EF_POISON && STATUS_HAS(player, STATUS_POISONED));
    events();
    /* Refused: on the floor, not doing anything when used, equipped */
    thing = gen_item(IT_ANTIDOTE, AREA);
    nge_location_set(thing, 4, 5);
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED && NGE_ENTITY_IS_ALIVE(thing));
    thing = gen_item(IT_SWORD, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_CANT_USE);
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_USE && a.used == 0);
    thing = gen_item(IT_ANTIDOTE, AREA);
    inv_stow(thing, player);
    item_slot[thing] = SLOT_NECK;                             /* as if it could be worn */
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_EQUIPPED);
    item_slot[thing] = SLOT_NONE;
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_SUCCESS && !STATUS_HAS(player, STATUS_POISONED));   /* cured */
    CHECK(events() == 4 && seen[0].id == EV_USED && seen[1].id == EV_ATTRS_CHANGED &&
          seen[2].id == EV_EFFECT_ENDED && seen[3].id == EV_EFFECT_STARTED && seen[3].p1 == EF_CURE);

    /* ---- open and close: a goblin at (7, 1), a closed door east of it at (8, 1) ---- */
    opener = creature(CR_GOBLIN, 7, 1);
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 8, 1);
    events();
    /* A closed door blocks a move: the door is the blocker (bump to open) */
    a = move(opener, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_BLOCKED_ENTITY && a.target == door);
    /* Check mode: possible, nothing changes */
    a = ctx(opener);
    a.dx = 1;
    CHECK(act_open(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.target == door);
    CHECK(!FEATURE_IS_OPEN(door) && events() == 0);
    /* Open, by direction: the door opens, the turn used, the event */
    CHECK(act_open(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(FEATURE_IS_OPEN(door) && NGE_MAP_TILE(8, 1).tile_id == LOOK_DOOR_OPEN);
    CHECK(events() == 1 && is(0, door, EV_OPENED, opener));
    CHECK(act_open(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ALREADY_OPEN && a.used == 0);
    /* Open, it can be walked through */
    a = move(opener, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(opener, 8, 1));
    events();                                                  /* EV_MOVED only: no EV_STOOD_ON message for a door */
    /* Back out; closing it with an item in the doorway: something is in the way */
    a = move(opener, -1, 0);
    act_move(&a, ACT_DO);
    thing = gen_item(IT_HEAL_POTION, AREA);
    nge_location_set(thing, 8, 1);                            /* an item in the doorway */
    events();
    a = ctx(opener);
    a.target = door;
    CHECK(act_close(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_OBSTRUCTED);
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_OBSTRUCTED && a.used == 0 && FEATURE_IS_OPEN(door));
    nge_location_remove(thing);
    /* Close, by target: closed, blocking again, the event */
    CHECK(act_close(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && FEATURE_IS_OPEN(door));
    CHECK(act_do(ACT_CLOSE, &a, ACT_DO) == ACT_SUCCESS && a.used == 1 && !FEATURE_IS_OPEN(door));
    CHECK(NGE_MAP_TILE(8, 1).tile_id == LOOK_DOOR_SHUT && events() == 1 && is(0, door, EV_CLOSED, opener));
    a = move(opener, 1, 0);
    CHECK(act_move(&a, ACT_CHECK) == ACT_FAIL && a.target == door);
    a = ctx(opener);
    a.target = door;
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ALREADY_CLOSED);
    CHECK(act_do(ACT_OPEN, &a, ACT_DO) == ACT_SUCCESS && FEATURE_IS_OPEN(door));   /* by number */
    events();
    /* ---- locks: closed and locked it won't open; a key that fits turns, a pick rolls ---- */
    {
        nge_entity_id_t iron = gen_item(IT_KEY_IRON, opener);
        nge_entity_id_t brass = gen_item(IT_KEY_BRASS, opener);
        nge_entity_id_t skeleton = gen_item(IT_KEY_SKELETON, opener);
        nge_entity_id_t sword = gen_item(IT_SWORD, opener);
        nge_entity_id_t pick = gen_item(IT_LOCK_PICK, opener);
        nge_entity_id_t theirs = gen_item(IT_KEY_IRON, player);
        unsigned int low;
        unsigned int high;

        /* Dex 14, +2, against FEAT_PICK_DC: a 12 fails and a 13 works */
        for (low = 1; 1; low++) {
            srand(low);
            if (1 + rand() % 20 == 12) {
                break;
            }
        }
        for (high = 1; 1; high++) {
            srand(high);
            if (1 + rand() % 20 == 13) {
                break;
            }
        }
        a = ctx(opener);
        a.target = door;
        CHECK(act_do(ACT_CLOSE, &a, ACT_DO) == ACT_SUCCESS);
        events();
        feature_lock(door, KEY_IRON);
        a = ctx(opener);
        a.target = door;
        CHECK(act_open(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_LOCKED);
        a = ctx(opener);
        a.dx = 1;                                             /* by direction: the door is found */
        a.target = NGE_ENTITY_INVALID;
        CHECK(act_do(ACT_OPEN, &a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_LOCKED && a.target == door && a.used == 0);
        CHECK(!FEATURE_IS_OPEN(door) && events() == 0);
        /* Locked is shut: closing it says so, and the lock stays */
        a = ctx(opener);
        a.target = door;
        CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ALREADY_CLOSED && FEATURE_IS_LOCKED(door));
        /* Refused: another's key, the wrong key, what isn't a key, what isn't locked */
        a = ctx(opener);
        a.target = door;
        a.item = theirs;
        CHECK(act_unlock(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED);
        a.item = brass;
        CHECK(act_unlock(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_WRONG_KEY && a.used == 0);
        a.item = sword;
        CHECK(act_unlock(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_WRONG_KEY);
        /* The key: a check changes nothing; done, it is unlocked, shut, the key kept, the turn used */
        a.item = iron;
        CHECK(act_unlock(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && FEATURE_IS_LOCKED(door) && events() == 0);
        CHECK(act_do(ACT_UNLOCK, &a, ACT_DO) == ACT_SUCCESS && a.used == 1);
        CHECK(!FEATURE_IS_LOCKED(door) && !FEATURE_IS_OPEN(door) && NGE_ENTITY_IS_ALIVE(iron));
        CHECK(events() == 1 && is(0, door, EV_UNLOCKED, opener));
        CHECK(act_unlock(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_LOCKED);
        /* The skeleton key fits any */
        feature_lock(door, KEY_GOLD);
        a.item = skeleton;
        CHECK(act_unlock(&a, ACT_DO) == ACT_SUCCESS && !FEATURE_IS_LOCKED(door));
        events();
        /* The pick: lost, a use gone and the turn used, nothing posted; won, a use gone */
        feature_lock(door, KEY_IRON);
        a = ctx(opener);
        a.target = door;
        a.item = pick;
        hp = item_count[pick];
        srand(low);
        CHECK(act_unlock(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_PICK_FAILED && a.used == 1);
        CHECK(FEATURE_IS_LOCKED(door) && item_count[pick] == hp - 1 && events() == 0);
        srand(9);
        r1 = rand();
        srand(9);
        CHECK(act_unlock(&a, ACT_CHECK) == ACT_SUCCESS && rand() == r1 && item_count[pick] == hp - 1);   /* no roll */
        srand(high);
        CHECK(act_unlock(&a, ACT_DO) == ACT_SUCCESS && !FEATURE_IS_LOCKED(door) && item_count[pick] == hp - 2);
        CHECK(events() == 1 && is(0, door, EV_UNLOCKED, opener));
        /* Its last use goes, win or lose */
        feature_lock(door, KEY_IRON);
        item_count[pick] = 1;
        srand(low);
        CHECK(act_unlock(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_PICK_FAILED && !NGE_ENTITY_IS_ALIVE(pick));
        CHECK(act_unlock(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED);
        feature_unlock(door);
        nge_entity_destroy(iron);
        nge_entity_destroy(brass);
        nge_entity_destroy(skeleton);
        nge_entity_destroy(sword);
        nge_entity_destroy(theirs);
        NGE_ENTITY_RELEASE_EACH_DYING(thing2) {
        }
        a = ctx(opener);
        a.target = door;
        CHECK(act_do(ACT_OPEN, &a, ACT_DO) == ACT_SUCCESS);
        events();
    }

    /* Refused: nothing there, not a feature, too far */
    a = ctx(opener);
    a.dy = -1;                                                /* (7, 0): floor */
    CHECK(act_open(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_THERE && a.target == NGE_ENTITY_INVALID);
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_THERE);
    a = ctx(opener);
    a.target = thing;                                         /* an item */
    CHECK(act_open(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_OPEN);
    a = ctx(opener);
    a.dx = -1;                                                /* (6, 1): a creature, not a feature */
    biter = creature(CR_RAT, 6, 1);
    CHECK(act_open(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOTHING_THERE);
    a = ctx(biter);
    a.target = door;                                          /* two tiles away */
    CHECK(act_open(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_OUT_OF_RANGE);
    nge_owner_set(biter, NGE_OWNER_AREA(1));
    nge_location_set(biter, 7, 1);                            /* next to it, but in another area */
    a = ctx(biter);
    a.target = door;
    CHECK(act_open(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_OUT_OF_RANGE);
    /* Reach: the door's tile and its neighbours, and nothing further (the door is open) */
    nge_owner_set(biter, AREA);
    for (x = 6; x <= 10; x++) {
        for (y = 0; y <= 3; y++) {
            nge_location_set(biter, (uint8_t)x, (uint8_t)y);
            a = ctx(biter);
            a.target = door;
            r1 = abs(x - 8) <= 1 && abs(y - 1) <= 1;
            CHECK(act_open(&a, ACT_CHECK) == ACT_FAIL);
            CHECK(a.reason == (r1 ? ACT_R_ALREADY_OPEN : ACT_R_OUT_OF_RANGE));
        }
    }
    nge_owner_set(biter, NGE_OWNER_AREA(1));
    nge_location_set(biter, 7, 1);
    nge_location_set(opener, 8, 1);                           /* in the open doorway */
    a = ctx(opener);
    a.target = door;
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_OBSTRUCTED);   /* itself in the way */
    nge_location_set(opener, 7, 1);
    nge_location_remove(door);
    a = ctx(opener);
    a.target = door;                                          /* not placed */
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_OUT_OF_RANGE);
    nge_location_set(door, 8, 1);
    nge_entity_destroy(door);
    a = ctx(opener);
    a.target = door;                                          /* dying */
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_OPEN);
    CHECK(events() == 0);

    /* ---- ascend and descend: stairs on the actor's tile (level_depth 0 here: the area is AREA) ---- */
    level_clear_request();
    events();
    CHECK(nge_location_find(AREA, 2, 8, 0, 0) == NGE_ENTITY_INVALID);
    nge_location_set(player, 2, 8);
    a = ctx(player);
    CHECK(act_descend(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_STAIRS && a.used == 0);
    CHECK(!level_pending() && events() == 0);
    stairs = gen_feature(FT_STAIRS_DOWN, AREA);
    feature_place(stairs, 2, 8);
    CHECK(NGE_MAP_TILE(2, 8).tile_id == LOOK_STAIRS_DOWN && NGE_BLOCK_OF(stairs) == NGE_BLOCK_NONE);
    a = ctx(player);
    CHECK(act_ascend(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_STAIRS);   /* the wrong way */
    a = ctx(player);
    a.target = stairs;
    CHECK(act_open(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_CANT_OPEN);   /* stairs don't open */
    a = ctx(player);
    CHECK(act_descend(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.target == stairs);
    CHECK(!level_pending() && events() == 0);                              /* check mode: nothing */
    a = ctx(player);
    CHECK(act_do(ACT_DESCEND, &a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.target == stairs);
    CHECK(level_pending() && at(player, 2, 8) && NGE_OWNER_OF(player) == AREA);   /* the game moves them */
    level_take_request(&depth, &arrival);
    CHECK(depth == 1 && arrival == LEVEL_ARRIVE_UP_STAIRS && events() == 0);
    /* A monster on the stairs leaves the level: destroyed, not persistent */
    nge_location_set(player, 3, 8);
    climber = creature(CR_GOBLIN, 2, 8);
    effect_apply(EF_HOLD, 1, climber, NGE_ENTITY_INVALID);    /* held: it can't take them */
    a = ctx(climber);
    CHECK(act_descend(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ROOTED && a.used == 1);
    CHECK(NGE_ENTITY_IS_ALIVE(climber) && !level_pending());
    effect_cure(climber, STATUS_ROOTED);
    events();
    a = ctx(climber);
    CHECK(act_descend(&a, ACT_DO) == ACT_SUCCESS && a.used == 1 && !level_pending());
    CHECK(NGE_ENTITY_STATE(climber) == NGE_ENTITY_DYING);
    CHECK(events() == 1 && is(0, climber, EV_LEFT_LEVEL, NGE_EVENT_NONE) && seen[0].p1 == 1);
    /* Up the stairs from depth 1: out of the dungeon (depth 0), only by the player with the quest item */
    level_depth = 1;
    climber = gen_creature(CR_PLAYER, LEVEL_AREA);
    nge_location_set(climber, 1, 8);
    stairs = gen_feature(FT_STAIRS_UP, LEVEL_AREA);
    feature_place(stairs, 1, 8);
    quest = gen_item(IT_QUEST, LEVEL_AREA);
    nge_entity_set_flags(quest, NGE_ENTITY_PERSISTENT);
    quest_set(quest);
    nge_location_set(quest, 2, 8);                            /* on the floor: not carried */
    events();
    a = ctx(climber);
    CHECK(act_ascend(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_NO_QUEST);
    a = ctx(climber);
    CHECK(act_ascend(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_QUEST && a.used == 0 && !level_pending());
    biter = gen_creature(CR_GOBLIN, LEVEL_AREA);              /* a monster with it: refused too */
    nge_location_set(biter, 1, 8);
    inv_stow(quest, biter);
    CHECK(quest_carried(biter) && !quest_carried(climber));
    a = ctx(biter);
    CHECK(act_ascend(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NO_QUEST && a.used == 0);
    CHECK(NGE_ENTITY_IS_ALIVE(biter) && events() == 0);
    inv_stow(quest, climber);
    CHECK(quest_carried(climber) && !quest_carried(biter));
    /* It can't be used, equipped or destroyed */
    a = ctx(climber);
    a.item = quest;
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_USE);
    a = ctx(climber);
    a.item = quest;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_EQUIP);
    CHECK(nge_entity_destroy(quest) == 0 && NGE_ENTITY_IS_ALIVE(quest));
    a = ctx(climber);
    CHECK(act_ascend(&a, ACT_DO) == ACT_SUCCESS && level_pending());
    level_take_request(&depth, &arrival);
    CHECK(depth == 0 && arrival == LEVEL_ARRIVE_DOWN_STAIRS);
    level_depth = 0;

    /* ---- wait: always possible, uses the turn, changes and posts nothing ---- */
    events();
    a = ctx(player);
    srand(9);
    r1 = rand();
    srand(9);
    CHECK(act_wait(&a, ACT_CHECK) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 0);
    CHECK(act_do(ACT_WAIT, &a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(rand() == r1 && events() == 0);
    TEST_EXPECT_HALT(act_wait(&a, ACT_DO));                   /* checked: alive */
    CHECK(test_halt_count == 0);
    a.actor = NGE_ENTITY_MAX;
    TEST_EXPECT_HALT(act_wait(&a, ACT_DO));
    CHECK(test_halt_count == 1);

    /* ---- every reason is refused, but a miss, a struggle, a failed pick and disarm, and a stumble ---- */
    ok = 1;
    for (r1 = 0; r1 < ACT_R_COUNT; r1++) {
        ok = ok && act_attempted((uint8_t)r1) == (r1 == ACT_R_MISSED || r1 == ACT_R_ROOTED ||
                                                  r1 == ACT_R_PICK_FAILED || r1 == ACT_R_DISARM_FAILED ||
                                                  r1 == ACT_R_SET_OFF || r1 == ACT_R_STUMBLED);   /* includes ACT_R_CANT_USE */
    }
    CHECK(ok);

    /* ---- asserts ---- */
    a = move(player, 2, 0);
    TEST_EXPECT_HALT(act_move(&a, ACT_DO));                   /* not one step */
    CHECK(test_halt_count == 1);
    a = move(player, 0, 0);
    TEST_EXPECT_HALT(act_move(&a, ACT_DO));
    CHECK(test_halt_count == 1);
    nge_location_remove(rat);
    a = move(rat, 1, 0);
    TEST_EXPECT_HALT(act_move(&a, ACT_DO));                   /* not placed */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(act_do(ACT_COUNT, &a, ACT_DO));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(act_attempted(ACT_R_COUNT));
    CHECK(test_halt_count == 1);

    /* ---- a move onto a pile: 4 items are told about, and what isn't an item always (docs/events.md, R26) ---- */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    player = creature(CR_PLAYER, 1, 1);
    rat = creature(CR_RAT, 1, 2);
    for (r1 = 0; r1 < 20; r1++) {
        nge_location_set(gen_item(IT_HEAL_POTION, AREA), 2, 1);
    }
    stairs = gen_feature(FT_STAIRS_DOWN, AREA);
    feature_place(stairs, 2, 1);
    nge_event_init();                                         /* the peak, for the largest actions */
    a = move(player, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS);
    CHECK(nge_event_peak() <= NGE_EVENT_QUEUE / 2);           /* R25 */
    CHECK(events() == 6 && is(0, player, EV_MOVED, NGE_EVENT_NONE) && is(5, stairs, EV_STOOD_ON, player));

    /* ---- the largest actions leave half the queue free (R25) ---- */
    nge_event_init();
    nge_location_set(rat, 2, 2);
    a = ctx(rat);
    a.target = player;
    srand(seed_for_roll(20));
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS);              /* a hit that poisons: 4 */
    CHECK(nge_event_peak() == 4 && nge_event_peak() <= NGE_EVENT_QUEUE / 2);
    events();

    /* ---- the line of fire: a goblin at (0, 2), looking along the row ---- */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    nge_map_set_block('O', NGE_BLOCK_NO_FLOOR);
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    opener = creature(CR_GOBLIN, 0, 2);
    rat = creature(CR_RAT, 6, 2);
    srand(9);
    r1 = rand();
    srand(9);
    CHECK(line(opener, 6, 2, 20) == ACT_R_NONE && line_target == rat);
    CHECK(line(opener, 6, 2, 6) == ACT_R_NONE && line_target == rat);       /* the last tile in range */
    CHECK(line(opener, 6, 2, 5) == ACT_R_OUT_OF_RANGE && line_target == NGE_ENTITY_INVALID);
    CHECK(rand() == r1 && events() == 0);                                   /* no random numbers, no events */
    CHECK(line(opener, 7, 2, 20) == ACT_R_NONE && line_target == rat);      /* first in line, short of the tile */
    CHECK(line(opener, 3, 2, 20) == ACT_R_NO_TARGET && line_target == NGE_ENTITY_INVALID);
    CHECK(line(opener, 0, 2, 20) == ACT_R_NO_TARGET);                       /* its own tile */
    /* The same tiles as nge_map_within() on every tile, not a square */
    ok = 1;
    for (y = 0; y < 10; y++) {
        for (x = 1; x < 10; x++) {
            ok = ok && (line(opener, (uint8_t)x, (uint8_t)y, 5) == ACT_R_OUT_OF_RANGE) ==
                       !nge_map_within(0, 2, (uint8_t)x, (uint8_t)y, 5);
        }
    }
    CHECK(ok);
    /* A nearer creature is hit first, and a corpse that stays doesn't stop a shot */
    biter = creature(CR_RAT, 3, 2);
    CHECK(line(opener, 6, 2, 20) == ACT_R_NONE && line_target == biter);
    /* A direct line passes it, to the creature on the chosen tile, with no random numbers or events */
    srand(9);
    CHECK(line_how(opener, 6, 2, 20, ACT_LINE_DIRECT) == ACT_R_NONE && line_target == rat);
    CHECK(line_how(opener, 3, 2, 20, ACT_LINE_DIRECT) == ACT_R_NONE && line_target == biter);
    CHECK(line_how(opener, 5, 2, 20, ACT_LINE_DIRECT) == ACT_R_NO_TARGET && line_target == NGE_ENTITY_INVALID);
    CHECK(line_how(opener, 7, 2, 20, ACT_LINE_DIRECT) == ACT_R_NO_TARGET);   /* past the rat: nobody there */
    CHECK(line_how(opener, 6, 2, 5, ACT_LINE_DIRECT) == ACT_R_OUT_OF_RANGE);
    CHECK(line_how(opener, 6, 2, 6, ACT_LINE_DIRECT) == ACT_R_NONE && line_target == rat);
    CHECK(line_how(opener, 0, 2, 20, ACT_LINE_DIRECT) == ACT_R_NO_TARGET);   /* its own tile */
    CHECK(rand() == r1 && events() == 0);
    /* The same range as a bolt on every tile */
    ok = 1;
    for (y = 0; y < 10; y++) {
        for (x = 1; x < 10; x++) {
            ok = ok && (line_how(opener, (uint8_t)x, (uint8_t)y, 5, ACT_LINE_DIRECT) == ACT_R_OUT_OF_RANGE) ==
                       (line(opener, (uint8_t)x, (uint8_t)y, 5) == ACT_R_OUT_OF_RANGE);
        }
    }
    CHECK(ok);
    /* A wall or a closed door stops it, before the creature in the way or after it */
    NGE_MAP_TILE(4, 2).tile_id = WALL;
    CHECK(line_how(opener, 6, 2, 20, ACT_LINE_DIRECT) == ACT_R_NO_LINE && line_target == NGE_ENTITY_INVALID);
    NGE_MAP_TILE(4, 2).tile_id = FLOOR;
    NGE_MAP_TILE(2, 2).tile_id = WALL;
    CHECK(line_how(opener, 3, 2, 20, ACT_LINE_DIRECT) == ACT_R_NO_LINE);
    NGE_MAP_TILE(2, 2).tile_id = FLOOR;
    health_hp[biter] = 0;
    CHECK(line(opener, 6, 2, 20) == ACT_R_NONE && line_target == rat);
    /* An item isn't a target */
    thing = gen_item(IT_HEAL_POTION, AREA);
    nge_location_set(thing, 3, 2);
    CHECK(line(opener, 3, 2, 20) == ACT_R_NO_TARGET);
    /* A wall stops the line, as does a wall on the chosen tile, and a creature in front of it is hit */
    NGE_MAP_TILE(4, 2).tile_id = WALL;
    CHECK(line(opener, 6, 2, 20) == ACT_R_NO_LINE && line_target == NGE_ENTITY_INVALID);
    CHECK(line(opener, 4, 2, 20) == ACT_R_NO_LINE);
    health_hp[biter] = 1;
    CHECK(line(opener, 6, 2, 20) == ACT_R_NONE && line_target == biter);
    health_hp[biter] = 0;
    NGE_MAP_TILE(4, 2).tile_id = FLOOR;
    /* A closed door stops it, and an open one doesn't */
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 4, 2);
    events();
    CHECK(line(opener, 6, 2, 20) == ACT_R_NO_LINE);
    feature_open(door);
    CHECK(line(opener, 6, 2, 20) == ACT_R_NONE && line_target == rat);
    /* A pit doesn't: a shot passes over it */
    NGE_MAP_TILE(5, 2).tile_id = 'O';
    CHECK(line(opener, 6, 2, 20) == ACT_R_NONE && line_target == rat);
    /* The map's edge stops it */
    nge_location_set(rat, 8, 2);
    CHECK(line(opener, 11, 2, 20) == ACT_R_NONE && line_target == rat);
    nge_location_set(rat, 8, 3);
    CHECK(line(opener, 11, 2, 20) == ACT_R_NO_LINE);

    /* ---- fire: the player at (0, 2), who has nothing yet, and a rat along the row ---- */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    player = creature(CR_PLAYER, 0, 2);
    rat = creature(CR_RAT, 3, 2);
    effects_set_base(rat, ATTR_HP_MAX, 200);
    health_hp[rat] = 200;
    effects_set_base(player, ATTR_ABILITY(ABIL_DEX), 10);   /* Dexterity +0 until its own checks below */
    events();
    CHECK(attack_actor_to_hit(player, ATTACK_RANGED) == 0 && attack_actor_to_damage(player, ATTACK_RANGED) == 0);
    /* The shooter table: what is refused, nothing changing */
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_ATTACK && last.used == 0);
    i1 = equipped(IT_ARROWS, player, SLOT_AMMO);
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_ATTACK);   /* arrows, no launcher */
    nge_entity_destroy(i1);
    i2 = equipped(IT_SHORT_BOW, player, SLOT_RANGED);
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_AMMO);
    i1 = equipped(IT_BOLTS, player, SLOT_AMMO);
    r2 = item_count[i1];
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_WRONG_AMMO && last.used == 0);
    CHECK(item_count[i1] == r2 && events() == 0);
    nge_entity_destroy(i1);
    /* A bow and arrows: a check changes nothing and draws no random numbers */
    i1 = equipped(IT_ARROWS, player, SLOT_AMMO);
    r2 = item_count[i1];
    srand(9);
    r1 = rand();
    srand(9);
    CHECK(fire(player, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.used == 0 && last.target == rat && last.item == i2);
    CHECK(rand() == r1 && item_count[i1] == r2 && health_hp[rat] == 200 && events() == 0);
    /* The player's own tile: settled, the range for the cursor (act_fire()) */
    CHECK(fire(player, 0, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_NO_TARGET && last.amount == 6);
    /* A hit: EV_FIRED first, the shot gone, the turn used, source the shooter, by act_do() */
    srand(seed_for_roll(20));
    a = ctx(player);
    a.x = 3;
    a.y = 2;
    CHECK(act_do(ACT_FIRE, &a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.reason == ACT_R_NONE && a.damage == DMG_PIERCE);
    CHECK(health_hp[rat] == 200 - a.amount && item_count[i1] == r2 - 1);
    CHECK(events() == 3 && is(0, rat, EV_FIRED, player) && seen[0].p1 == i2 && is(1, rat, EV_HIT, player) &&
          seen[1].p2 == i2 && is(2, rat, EV_DAMAGED, player));
    /* A miss: attempted, the shot gone too */
    srand(seed_for_roll(1));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_MISSED && last.used == 1);
    CHECK(item_count[i1] == r2 - 2 && health_hp[rat] == 200 - a.amount);
    CHECK(events() == 2 && is(0, rat, EV_FIRED, player) && is(1, rat, EV_MISSED, player));
    /* A shot past the chosen tile is a shot at what is in line first, a refused one uses nothing */
    CHECK(fire(player, 5, 2, ACT_CHECK) == ACT_SUCCESS && last.target == rat);
    CHECK(fire(player, 2, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_TARGET && last.used == 0);
    CHECK(fire(player, 7, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_OUT_OF_RANGE && last.used == 0);
    NGE_MAP_TILE(1, 2).tile_id = WALL;
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_LINE && last.used == 0);
    NGE_MAP_TILE(1, 2).tile_id = FLOOR;
    CHECK(item_count[i1] == r2 - 2 && events() == 0);
    /* Ammo's bonuses add to the launcher's: AC 11 and a 10 misses without, hits with +1 */
    health_ac[rat] = 11;
    srand(seed_for_roll(10));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_MISSED);
    /* Dexterity 14 (+2) adds to the bow's to hit: a 9 hits AC 11, an 8 misses; and to its damage, 1d6 + 2 */
    item_count[i1] = 20;
    effects_set_base(player, ATTR_ABILITY(ABIL_DEX), 14);
    events();
    CHECK(attack_actor_to_hit(player, ATTACK_RANGED) == 2 && attack_actor_to_damage(player, ATTACK_RANGED) == 2);
    CHECK(attack_actor_to_hit(i2, ATTACK_RANGED) == 0);       /* no stats: the bow itself */
    srand(seed_for_roll(8));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_MISSED);
    srand(seed_for_roll(9));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_SUCCESS && last.amount >= 3 && last.amount <= 8);
    health_hp[rat] = 200;
    /* Dexterity 6 (-2): a 12 misses, a 13 hits */
    effects_set_base(player, ATTR_ABILITY(ABIL_DEX), 6);
    events();
    srand(seed_for_roll(12));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_MISSED);
    srand(seed_for_roll(13));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_SUCCESS);
    health_hp[rat] = 200;
    /* Dexterity 14 and blinded: +2 - 4 to hit (a 12 misses, a 13 hits), +2 damage */
    effects_set_base(player, ATTR_ABILITY(ABIL_DEX), 14);
    events();
    effect_apply(EF_BLIND, 1, player, NGE_ENTITY_INVALID);
    events();
    CHECK(attack_actor_to_hit(player, ATTACK_RANGED) == 2 + -4 &&
          attack_actor_to_damage(player, ATTACK_RANGED) == 2);
    srand(seed_for_roll(12));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_MISSED);
    srand(seed_for_roll(13));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_SUCCESS && last.amount >= 3 && last.amount <= 8);
    effect_cure(player, STATUS_BLINDED);
    events();
    health_hp[rat] = 200;
    effects_set_base(player, ATTR_ABILITY(ABIL_DEX), 10);
    events();
    nge_entity_destroy(i1);
    i1 = equipped(IT_ARROWS_1, player, SLOT_AMMO);
    srand(seed_for_roll(10));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_SUCCESS && last.amount >= 2);
    ok = 1;
    item_count[i1] = 20;
    for (x = 0; x < 12; x++) {                                /* 1d6 + 1: at least 2 every time */
        srand(seed_for_roll(20));
        health_hp[rat] = 200;
        ok = ok && fire(player, 3, 2, ACT_DO) == ACT_SUCCESS && last.amount >= 2;
        events();
    }
    CHECK(ok);
    /* The last of a stack: destroyed, and the slot empty */
    item_count[i1] = 1;
    events();
    srand(seed_for_roll(1));
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_MISSED);
    CHECK(!NGE_ENTITY_IS_ALIVE(i1) && inv_equipped(player, SLOT_AMMO) == NGE_ENTITY_INVALID);
    CHECK(fire(player, 3, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_NO_AMMO);
    events();
    /* Thrown: its own attack and range, with or without a launcher, one used up */
    i1 = equipped(IT_DARTS, player, SLOT_AMMO);
    CHECK(fire(player, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.item == i1 && last.amount == 4);
    CHECK(fire(player, 0, 2, ACT_CHECK) == ACT_FAIL && last.amount == 4);
    nge_entity_destroy(i2);
    srand(seed_for_roll(20));
    r2 = item_count[i1];
    CHECK(fire(player, 3, 2, ACT_DO) == ACT_SUCCESS && last.item == i1 && item_count[i1] == r2 - 1);
    CHECK(events() == 3 && seen[0].p1 == i1 && seen[1].p2 == i1);
    nge_entity_destroy(i1);
    /* The ammo's hit effect: a flask of oil's fire, then it is gone. The most events a shot posts */
    nge_location_set(rat, 2, 2);
    i1 = equipped(IT_OIL, player, SLOT_AMMO);
    events();
    nge_event_init();                                         /* the peak */
    srand(seed_for_roll(20));
    health_hp[rat] = 2;
    CHECK(fire(player, 2, 2, ACT_DO) == ACT_SUCCESS && !NGE_ENTITY_IS_ALIVE(i1));
    r1 = nge_event_peak();
    CHECK(r1 >= 3 && r1 <= NGE_EVENT_QUEUE / 2);
    events();
    /* A creature's own ranged attack: nothing used up, the event says so, no launcher */
    {
        static const attack_kind_t spit = { 0, 1, 4, 0, DMG_ACID, 5 };

        attack_add(player, ATTACK_RANGED, &spit);
        rat = creature(CR_RAT, 3, 2);
        srand(seed_for_roll(20));
        CHECK(fire(player, 3, 2, ACT_DO) == ACT_SUCCESS && last.item == NGE_ENTITY_INVALID && last.damage == DMG_ACID);
        CHECK(events() >= 3 && seen[0].id == EV_FIRED && seen[0].p1 == NGE_EVENT_NONE);
    }

    /* ---- stacks: the player at (0, 2), arrows stowed ---- */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    player = creature(CR_PLAYER, 0, 2);
    i1 = gen_item(IT_ARROWS, player);
    /* Merged into the stack: gone from the floor, the event as ever, a check changing nothing */
    i2 = floor_stack(IT_ARROWS, 7, 0, 2);
    events();
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_CHECK) == ACT_SUCCESS && a.target == i2 && item_count[i1] == 15 && at(i2, 0, 2));
    r2 = item_count[i1];
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.target == i2);
    CHECK(item_count[i1] == r2 + 7 && !NGE_ENTITY_IS_ALIVE(i2) && inv_count(player) == 1);
    CHECK(events() == 1 && is(0, i2, EV_PICKED_UP, player));
    /* A merge needs no room, a new stack does */
    inv_capacity[player] = 1;
    i2 = floor_stack(IT_ARROWS, 1, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && !NGE_ENTITY_IS_ALIVE(i2));
    i2 = floor_stack(IT_BOLTS, 12, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_FULL && a.target == i2 && a.used == 0);
    nge_entity_destroy(i2);
    /* Into an equipped stack too */
    item_slot[i1] = SLOT_AMMO;
    r2 = item_count[i1];
    i2 = floor_stack(IT_ARROWS, 3, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && item_count[i1] == r2 + 3 && item_slot[i1] == SLOT_AMMO);
    /* Exactly to 255: all of it fits */
    item_count[i1] = 250;
    i2 = floor_stack(IT_ARROWS, 5, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && item_count[i1] == 255 && !NGE_ENTITY_IS_ALIVE(i2));
    /* Past 255: the rest stays where it is, the turn used */
    item_count[i1] = 250;
    i2 = floor_stack(IT_ARROWS, 8, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.target == i2);
    CHECK(item_count[i1] == 255 && item_count[i2] == 3 && at(i2, 0, 2) && NGE_ENTITY_IS_ALIVE(i2));
    /* A full stack takes no more: it takes room, so with none it is refused, with some a new stack */
    item_slot[i1] = SLOT_NONE;
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_FULL);
    inv_capacity[player] = 20;
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && stowed(i2, player) && item_count[i2] == 3);
    events();
    /* Not the same kind, and not ammo: a count that isn't a quantity doesn't merge */
    i2 = floor_stack(IT_ARROWS_1, 10, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && stowed(i2, player));
    i1 = gen_item(IT_WAND_FIRE, player);
    i2 = floor_stack(IT_WAND_FIRE, 5, 0, 2);
    a = ctx(player);
    CHECK(act_pickup(&a, ACT_DO) == ACT_SUCCESS && stowed(i2, player) && item_count[i1] == 5 && item_count[i2] == 5);
    events();

    /* ---- aim: the player at (0, 2), a wand of fire stowed, a rat along the row ---- */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    player = creature(CR_PLAYER, 0, 2);
    rat = creature(CR_RAT, 3, 2);
    effects_set_base(rat, ATTR_HP_MAX, 200);
    health_hp[rat] = 200;
    i1 = gen_item(IT_WAND_FIRE, player);
    events();
    /* Check mode: possible, the effect and target found, nothing changes, no random numbers */
    r2 = item_count[i1];
    srand(9);
    r1 = rand();
    srand(9);
    CHECK(aim(player, i1, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.used == 0 && last.target == rat &&
          last.effect == EF_BOLT_FIRE);
    CHECK(rand() == r1 && item_count[i1] == r2 && health_hp[rat] == 200 && events() == 0);
    /* Done, by act_do(): a charge used, the event first, the effect's source the actor */
    a = ctx(player);
    a.item = i1;
    a.x = 3;
    a.y = 2;
    CHECK(act_do(ACT_AIM, &a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.reason == ACT_R_NONE && a.target == rat);
    CHECK(item_count[i1] == r2 - 1 && NGE_ENTITY_IS_ALIVE(i1) && health_hp[rat] < 200);
    CHECK(events() >= 2 && is(0, rat, EV_AIMED, player) && seen[0].p1 == i1 && is(1, rat, EV_DAMAGED, player));
    /* The line and the range, as a shot's; first in line takes it */
    biter = creature(CR_RAT, 2, 2);
    CHECK(aim(player, i1, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.target == biter);
    /* A direct item passes it, to the one chosen; a check gives the same target */
    i2 = gen_item(IT_WAND_TELEPORT, player);
    CHECK(aim(player, i2, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.target == rat);
    CHECK(aim(player, i2, 2, 2, ACT_CHECK) == ACT_SUCCESS && last.target == biter);
    nge_entity_destroy(i2);
    nge_entity_release(i2);
    i2 = gen_item(IT_WAND_SLOW, player);
    r1 = item_count[i2];
    CHECK(aim(player, i2, 3, 2, ACT_DO) == ACT_SUCCESS && last.target == rat && item_count[i2] == r1 - 1);
    CHECK(events() >= 1 && is(0, rat, EV_AIMED, player));
    nge_entity_destroy(i2);
    nge_entity_release(i2);
    events();
    nge_entity_destroy(biter);
    nge_entity_release(biter);
    CHECK(aim(player, i1, 9, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_OUT_OF_RANGE && last.used == 0);
    CHECK(aim(player, i1, 1, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_TARGET);
    NGE_MAP_TILE(1, 2).tile_id = WALL;
    CHECK(aim(player, i1, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_LINE);
    NGE_MAP_TILE(1, 2).tile_id = FLOOR;
    CHECK(item_count[i1] == r2 - 1 && events() == 0);
    /* The range is ACT_AIM_RANGE exactly */
    nge_location_set(rat, 8, 2);
    CHECK(aim(player, i1, 8, 2, ACT_CHECK) == ACT_SUCCESS && last.target == rat);
    nge_location_set(rat, 9, 2);
    CHECK(aim(player, i1, 9, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_OUT_OF_RANGE);
    nge_location_set(rat, 3, 2);
    /* A wand at 0 stays, refused; its own tile asks about the item alone */
    item_count[i1] = 0;
    CHECK(aim(player, i1, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_CHARGES && last.used == 0);
    CHECK(NGE_ENTITY_IS_ALIVE(i1) && events() == 0);
    item_count[i1] = 1;
    CHECK(aim(player, i1, 0, 2, ACT_CHECK) == ACT_SUCCESS && last.target == player);
    /* The last charge: down to 0, and kept */
    CHECK(aim(player, i1, 3, 2, ACT_DO) == ACT_SUCCESS && item_count[i1] == 0 && stowed(i1, player));
    events();
    /* A parchment: no count, destroyed, the effect still works */
    i2 = gen_item(IT_SP_MISSILE, player);
    health_hp[rat] = 200;
    CHECK(aim(player, i2, 3, 2, ACT_DO) == ACT_SUCCESS && !NGE_ENTITY_IS_ALIVE(i2) && health_hp[rat] < 200);
    CHECK(events() >= 2 && seen[0].id == EV_AIMED && seen[0].p1 == i2);
    /* Own tile: a staff of healing on its holder */
    i2 = gen_item(IT_STAFF_HEALING, player);
    r2 = item_count[i2];
    health_hp[player] = 1;
    CHECK(aim(player, i2, 0, 2, ACT_DO) == ACT_SUCCESS && last.target == player && health_hp[player] > 1);
    CHECK(item_count[i2] == r2 - 1 && events() >= 2 && is(0, player, EV_AIMED, player));
    /* A kill names the actor, so its XP is earned */
    i2 = gen_item(IT_STAFF_POWER, player);
    health_hp[rat] = 1;
    CHECK(aim(player, i2, 3, 2, ACT_DO) == ACT_SUCCESS && health_hp[rat] == 0);
    ok = 0;
    r1 = events();
    for (x = 0; x < r1; x++) {
        ok = ok || (seen[x].id == EV_DIED && seen[x].source == player);
    }
    CHECK(ok);
    /* What can't be aimed, what isn't held and what is equipped */
    i2 = gen_item(IT_HEAL_POTION, player);
    rat = creature(CR_RAT, 3, 2);
    CHECK(aim(player, i2, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_CANT_AIM && last.used == 0);
    i2 = gen_item(IT_WAND_FIRE, AREA);
    CHECK(aim(player, i2, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NOT_CARRIED);
    i2 = equipped(IT_WAND_FIRE, player, SLOT_NECK);
    CHECK(aim(player, i2, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_EQUIPPED);
    events();

    /* ---- cast: the player at (0, 2), who knows Magic Bolt and has 5 MP, a rat along the row ---- */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    player = creature(CR_PLAYER, 0, 2);
    rat = creature(CR_RAT, 3, 2);
    effects_set_base(rat, ATTR_HP_MAX, 200);
    health_hp[rat] = 200;
    events();
    CHECK(spell_knows(player, SP_MAGIC_BOLT) && mana_mp[player] == 5);
    /* Refused, in order, changing nothing and posting nothing: not known, then the MP, then a target */
    mana_spend(player, 5);
    events();
    CHECK(cast(player, SP_FIRE_BOLT, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_SPELL && last.used == 0);
    CHECK(cast(player, SP_NONE, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_SPELL);
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_MANA && last.used == 0);
    CHECK(cast(player, SP_MAGIC_BOLT, 0, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_NO_MANA);
    mana_restore(player, 255);
    events();
    CHECK(cast(player, SP_MAGIC_BOLT, 0, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_TARGET);   /* can cast: a tile next */
    CHECK(cast(rat, SP_MAGIC_BOLT, 0, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_SPELL);      /* not a caster */
    CHECK(mana_mp[player] == 5 && health_hp[rat] == 200 && events() == 0);
    /* Check mode: the target found, nothing changed, no random numbers, the spell left as it was */
    srand(9);
    r1 = rand();
    srand(9);
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.used == 0 && last.target == rat &&
          last.spell == SP_MAGIC_BOLT);
    CHECK(rand() == r1 && mana_mp[player] == 5 && health_hp[rat] == 200 && events() == 0);
    /* A bolt, by act_do(): EV_CAST, the MP, then the effect's events with the actor as source */
    a = ctx(player);
    a.spell = SP_MAGIC_BOLT;
    a.x = 3;
    a.y = 2;
    nge_event_init();
    CHECK(act_do(ACT_CAST, &a, ACT_DO) == ACT_SUCCESS && a.used == 1 && a.reason == ACT_R_NONE && a.target == rat &&
          a.spell == SP_MAGIC_BOLT);
    CHECK(mana_mp[player] == 4 && health_hp[rat] == 197);              /* power 3, 1 magic damage each */
    CHECK(nge_event_peak() <= NGE_EVENT_QUEUE / 2);                      /* R32 */
    CHECK(events() >= 3 && is(0, rat, EV_CAST, player) && seen[0].p1 == SP_MAGIC_BOLT &&
          is(1, player, EV_MANA_SPENT, NGE_EVENT_NONE) && seen[1].p1 == 1 && is(2, rat, EV_DAMAGED, player));
    /* The first creature in line takes a bolt; a direct spell passes it, to the one chosen */
    biter = creature(CR_RAT, 2, 2);
    spell_learn(player, SP_HOLD);
    spell_learn(player, SP_CURE_WOUNDS);
    events();
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.target == biter);
    CHECK(cast(player, SP_HOLD, 3, 2, ACT_CHECK) == ACT_SUCCESS && last.target == rat);
    CHECK(cast(player, SP_HOLD, 2, 2, ACT_CHECK) == ACT_SUCCESS && last.target == biter);
    nge_entity_destroy(biter);
    nge_entity_release(biter);
    /* A wall stops either; the range is the spell's, exactly */
    NGE_MAP_TILE(1, 2).tile_id = WALL;
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_LINE);
    CHECK(cast(player, SP_HOLD, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_LINE);
    NGE_MAP_TILE(1, 2).tile_id = FLOOR;
    nge_location_set(rat, 8, 2);
    CHECK(cast(player, SP_MAGIC_BOLT, 8, 2, ACT_CHECK) == ACT_SUCCESS && last.target == rat);
    CHECK(cast(player, SP_MAGIC_BOLT, 9, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_OUT_OF_RANGE);
    CHECK(cast(player, SP_HOLD, 6, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_NO_TARGET);       /* empty tile */
    CHECK(cast(player, SP_HOLD, 7, 2, ACT_CHECK) == ACT_FAIL && last.reason == ACT_R_OUT_OF_RANGE);    /* range 6 */
    nge_location_set(rat, 3, 2);
    CHECK(mana_mp[player] == 4 && events() == 0);
    /* A self spell: on the caster wherever the tile is; no line, no range */
    health_hp[player] = 2;
    CHECK(cast(player, SP_CURE_WOUNDS, 9, 9, ACT_DO) == ACT_SUCCESS && last.target == player && health_hp[player] == 6);
    CHECK(events() >= 3 && is(0, player, EV_CAST, player) && seen[0].p1 == SP_CURE_WOUNDS &&
          is(1, player, EV_MANA_SPENT, NGE_EVENT_NONE) && seen[2].id == EV_HEALED);
    /* MP at exactly the cost: cast, and then none left */
    CHECK(mana_mp[player] == 3);
    mana_spend(player, 2);
    events();
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_DO) == ACT_SUCCESS && mana_mp[player] == 0);
    events();
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_MANA);
    /* A kill names the actor, so its XP is earned: 5 events, within half the queue */
    mana_restore(player, 255);
    events();
    health_hp[rat] = 1;
    nge_event_init();
    CHECK(cast(player, SP_MAGIC_BOLT, 3, 2, ACT_DO) == ACT_SUCCESS && health_hp[rat] == 0);
    CHECK(nge_event_peak() <= NGE_EVENT_QUEUE / 2);
    ok = 0;
    r1 = events();
    for (x = 0; x < r1; x++) {
        ok = ok || (seen[x].id == EV_DIED && seen[x].source == player);
    }
    CHECK(ok);
    /* A caster without mana: refused a spell that costs */
    rat = creature(CR_RAT, 3, 2);
    {
        static const uint8_t bolt[1] = { SP_MAGIC_BOLT };

        spell_add(rat, bolt, 1);
    }
    CHECK(cast(rat, SP_MAGIC_BOLT, 0, 2, ACT_DO) == ACT_FAIL && last.reason == ACT_R_NO_MANA);
    /* A timed effect with no entity free: refused by the effect, and the MP stay spent (section 8) */
    events();
    r2 = mana_mp[player];
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    CHECK(cast(player, SP_HOLD, 3, 2, ACT_DO) == ACT_SUCCESS && mana_mp[player] == r2 - 2 &&
          !STATUS_HAS(rat, STATUS_ROOTED));
    events();
    TEST_EXPECT_HALT(cast(player, SPELL_COUNT, 3, 2, ACT_DO));
    CHECK(test_halt_count == 1);

    test_operate();
    test_container();
    test_traps();
    test_statuses();
    test_blink_detect();
    test_search_disarm();
    test_trapdoor();
    test_learn();

    return test_summary("act");
}
