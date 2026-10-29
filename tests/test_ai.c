/* game/ai: the AI component, its brains, capabilities, events, turns and ai_try(). */
#include "test.h"

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "map/nge_map.h"

#include "game/ai/ai_internal.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/stats.h"
#include "game/events.h"
#include "game/gen/gen.h"

#define AREA   NGE_OWNER_AREA(0)
#define FLOOR  '.'
#define WALL   '#'

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
    act_t c;
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
            }
            ok = ok && ai_brains[k.brain].next[s][AI_STIM_NONE] == AI_STATE_NONE;
            ok = ok && (k.brain != AI_BRAIN_NONE || ai_brains[k.brain].next[s][AI_STIM_HURT] == AI_STATE_NONE);
        }
    }
    CHECK(ok);
    CHECK(ai_brains[AI_BRAIN_DEFAULT].next[AI_STATE_ASLEEP][AI_STIM_HURT] == AI_STATE_IDLE);
    CHECK(ai_brains[AI_BRAIN_DEFAULT].next[AI_STATE_IDLE][AI_STIM_HURT] == AI_STATE_NONE);

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

    /* ---- events: attacked wakes it, once; a change of state is an event ---- */
    CHECK(ai_get_state(rat) == AI_STATE_ASLEEP);
    nge_event_post(rat, EV_HIT, player, 15, NGE_EVENT_NONE);
    nge_event_post(rat, EV_DAMAGED, player, 1, 0);
    CHECK(events() == 3 && is(0, rat, EV_HIT, player) && is(1, rat, EV_DAMAGED, player) &&
          is(2, rat, EV_AI_CHANGED, player));
    CHECK(seen[2].p1 == AI_STATE_ASLEEP && seen[2].p2 == AI_STATE_IDLE);
    CHECK(ai_get_state(rat) == AI_STATE_IDLE);
    nge_event_post(rat, EV_HIT, player, 15, NGE_EVENT_NONE);   /* awake: hurt changes nothing */
    CHECK(events() == 1 && ai_get_state(rat) == AI_STATE_IDLE);
    /* A miss wakes it too; damage with no source (poison) too */
    nge_event_post(goblin, EV_MISSED, player, 3, NGE_EVENT_NONE);
    CHECK(events() == 2 && is(1, goblin, EV_AI_CHANGED, player) && ai_get_state(goblin) == AI_STATE_IDLE);
    nge_event_post(gob2, EV_DAMAGED, NGE_EVENT_NONE, 1, 0);
    CHECK(events() == 2 && is(1, gob2, EV_AI_CHANGED, NGE_EVENT_NONE) && ai_get_state(gob2) == AI_STATE_IDLE);
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

    /* ---- turns: asleep and idle do nothing ---- */
    b = creature(CR_RAT, 1, 8);
    ai_on_turn(b);                                          /* asleep */
    ai_on_turn(rat);                                        /* idle */
    CHECK(events() == 0 && at(b, 1, 8) && at(rat, 6, 5));
    CHECK(ai_get_state(b) == AI_STATE_ASLEEP && ai_get_state(rat) == AI_STATE_IDLE);
    ai_state[b] = AI_STATE_NONE;                            /* a bad state */
    TEST_EXPECT_HALT(ai_on_turn(b));
    CHECK(test_halt_count == 1);
    ai_state[b] = AI_STATE_ASLEEP;
    TEST_EXPECT_HALT(ai_on_turn(player));                   /* no AI */
    CHECK(test_halt_count == 1);

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

    return test_summary("ai");
}
