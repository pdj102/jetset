/* game/ai: the AI component, its brains, capabilities, events, ai_try(), and the default brain's turns. */
#include "test.h"

#include <stdlib.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "map/nge_map.h"

#include "data/tile_data.h"

#include "game/ai/ai_internal.h"
#include "game/components/attack.h"
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/stats.h"
#include "game/events.h"
#include "game/gen/gen.h"

#define AREA   NGE_OWNER_AREA(0)
#define AREA1  NGE_OWNER_AREA(1)
#define FLOOR  '.'
#define WALL   '#'

/* The player: the AI looks and listens for them (game/actors.c isn't linked) */
nge_entity_id_t actor_player;

static const nge_rect_t window = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 19 };   /* a view of 10 x 10 tiles */

/* The screen's map view, for the level module (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

/* The events handled since the last call, each passed to the AI first, as the game does */
static nge_event_t seen[16];
static int seen_count;

static void dispatch(const nge_event_t *e)
{
    seen[seen_count++] = *e;
    ai_on_event(e);
}

static int events(void)
{
    seen_count = 0;
    nge_event_process(dispatch);
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

/* A seed whose first rand() gives the d20 roll r: hearing's first draw is its roll */
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

/* A seed whose first rand() is even: an idle entity doesn't wander this turn */
static unsigned int seed_no_wander(void)
{
    unsigned int s;

    for (s = 1; s < 100000; s++) {
        srand(s);
        if ((rand() & 1) == 0) {
            return s;
        }
    }
    return 0;
}

/* A fresh room of floor, 10 x 10, with nothing in it but the player at (px, py) */
static void scene(uint8_t px, uint8_t py)
{
    uint8_t x;
    uint8_t y;

    for (y = 0; y < 10; y++) {
        for (x = 0; x < 10; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    actor_player = creature(CR_PLAYER, px, py);
    events();                                               /* its innate effect started */
}

/* A wall on column x, from y0 to y1 */
static void wall_column(uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t y;

    for (y = y0; y <= y1; y++) {
        NGE_MAP_TILE(x, y).tile_id = WALL;
    }
}

/* A creature of kind at (x, y), hunting the player, its goal (gx, gy) */
static nge_entity_id_t hunter(uint8_t kind, uint8_t x, uint8_t y, uint8_t gx, uint8_t gy)
{
    nge_entity_id_t id = creature(kind, x, y);

    ai_state[id] = AI_STATE_HUNT;
    ai_target[id] = actor_player;
    ai_goal_x[id] = gx;
    ai_goal_y[id] = gy;
    return id;
}

/* A context for actor with no inputs */
static act_t ctx(nge_entity_id_t actor)
{
    act_t a = { 0 };

    a.actor = actor;
    a.target = NGE_ENTITY_INVALID;
    a.item = NGE_ENTITY_INVALID;
    return a;
}

int main(void)
{
    static const ai_kind_t asleep = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_ASLEEP };
    static const ai_kind_t idle = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_IDLE };
    ai_kind_t k;
    nge_entity_id_t a;
    nge_entity_id_t b;
    nge_entity_id_t player;
    nge_entity_id_t rat;
    nge_entity_id_t goblin;
    nge_entity_id_t gob2;
    nge_entity_id_t holder;
    nge_entity_id_t sword;
    nge_entity_id_t door;
    nge_entity_id_t potion;
    act_t c;
    uint8_t hp;
    int r;
    uint8_t x;
    uint8_t y;
    uint8_t s;
    uint8_t t;
    int ok;

    test_init();

    /* A 10 x 10 room of floor, a wall at (5, 4) */
    nge_map_init(10, 10, &window);
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    nge_map_set_block(FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_SHUT, NGE_BLOCK_NONE);   /* as the level sets them: the door blocks */
    nge_map_set_block(LOOK_DOOR_OPEN, NGE_BLOCK_NONE);
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

    /* ---- the component: add and remove, its brain and state ---- */
    a = nge_entity_create();
    b = nge_entity_create();
    CHECK(!AI_IS(a));                                       /* new: none */
    NGE_ENTITY_ADD_COMPONENT(a, COMPONENT_PC);
    ai_add(a, &asleep);
    CHECK(AI_IS(a) && !AI_IS(b) && ai_get_state(a) == AI_STATE_ASLEEP && ai_brain[a] == AI_BRAIN_DEFAULT);
    CHECK(NGE_ENTITY_HAS_COMPONENT(a, COMPONENT_PC));       /* others kept */
    ai_add(a, &idle);                                       /* again: the new start */
    CHECK(AI_IS(a) && ai_get_state(a) == AI_STATE_IDLE);
    ai_remove(a);
    CHECK(!AI_IS(a) && NGE_ENTITY_HAS_COMPONENT(a, COMPONENT_PC));
    ai_remove(a);                                           /* without it: no change */
    CHECK(!AI_IS(a));
    CHECK(events() == 0);                                   /* adding posts nothing */

    /* Cleared with the entity */
    ai_add(b, &asleep);
    nge_entity_destroy(b);
    nge_entity_release(nge_entity_next_dying());
    CHECK(!AI_IS(b));

    /* Asserts: a free entity, no entity, no brain or state, out of range, no AI */
    TEST_EXPECT_HALT(ai_add(b, &asleep));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(ai_remove(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);
    k = asleep;
    k.brain = AI_BRAIN_NONE;
    TEST_EXPECT_HALT(ai_add(a, &k));
    CHECK(test_halt_count == 1);
    k.brain = AI_BRAIN_COUNT;
    TEST_EXPECT_HALT(ai_add(a, &k));
    CHECK(test_halt_count == 1);
    k = asleep;
    k.state = AI_STATE_NONE;
    TEST_EXPECT_HALT(ai_add(a, &k));
    CHECK(test_halt_count == 1);
    k.state = AI_STATE_COUNT;
    TEST_EXPECT_HALT(ai_add(a, &k));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(ai_get_state(a));                      /* a has no AI */
    CHECK(test_halt_count == 1);
    nge_entity_destroy(a);
    nge_entity_release(nge_entity_next_dying());

    /* ---- the brains: every entry a state or no change; nothing new changes nothing ---- */
    ok = 1;
    for (k.brain = 0; k.brain < AI_BRAIN_COUNT; k.brain++) {
        for (s = 0; s < AI_STATE_COUNT; s++) {
            for (t = 0; t < AI_STIM_COUNT; t++) {
                ok = ok && ai_brains[k.brain].next[s][t] < AI_STATE_COUNT;
                ok = ok && (k.brain != AI_BRAIN_NONE || ai_brains[k.brain].next[s][t] == AI_STATE_NO_CHANGE);
            }
            ok = ok && ai_brains[k.brain].next[s][AI_STIM_NONE] == AI_STATE_NO_CHANGE;
        }
    }
    CHECK(ok);
    /* The default brain, in full (docs/ai_default_brain.md, R11) */
#define NEXT(s, t)  (ai_brains[AI_BRAIN_DEFAULT].next[AI_STATE_##s][AI_STIM_##t])
    CHECK(NEXT(ASLEEP, HURT) == AI_STATE_HUNT && NEXT(ASLEEP, NOISE) == AI_STATE_IDLE &&
          NEXT(ASLEEP, SEEN) == AI_STATE_NO_CHANGE && NEXT(ASLEEP, LOST) == AI_STATE_NO_CHANGE);
    CHECK(NEXT(IDLE, HURT) == AI_STATE_HUNT && NEXT(IDLE, NOISE) == AI_STATE_NO_CHANGE &&
          NEXT(IDLE, SEEN) == AI_STATE_HUNT && NEXT(IDLE, LOST) == AI_STATE_NO_CHANGE);
    CHECK(NEXT(HUNT, HURT) == AI_STATE_NO_CHANGE && NEXT(HUNT, NOISE) == AI_STATE_NO_CHANGE &&
          NEXT(HUNT, SEEN) == AI_STATE_NO_CHANGE && NEXT(HUNT, LOST) == AI_STATE_IDLE);
    for (s = 0; s < AI_STATE_COUNT; s++) {
        CHECK(ai_brains[AI_BRAIN_DEFAULT].next[s][AI_STIM_NONE] == AI_STATE_NO_CHANGE);
    }
#undef NEXT

    /* ---- ai_can(): from the components and the current abilities ---- */
    player = creature(CR_PLAYER, 4, 5);
    rat = creature(CR_RAT, 6, 5);
    goblin = creature(CR_GOBLIN, 2, 2);
    gob2 = creature(CR_GOBLIN, 8, 8);
    events();                                               /* the player's innate effect started */
    a = nge_entity_create();                                /* no components */
    CHECK(ai_can(a) == 0);
    CHECK(ai_can(rat) == AI_CAN_MELEE);                     /* a bite; Int 2, no inventory */
    CHECK(ai_can(goblin) == (AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP | AI_CAN_USE | AI_CAN_OPEN));
    /* The thresholds, on the ability's value, as an effect would change it */
    stats_ability[ABIL_INT][goblin] = AI_INT_OPEN;
    CHECK(ai_can(goblin) & AI_CAN_OPEN);
    stats_ability[ABIL_INT][goblin] = AI_INT_OPEN - 1;
    CHECK(!(ai_can(goblin) & AI_CAN_OPEN) && (ai_can(goblin) & AI_CAN_USE));   /* 5: uses, can't open */
    stats_ability[ABIL_INT][goblin] = AI_INT_USE - 1;
    CHECK(ai_can(goblin) == (AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP));    /* 4: neither */
    stats_ability[ABIL_INT][goblin] = 10;
    /* Int with no inventory: opens, but nothing to use */
    stats_ability[ABIL_INT][rat] = AI_INT_OPEN;
    CHECK(ai_can(rat) == (AI_CAN_MELEE | AI_CAN_OPEN));
    stats_ability[ABIL_INT][rat] = 2;
    /* An inventory with no slots: picks up, can't equip; no stats: no Int */
    holder = nge_entity_create();
    nge_location_set(holder, 0, 9);
    inv_add(holder, 4, 0);
    CHECK(ai_can(holder) == AI_CAN_PICKUP);
    /* With a weapon slot, and melee only while a sword is wielded */
    nge_entity_destroy(holder);
    nge_entity_release(nge_entity_next_dying());
    holder = nge_entity_create();
    nge_location_set(holder, 0, 9);
    inv_add(holder, 4, SLOT_BIT(SLOT_WEAPON));
    CHECK(ai_can(holder) == (AI_CAN_PICKUP | AI_CAN_EQUIP));
    sword = gen_item(IT_SWORD, AREA);
    inv_stow(sword, holder);
    CHECK(!(ai_can(holder) & AI_CAN_MELEE));                /* carried, not wielded */
    c = ctx(holder);
    c.item = sword;
    c.kind = SLOT_NONE;
    CHECK(act_equip(&c, ACT_DO) == ACT_SUCCESS);
    CHECK(ai_can(holder) == (AI_CAN_MELEE | AI_CAN_PICKUP | AI_CAN_EQUIP));
    c = ctx(holder);
    c.item = sword;
    CHECK(act_unequip(&c, ACT_DO) == ACT_SUCCESS);
    CHECK(!(ai_can(holder) & AI_CAN_MELEE));
    events();
    /* Asserts: not alive */
    TEST_EXPECT_HALT(ai_can(NGE_ENTITY_MAX));
    CHECK(test_halt_count == 1);

    /* ---- events: attacked wakes it to hunt the attacker, once; a change of state is an event ---- */
    CHECK(ai_get_state(rat) == AI_STATE_ASLEEP && ai_target[rat] == NGE_ENTITY_INVALID &&
          ai_goal_x[rat] == NGE_MAP_X_INVALID && ai_goal_y[rat] == NGE_MAP_Y_INVALID);   /* ai_add(): none */
    nge_event_post(rat, EV_HIT, player, 15, NGE_EVENT_NONE);
    nge_event_post(rat, EV_DAMAGED, player, 1, 0);
    CHECK(events() == 3 && is(0, rat, EV_HIT, player) && is(1, rat, EV_DAMAGED, player) &&
          is(2, rat, EV_AI_CHANGED, player));
    CHECK(seen[2].p1 == AI_STATE_ASLEEP && seen[2].p2 == AI_STATE_HUNT);
    CHECK(ai_get_state(rat) == AI_STATE_HUNT);
    CHECK(ai_target[rat] == player && ai_goal_x[rat] == 4 && ai_goal_y[rat] == 5);   /* the attacker's tile */
    CHECK(ai_track[rat] == AI_TRACK);                         /* and it follows them a while */
    nge_event_post(rat, EV_HIT, player, 15, NGE_EVENT_NONE);   /* hunting: hurt changes nothing */
    CHECK(events() == 1 && ai_get_state(rat) == AI_STATE_HUNT);
    /* A miss wakes it too; damage with no source (poison) too, with no one to hunt */
    nge_event_post(goblin, EV_MISSED, player, 3, NGE_EVENT_NONE);
    CHECK(events() == 2 && is(1, goblin, EV_AI_CHANGED, player) && ai_get_state(goblin) == AI_STATE_HUNT);
    nge_event_post(gob2, EV_DAMAGED, NGE_EVENT_NONE, 1, 0);
    CHECK(events() == 2 && is(1, gob2, EV_AI_CHANGED, NGE_EVENT_NONE) && ai_get_state(gob2) == AI_STATE_HUNT);
    CHECK(ai_target[gob2] == NGE_ENTITY_INVALID && ai_goal_x[gob2] == NGE_MAP_X_INVALID);
    /* Other events, entities without AI, no target: nothing */
    b = creature(CR_RAT, 1, 8);
    nge_event_post(b, EV_MOVED, NGE_EVENT_NONE, 1, 0);
    nge_event_post(b, EV_DIED, player, 0, 0);
    nge_event_post(player, EV_HIT, b, 15, NGE_EVENT_NONE);
    nge_event_post(NGE_EVENT_NONE, EV_DAMAGED, player, 1, 0);
    CHECK(events() == 4 && ai_get_state(b) == AI_STATE_ASLEEP);
    /* Killed by the blow: dying when its events come, so it doesn't wake */
    nge_entity_destroy(b);
    nge_event_post(b, EV_HIT, player, 15, NGE_EVENT_NONE);
    nge_event_post(b, EV_DAMAGED, player, 3, 0);
    CHECK(events() == 2 && ai_get_state(b) == AI_STATE_ASLEEP);
    nge_entity_release(nge_entity_next_dying());

    /* ---- ai_try(): checked, then done; refused changes nothing ---- */
    c = ctx(rat);
    c.dx = 0;
    c.dy = 1;
    CHECK(ai_try(ACT_MOVE, &c) == 1 && c.reason == ACT_R_NONE && at(rat, 6, 6));
    CHECK(events() == 1 && is(0, rat, EV_MOVED, NGE_EVENT_NONE));
    c = ctx(rat);
    c.dx = -1;
    c.dy = -1;
    CHECK(ai_try(ACT_MOVE, &c) == 1 && at(rat, 5, 5));
    events();
    c = ctx(rat);
    c.dx = 0;
    c.dy = -1;                                              /* the wall at (5, 4) */
    CHECK(ai_try(ACT_MOVE, &c) == 0 && c.reason == ACT_R_BLOCKED && c.used == 0 && at(rat, 5, 5));
    CHECK(events() == 0);
    c = ctx(rat);
    c.dx = -1;
    c.dy = 0;                                               /* the player at (4, 5) */
    CHECK(ai_try(ACT_MOVE, &c) == 0 && c.reason == ACT_R_BLOCKED_ENTITY && c.target == player && at(rat, 5, 5));
    CHECK(events() == 0);

    /* ======== the default brain's turns (docs/ai_default_brain.md) ======== */

    /* ---- sight: range, walls, closed doors; creatures don't block; another area ---- */
    scene(0, 5);
    CHECK(ai_in_sight(AREA, 0, 5, 8, 5) == 1);
    CHECK(ai_in_sight(AREA, 0, 5, 9, 5) == 0);              /* 9 tiles: beyond AI_SIGHT */
    NGE_MAP_TILE(4, 5).tile_id = WALL;
    CHECK(ai_in_sight(AREA, 0, 5, 8, 5) == 0);
    NGE_MAP_TILE(4, 5).tile_id = FLOOR;
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 4, 5);
    CHECK(ai_in_sight(AREA, 0, 5, 8, 5) == 0);              /* a closed door */
    feature_open(door);
    CHECK(ai_in_sight(AREA, 0, 5, 8, 5) == 1);
    rat = creature(CR_RAT, 3, 5);
    CHECK(ai_in_sight(AREA, 0, 5, 8, 5) == 1);              /* a creature between */
    CHECK(ai_sees(actor_player, rat) && ai_sees(rat, actor_player));
    nge_owner_set(rat, AREA1);                              /* another level */
    CHECK(!ai_sees(actor_player, rat) && !ai_sees(rat, actor_player));
    CHECK(!ai_sees(NGE_ENTITY_INVALID, actor_player) && !ai_sees(actor_player, NGE_ENTITY_INVALID));
    events();

    /* ---- hearing: asleep, within AI_HEAR, 1d20 + the Wisdom modifier against AI_WAKE_DC ---- */
    scene(0, 0);
    rat = creature(CR_RAT, 3, 0);                           /* Wis 10: no modifier */
    srand(seed_for_roll(AI_WAKE_DC - 1));
    ai_on_turn(rat);
    CHECK(ai_get_state(rat) == AI_STATE_ASLEEP && at(rat, 3, 0) && events() == 0);   /* waited */
    srand(seed_for_roll(AI_WAKE_DC));
    ai_on_turn(rat);
    CHECK(ai_get_state(rat) == AI_STATE_IDLE);              /* woken: idle acts this turn */
    CHECK(events() >= 1 && is(0, rat, EV_AI_CHANGED, NGE_EVENT_NONE) && seen[0].p1 == AI_STATE_ASLEEP &&
          seen[0].p2 == AI_STATE_IDLE);
    goblin = creature(CR_GOBLIN, 0, 3);                     /* Wis 8: -1 */
    srand(seed_for_roll(AI_WAKE_DC));
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_ASLEEP);
    srand(seed_for_roll(AI_WAKE_DC + 1));
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_IDLE);
    events();
    /* Out of AI_HEAR: no roll at all */
    b = creature(CR_RAT, 9, 9);
    srand(9);
    r = rand();
    srand(9);
    ai_on_turn(b);
    CHECK(rand() == r && ai_get_state(b) == AI_STATE_ASLEEP && at(b, 9, 9) && events() == 0);

    /* ---- the frame: an idle entity that sees the player hunts at once, and comes ---- */
    scene(0, 5);
    goblin = creature(CR_GOBLIN, 5, 5);
    ai_state[goblin] = AI_STATE_IDLE;
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_HUNT && at(goblin, 4, 5));
    CHECK(ai_target[goblin] == actor_player && ai_goal_x[goblin] == 0 && ai_goal_y[goblin] == 5);
    CHECK(events() == 2 && is(0, goblin, EV_AI_CHANGED, actor_player) && seen[0].p1 == AI_STATE_IDLE &&
          seen[0].p2 == AI_STATE_HUNT && is(1, goblin, EV_MOVED, NGE_EVENT_NONE));
    /* Seeing them again changes nothing: it keeps coming, then attacks */
    ai_on_turn(goblin);
    ai_on_turn(goblin);
    ai_on_turn(goblin);
    CHECK(at(goblin, 1, 5) && ai_get_state(goblin) == AI_STATE_HUNT && events() == 3);
    ai_on_turn(goblin);
    CHECK(at(goblin, 1, 5) && events() >= 1 &&
          (is(0, actor_player, EV_HIT, goblin) || is(0, actor_player, EV_MISSED, goblin)));

    /* ---- lost: at the goal with the target out of sight, or no way towards it ---- */
    scene(0, 0);
    wall_column(3, 0, 9);                                   /* hides the player */
    goblin = hunter(CR_GOBLIN, 5, 5, 5, 5);
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_IDLE && at(goblin, 5, 5) && ai_goal_x[goblin] == NGE_MAP_X_INVALID);
    CHECK(ai_target[goblin] == actor_player);               /* kept: it may see them again */
    CHECK(events() == 1 && is(0, goblin, EV_AI_CHANGED, NGE_EVENT_NONE) && seen[0].p1 == AI_STATE_HUNT &&
          seen[0].p2 == AI_STATE_IDLE);
    /* The direct step and both beside it blocked */
    NGE_MAP_TILE(6, 4).tile_id = WALL;
    NGE_MAP_TILE(6, 5).tile_id = WALL;
    NGE_MAP_TILE(6, 6).tile_id = WALL;
    ai_state[goblin] = AI_STATE_HUNT;
    ai_goal_x[goblin] = 8;
    ai_goal_y[goblin] = 5;
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_IDLE && at(goblin, 5, 5) && events() == 1);
    /* Only the direct step blocked, by a creature: 45 degrees left; by a wall: the same */
    NGE_MAP_TILE(6, 4).tile_id = FLOOR;
    NGE_MAP_TILE(6, 5).tile_id = FLOOR;
    NGE_MAP_TILE(6, 6).tile_id = FLOOR;
    rat = creature(CR_RAT, 6, 5);
    ai_state[goblin] = AI_STATE_HUNT;
    ai_goal_x[goblin] = 8;
    ai_goal_y[goblin] = 5;
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_HUNT && at(goblin, 6, 4));
    NGE_MAP_TILE(7, 5).tile_id = WALL;                      /* south-east: east instead */
    ai_on_turn(goblin);
    CHECK(ai_get_state(goblin) == AI_STATE_HUNT && at(goblin, 7, 4));
    events();

    /* ---- doors: a goblin opens one in its way; a rat, or a dull goblin, can't, and gives up ---- */
    for (s = 0; s < 3; s++) {
        scene(8, 5);
        wall_column(5, 0, 9);
        NGE_MAP_TILE(5, 5).tile_id = FLOOR;
        door = gen_feature(FT_DOOR, AREA);
        feature_place(door, 5, 5);
        a = hunter(s == 1 ? CR_RAT : CR_GOBLIN, 4, 5, 8, 5);
        if (s == 2) {
            stats_ability[ABIL_INT][a] = AI_INT_OPEN - 1;
        }
        events();
        ai_on_turn(a);
        if (s == 0) {
            CHECK(FEATURE_IS_OPEN(door) && at(a, 4, 5) && ai_get_state(a) == AI_STATE_HUNT);
            CHECK(events() == 1 && is(0, door, EV_OPENED, a));
        } else {
            CHECK(!FEATURE_IS_OPEN(door) && at(a, 4, 5) && ai_get_state(a) == AI_STATE_IDLE);
            CHECK(events() == 1 && is(0, a, EV_AI_CHANGED, NGE_EVENT_NONE));
        }
    }

    /* ---- doors off the straight line: seen through the doorway, then the door shut ---- */
    scene(8, 4);
    wall_column(4, 0, 9);
    NGE_MAP_TILE(4, 2).tile_id = FLOOR;
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 4, 2);
    a = hunter(CR_GOBLIN, 0, 0, 8, 4);                      /* saw the player through the doorway, now shut */
    events();
    for (s = 0; s < 12 && !FEATURE_IS_OPEN(door) && ai_get_state(a) == AI_STATE_HUNT; s++) {
        ai_on_turn(a);
    }
    CHECK(FEATURE_IS_OPEN(door) && at(a, 3, 1) && ai_get_state(a) == AI_STATE_HUNT);   /* along the line, not into the wall */
    events();

    /* ---- the door shut in its face (CSpect check 3): it opens it next turn ---- */
    scene(5, 6);
    for (s = 0; s < 10; s++) {
        NGE_MAP_TILE(s, 5).tile_id = WALL;
    }
    NGE_MAP_TILE(5, 5).tile_id = FLOOR;
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 5, 5);
    feature_open(door);
    a = creature(CR_GOBLIN, 5, 1);
    ai_state[a] = AI_STATE_IDLE;
    events();
    for (s = 0; s < 6 && !at(a, 5, 4); s++) {
        ai_on_turn(a);                                      /* sees the player through the doorway, comes */
    }
    CHECK(at(a, 5, 4) && ai_get_state(a) == AI_STATE_HUNT && ai_goal_x[a] == 5 && ai_goal_y[a] == 6);
    c = ctx(actor_player);
    c.target = door;
    CHECK(act_close(&c, ACT_DO) == ACT_SUCCESS);
    events();
    ai_on_turn(a);
    CHECK(FEATURE_IS_OPEN(door) && at(a, 5, 4) && ai_get_state(a) == AI_STATE_HUNT);
    CHECK(events() == 1 && is(0, door, EV_OPENED, a));

    /* ---- tracking: seen on its side, then gone through the door and shut it: followed ---- */
    scene(5, 4);
    for (s = 0; s < 10; s++) {
        NGE_MAP_TILE(s, 5).tile_id = WALL;
    }
    NGE_MAP_TILE(5, 5).tile_id = FLOOR;
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 5, 5);
    feature_open(door);
    a = creature(CR_GOBLIN, 5, 0);
    ai_state[a] = AI_STATE_IDLE;
    ai_on_turn(a);                                          /* sees the player at (5, 4): goes to (5, 1) */
    CHECK(ai_get_state(a) == AI_STATE_HUNT && ai_goal_y[a] == 4 && ai_track[a] == AI_TRACK && at(a, 5, 1));
    nge_location_set(actor_player, 5, 7);                   /* through the door, two tiles on... */
    feature_close(door);                                    /* ...and shut it, before its next look */
    events();
    for (s = 0; s < 6 && !FEATURE_IS_OPEN(door); s++) {
        ai_on_turn(a);
    }
    CHECK(FEATURE_IS_OPEN(door) && at(a, 5, 4) && ai_get_state(a) == AI_STATE_HUNT);
    CHECK(ai_goal_x[a] == 5 && ai_goal_y[a] == 7);         /* from where it saw them, to where they went */
    events();
    /* Out through the south door, then left out of sight: it goes through the doorway after them */
    scene(5, 6);
    for (s = 0; s < 10; s++) {
        NGE_MAP_TILE(s, 5).tile_id = WALL;
    }
    NGE_MAP_TILE(5, 5).tile_id = FLOOR;                    /* the doorway, its door open */
    a = creature(CR_GOBLIN, 5, 1);
    ai_state[a] = AI_STATE_IDLE;
    ai_on_turn(a);                                          /* sees them beyond the doorway */
    CHECK(ai_get_state(a) == AI_STATE_HUNT && ai_goal_x[a] == 5 && ai_goal_y[a] == 6);
    nge_location_set(actor_player, 2, 6);                   /* left, behind the wall */
    for (s = 0; s < 8 && NGE_LOCATION_Y(a) < 6 && ai_get_state(a) == AI_STATE_HUNT; s++) {
        ai_on_turn(a);
    }
    CHECK(ai_get_state(a) == AI_STATE_HUNT && NGE_LOCATION_Y(a) == 6);   /* through, not lost at the wall */
    ai_on_turn(a);
    CHECK(ai_get_state(a) == AI_STATE_HUNT && ai_goal_x[a] == 2 && ai_goal_y[a] == 6);   /* and sees them */
    events();

    /* On the way to the last sighting, unseen: the goal stays there, and tracking waits */
    scene(0, 0);
    wall_column(3, 0, 9);
    a = hunter(CR_GOBLIN, 6, 9, 6, 5);
    ai_track[a] = AI_TRACK;
    nge_location_set(actor_player, 0, 9);
    ai_on_turn(a);
    CHECK(at(a, 6, 8) && ai_goal_x[a] == 6 && ai_goal_y[a] == 5 && ai_track[a] == AI_TRACK);
    /* At it: the goal follows for AI_TRACK turns, then stays */
    scene(0, 0);
    wall_column(3, 0, 9);
    a = hunter(CR_GOBLIN, 6, 9, 6, 9);
    ai_track[a] = AI_TRACK;
    for (s = 0; s < AI_TRACK; s++) {
        nge_location_set(actor_player, 0, (uint8_t)(s + 1));
        ai_on_turn(a);
        CHECK(ai_goal_x[a] == 0 && ai_goal_y[a] == s + 1 && ai_track[a] == AI_TRACK - 1 - s);
    }
    nge_location_set(actor_player, 1, 8);
    ai_on_turn(a);
    CHECK(ai_goal_x[a] == 0 && ai_goal_y[a] == AI_TRACK && ai_track[a] == 0);
    /* Lost: no tracking left to pick it up again */
    ai_stimulus(a, AI_STIM_LOST, NGE_EVENT_NONE);
    CHECK(ai_track[a] == 0 && ai_goal_x[a] == NGE_MAP_X_INVALID);
    events();

    /* ---- attack: next to its target, a rat bites ---- */
    scene(4, 5);
    rat = hunter(CR_RAT, 5, 5, 4, 5);
    ai_on_turn(rat);
    CHECK(at(rat, 5, 5) && events() >= 1 &&
          (is(0, actor_player, EV_HIT, rat) || is(0, actor_player, EV_MISSED, rat)));

    /* ---- idle: a goblin picks up what it stands on, then wields it; a rat doesn't ---- */
    scene(0, 0);
    goblin = creature(CR_GOBLIN, 9, 9);                     /* out of the player's sight */
    ai_state[goblin] = AI_STATE_IDLE;
    sword = gen_item(IT_SWORD, AREA);
    nge_location_set(sword, 9, 9);
    events();
    ai_on_turn(goblin);
    CHECK(NGE_OWNER_OF(sword) == goblin && at(goblin, 9, 9));
    CHECK(events() >= 1 && is(0, sword, EV_PICKED_UP, goblin));
    ai_on_turn(goblin);
    CHECK(item_slot[sword] == SLOT_WEAPON && attack_weapon(goblin) == sword && at(goblin, 9, 9));
    CHECK(events() == 1 && is(0, sword, EV_EQUIPPED, goblin) && seen[0].p1 == SLOT_WEAPON);
    srand(seed_no_wander());                                /* nothing left to do: waits */
    ai_on_turn(goblin);
    CHECK(at(goblin, 9, 9) && events() == 0);
    rat = creature(CR_RAT, 8, 9);
    ai_state[rat] = AI_STATE_IDLE;
    b = gen_item(IT_SWORD, AREA);
    nge_location_set(b, 8, 9);
    events();
    srand(seed_no_wander());
    ai_on_turn(rat);
    CHECK(NGE_OWNER_OF(b) == AREA && at(rat, 8, 9) && events() == 0);

    /* ---- hunting: a healing potion at half hit points or less, with the wit to use it ---- */
    scene(0, 5);
    goblin = hunter(CR_GOBLIN, 5, 5, 0, 5);
    potion = gen_item(IT_POTION, AREA);
    inv_stow(potion, goblin);
    health_hp[goblin] = (uint8_t)(health_hp_max[goblin] / 2 + 1);   /* above half: comes on */
    events();
    ai_on_turn(goblin);
    CHECK(at(goblin, 4, 5) && NGE_ENTITY_IS_ALIVE(potion));
    events();
    health_hp[goblin] = (uint8_t)(health_hp_max[goblin] / 2);
    stats_ability[ABIL_INT][goblin] = AI_INT_USE - 1;      /* half, but no AI_CAN_USE */
    ai_on_turn(goblin);
    CHECK(at(goblin, 3, 5) && NGE_ENTITY_IS_ALIVE(potion));
    events();
    stats_ability[ABIL_INT][goblin] = 10;
    hp = health_hp[goblin];
    ai_on_turn(goblin);
    CHECK(at(goblin, 3, 5) && !NGE_ENTITY_IS_ALIVE(potion) && health_hp[goblin] > hp);
    CHECK(events() >= 1 && is(0, potion, EV_USED, goblin));
    /* A potion of strength: not for healing */
    potion = gen_item(IT_STRENGTH, AREA);
    inv_stow(potion, goblin);
    health_hp[goblin] = 1;
    ai_on_turn(goblin);
    CHECK(at(goblin, 2, 5) && NGE_ENTITY_IS_ALIVE(potion));
    events();

    /* ---- a target gone (on another level, or dead) is dropped; hurt by no one, it hunts no one ---- */
    scene(0, 0);
    wall_column(3, 0, 9);
    rat = creature(CR_RAT, 8, 8);
    goblin = hunter(CR_GOBLIN, 5, 5, 8, 8);
    ai_target[goblin] = rat;
    nge_owner_set(rat, AREA1);
    ai_on_turn(goblin);
    CHECK(ai_target[goblin] == NGE_ENTITY_INVALID && ai_get_state(goblin) == AI_STATE_IDLE && at(goblin, 5, 5));
    b = creature(CR_RAT, 8, 7);
    ai_state[goblin] = AI_STATE_HUNT;
    ai_target[goblin] = b;
    ai_goal_x[goblin] = 8;
    ai_goal_y[goblin] = 7;
    nge_entity_destroy(b);
    ai_on_turn(goblin);
    CHECK(ai_target[goblin] == NGE_ENTITY_INVALID && ai_get_state(goblin) == AI_STATE_IDLE && at(goblin, 5, 5));
    nge_entity_release(nge_entity_next_dying());
    gob2 = creature(CR_GOBLIN, 7, 2);
    events();
    nge_event_post(gob2, EV_DAMAGED, NGE_EVENT_NONE, 1, 0);
    events();
    CHECK(ai_get_state(gob2) == AI_STATE_HUNT && ai_target[gob2] == NGE_ENTITY_INVALID);
    ai_on_turn(gob2);
    CHECK(ai_get_state(gob2) == AI_STATE_IDLE && at(gob2, 7, 2));

    /* ---- asserts: a bad state, no AI ---- */
    ai_state[gob2] = AI_STATE_NONE;
    TEST_EXPECT_HALT(ai_on_turn(gob2));
    CHECK(test_halt_count == 1);
    ai_state[gob2] = AI_STATE_IDLE;
    TEST_EXPECT_HALT(ai_on_turn(actor_player));
    CHECK(test_halt_count == 1);

    return test_summary("ai");
}
