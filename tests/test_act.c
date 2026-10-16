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

#include "game/actions/act.h"
#include "game/components/stats.h"
#include "game/events.h"
#include "game/gen/gen.h"

#define AREA   NGE_OWNER_AREA(0)
#define FLOOR  '.'
#define WALL   '#'

static const nge_rect_t window = { .x1 = 0, .x2 = 9, .y1 = 0, .y2 = 9 };

/* The events posted since the last call */
static nge_event_t seen[16];
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

int main(void)
{
    nge_entity_id_t player;
    nge_entity_id_t rat;
    nge_entity_id_t potion;
    act_t a;
    uint8_t x;
    uint8_t y;
    int r1;
    int r2;
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
    player = creature(CR_PLAYER, 4, 5);
    rat = creature(CR_RAT, 6, 5);
    potion = gen_item(IT_POTION, AREA);
    nge_location_set(potion, 5, 5);

    /* ---- a move: the location, the result, the turn used, the events ---- */
    a = move(player, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS);
    CHECK(a.reason == ACT_R_NONE && a.used == 1 && a.target == NGE_ENTITY_INVALID);
    CHECK(at(player, 5, 5));
    CHECK(events() == 2 && is(0, player, EV_MOVED, NGE_EVENT_NONE) && is(1, potion, EV_STOOD_ON, player));
    /* Diagonally, onto empty floor: EV_MOVED only */
    a = move(player, -1, 1);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(player, 4, 6));
    CHECK(events() == 1 && is(0, player, EV_MOVED, NGE_EVENT_NONE));
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
    CHECK(a.reason == ACT_R_BLOCKED_CREATURE && a.target == rat && a.used == 0);
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
    ok = ok && act_move(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_BLOCKED_CREATURE && a.target == rat;
    r2 = rand();
    CHECK(ok);
    CHECK(at(player, 5, 5) && events() == 0 && r1 == r2);

    /* ---- act_do: the same as calling the action ---- */
    a = move(player, 0, 1);
    CHECK(act_do(ACT_MOVE, &a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_BLOCKED_CREATURE && a.target == rat);
    a = move(player, -1, 0);
    CHECK(act_do(ACT_MOVE, &a, ACT_DO) == ACT_SUCCESS && at(player, 4, 5) && a.used == 1);
    events();

    /* ---- every reason so far is refused ---- */
    CHECK(!act_attempted(ACT_R_NONE) && !act_attempted(ACT_R_BLOCKED) && !act_attempted(ACT_R_BLOCKED_CREATURE));

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

    return test_summary("act");
}
