/* game/noise: the noise table and mask, the noises events make, and the AI's records (ai_hear()). */
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

#include "game/actions/act.h"
#include "game/ai/ai_internal.h"
#include "game/components/feature.h"
#include "game/components/pc.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/noise/noise.h"

#define AREA   NGE_OWNER_AREA(0)
#define AREA1  NGE_OWNER_AREA(1)
#define FLOOR  '.'
#define WALL   '#'
#define SIZE   20

/* The player (game/actors.c isn't linked) */
nge_entity_id_t actor_player;

static const nge_rect_t window = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 19 };

/* The screen's map view, for the level module (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

static void drop(const nge_event_t *e)
{
    (void)e;
}

/* A creature of kind in area at (x, y), with no record */
static nge_entity_id_t creature_in(uint8_t kind, nge_owner_t area, uint8_t x, uint8_t y)
{
    nge_entity_id_t id = gen_creature(kind, area);

    nge_location_set(id, x, y);
    nge_event_process(drop);   /* innate effects */
    ai_heard[id] = 0;
    return id;
}

static nge_entity_id_t creature(uint8_t kind, uint8_t x, uint8_t y)
{
    return creature_in(kind, AREA, x, y);
}

/* A feature of kind at (x, y) */
static nge_entity_id_t feature(uint8_t kind, uint8_t x, uint8_t y)
{
    nge_entity_id_t id = gen_feature(kind, AREA);

    feature_place(id, x, y);
    nge_event_process(drop);
    return id;
}

/* A fresh room of floor, SIZE x SIZE, with nothing in it but the player at (px, py) */
static void scene(uint8_t px, uint8_t py)
{
    uint8_t x;
    uint8_t y;

    for (y = 0; y < SIZE; y++) {
        for (x = 0; x < SIZE; x++) {
            NGE_MAP_TILE(x, y).tile_id = FLOOR;
        }
    }
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    actor_player = creature(CR_PLAYER, px, py);
}

/* Make the noise of event id, by the row alone (noise_event()) */
static void noise(nge_entity_id_t target, nge_event_id_t id, nge_entity_id_t source, uint8_t p1, uint8_t p2)
{
    nge_event_t e;

    e.target = target;
    e.id = id;
    e.source = source;
    e.p1 = p1;
    e.p2 = p2;
    noise_event(&e);
}

/* The same through the filter (noise_on_event()) */
static void filtered(nge_entity_id_t target, nge_event_id_t id, nge_entity_id_t source)
{
    nge_event_t e;

    e.target = target;
    e.id = id;
    e.source = source;
    e.p1 = 0;
    e.p2 = 0;
    noise_on_event(&e);
}

/* As the game's dispatcher: the AI, then the noises */
static void dispatch(const nge_event_t *e)
{
    ai_on_event(e);
    noise_on_event(e);
}

/* A seed whose first rand() gives the d20 roll r: the notice roll is a turn's first draw */
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

/* id's record: heard loudness h (heard + 1 stored) at (x, y) */
static int record(nge_entity_id_t id, uint8_t h, uint8_t x, uint8_t y)
{
    return ai_heard[id] == h + 1 && ai_heard_x[id] == x && ai_heard_y[id] == y;
}

static int mask_bit(uint8_t i)
{
    return (noise_mask[i >> 3] >> (i & 7)) & 1;
}

int main(void)
{
    static const ai_kind_t idle = { .brain = AI_BRAIN_DEFAULT, .state = AI_STATE_IDLE };
    static const nge_event_id_t loud[] = {
        EV_MOVED, EV_HIT, EV_MISSED, EV_DIED, EV_FIRED, EV_CAST, EV_AREA, EV_TRIGGERED,
        EV_OPENED, EV_CLOSED, EV_UNLOCKED,
    };
    nge_entity_id_t m;
    nge_entity_id_t h;
    nge_entity_id_t h2;
    nge_entity_id_t far;
    nge_entity_id_t other;
    nge_entity_id_t door;
    nge_entity_id_t trap;
    act_t a;
    uint8_t i;
    uint8_t j;
    uint8_t y;
    int ok;
    int set;

    test_init();

    nge_map_init(SIZE, SIZE, &window);
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    nge_map_set_block(FLOOR, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_SHUT, NGE_BLOCK_NONE);   /* as the level sets them: the door blocks */
    nge_map_set_block(LOOK_DOOR_OPEN, NGE_BLOCK_NONE);

    /* ---- the mask: the rows with a loudness, and no others; again, the same ---- */
    noise_mask[0] = 0xFF;   /* stale: init clears it */
    noise_init();
    ok = 1;
    for (i = 0; i < NOISE_EVENTS; i++) {
        set = 0;
        for (j = 0; j < sizeof(loud) / sizeof(loud[0]); j++) {
            set = set || loud[j] == NGE_EVENT_USER + i;
        }
        ok = ok && mask_bit(i) == set;
    }
    CHECK(ok);
    CHECK(mask_bit(EV_DAMAGED - NGE_EVENT_USER) == 0);   /* follows a hit; ticks from poison */
    noise_init();
    CHECK(mask_bit(EV_HIT - NGE_EVENT_USER) == 1 && mask_bit(EV_LOOKED - NGE_EVENT_USER) == 0);

    /* ---- who is involved, and where (R4, R5) ---- */
    scene(2, 5);
    CHECK(PC_IS(actor_player));
    m = creature(CR_RAT, 5, 5);
    h = creature(CR_RAT, 5, 8);
    CHECK(AI_IS(m) && AI_IS(h));

    /* The player hitting a monster: at the target, the monster */
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);
    CHECK(record(h, 6 - 3, 5, 5));
    ai_heard[h] = 0;
    noise(m, EV_MISSED, actor_player, 2, NGE_EVENT_NONE);
    CHECK(record(h, 6 - 3, 5, 5));
    ai_heard[h] = 0;
    /* A monster hitting the player: at the target, the player */
    noise(actor_player, EV_HIT, m, 15, NGE_EVENT_NONE);
    CHECK(ai_heard_x[h] == 2 && ai_heard_y[h] == 5 && ai_heard[h] != 0);
    ai_heard[h] = 0;
    /* A monster hitting a monster: not heard (D6) */
    noise(h, EV_HIT, m, 15, NGE_EVENT_NONE);
    CHECK(ai_heard[h] == 0 && ai_heard[m] == 0);

    /* Doors: a monster opening one is silent, the player isn't */
    door = feature(FT_DOOR, 5, 6);
    noise(door, EV_OPENED, m, 0, 0);
    CHECK(ai_heard[h] == 0);
    noise(door, EV_OPENED, actor_player, 0, 0);
    CHECK(record(h, 3 - 2, 5, 6));
    ai_heard[h] = 0;
    noise(door, EV_UNLOCKED, actor_player, 0, 0);
    CHECK(record(h, 2 - 2, 5, 6));
    ai_heard[h] = 0;

    /* A trap set off by a monster: at the trap (any feature: only its tile counts) */
    trap = feature(FT_STATUE, 5, 10);
    ai_heard[m] = 0;                                        /* heard the doors */
    noise(trap, EV_TRIGGERED, m, 0, 0);
    CHECK(record(h, 8 - 2, 5, 10));
    CHECK(ai_heard[m] == 0);                                /* the source: never */
    ai_heard[h] = 0;
    ai_heard[m] = 0;

    /* A burst: at (p1, p2), whoever made it; its user doesn't hear it */
    noise(NGE_EVENT_NONE, EV_AREA, m, 5, 7);
    CHECK(record(h, 8 - 1, 5, 7) && ai_heard[m] == 0);
    ai_heard[h] = 0;

    /* A shot and a spell: at the source */
    noise(m, EV_FIRED, actor_player, 0, 0);
    CHECK(ai_heard[h] == 0);                                /* 3, from range 4 */
    noise(m, EV_CAST, actor_player, 0, 0);
    CHECK(record(h, 5 - 4, 2, 5));                          /* (2, 5) to (5, 8): range 4 */
    ai_heard[h] = 0;

    /* A step by the player, not by a monster; a teleport (no move) is silent */
    noise(actor_player, EV_MOVED, NGE_EVENT_NONE, 0, 0);
    CHECK(ai_heard[h] == 0);
    noise(m, EV_MOVED, NGE_EVENT_NONE, 1, 0);
    CHECK(ai_heard[h] == 0);
    noise(actor_player, EV_MOVED, NGE_EVENT_NONE, 1, (uint8_t)-1);
    CHECK(ai_heard[m] == noise_step_loudness() - 3 + 1 && ai_heard_x[m] == 2);
    ai_heard[m] = 0;
    ai_heard[h] = 0;

    /* A death cry, from a creature already dying of the blow: whoever killed it */
    nge_entity_destroy(m);
    noise(m, EV_DIED, h, 0, 0);
    CHECK(ai_heard[h] == 0);                                /* the source */
    h2 = creature(CR_RAT, 8, 5);
    noise(m, EV_DIED, h, 0, 0);
    CHECK(record(h2, 6 - 3, 5, 5));
    nge_entity_release(nge_entity_next_dying());

    /* Silent and unknown events: no noise, through the filter or not */
    ai_heard[h] = 0;
    ai_heard[h2] = 0;
    filtered(h, EV_DAMAGED, actor_player);
    noise(h, EV_DAMAGED, actor_player, 3, 0);
    noise(h, EV_LOOKED, actor_player, 3, 0);
    filtered(h, 0, actor_player);
    filtered(h, NGE_EVENT_USER - 1, actor_player);
    filtered(h, NGE_EVENT_USER + NOISE_EVENTS, actor_player);
    noise(h, NGE_EVENT_USER + NOISE_EVENTS, actor_player, 0, 0);
    noise(h, 0, actor_player, 0, 0);
    CHECK(ai_heard[h] == 0 && ai_heard[h2] == 0);
    filtered(h, EV_HIT, actor_player);                      /* a noise: through */
    CHECK(record(h2, 6 - 4, 5, 8));                         /* (3, 3): range 4 */

    /* ---- the records: the distance, on nge_map_within()'s circle ---- */
    scene(1, 1);
    m = creature(CR_RAT, 5, 5);
    h = creature(CR_RAT, 11, 5);                            /* 6: heard 0 */
    far = creature(CR_RAT, 12, 5);                          /* 7: out of range */
    h2 = creature(CR_RAT, 9, 9);                            /* (4, 4): range 6 */
    other = creature_in(CR_RAT, AREA1, 6, 5);               /* another area, adjacent by tile */
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);
    CHECK(record(h, 0, 5, 5) && ai_heard[far] == 0 && record(h2, 0, 5, 5));
    CHECK(ai_heard[other] == 0 && ai_heard[m] == 0 && ai_heard[actor_player] == 0);
    nge_location_set(h2, 10, 10);                           /* (5, 5): beyond 6 */
    ai_heard[h2] = 0;
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);
    CHECK(ai_heard[h2] == 0);

    /* Louder replaces the record and its tile; quieter doesn't */
    ai_heard[h] = 0;
    nge_location_set(actor_player, 10, 5);
    noise(actor_player, EV_MOVED, NGE_EVENT_NONE, 1, 0);    /* a step next to it: 4 - 1 */
    CHECK(record(h, 3, 10, 5));
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);     /* 6 - 6: quieter */
    CHECK(record(h, 3, 10, 5));
    nge_location_set(m, 9, 5);
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);     /* 6 - 2: louder */
    CHECK(record(h, 4, 9, 5));

    /* ai_add() clears it */
    ai_add(h, &idle);
    CHECK(ai_heard[h] == 0);

    /* ---- muffled: a wall or a closed door between takes AI_MUFFLE off ---- */
    scene(1, 1);
    for (y = 0; y < SIZE; y++) {
        NGE_MAP_TILE(7, y).tile_id = WALL;
    }
    m = creature(CR_RAT, 5, 5);
    h = creature(CR_RAT, 9, 5);                             /* 4 away: 6 - 4 - 2 */
    far = creature(CR_RAT, 10, 5);                          /* 5 away: 6 - 5 < AI_MUFFLE */
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);
    CHECK(record(h, 6 - 4 - AI_MUFFLE, 5, 5) && ai_heard[far] == 0);
    /* The same through a closed door in the wall; open, not muffled */
    NGE_MAP_TILE(7, 5).tile_id = FLOOR;
    door = feature(FT_DOOR, 7, 5);
    ai_heard[h] = 0;
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);
    CHECK(record(h, 6 - 4 - AI_MUFFLE, 5, 5) && ai_heard[far] == 0);
    feature_open(door);
    nge_event_process(drop);
    ai_heard[h] = 0;
    noise(m, EV_HIT, actor_player, 15, NGE_EVENT_NONE);
    CHECK(record(h, 6 - 4, 5, 5) && record(far, 6 - 5, 5, 5));

    /* ---- end to end: the player's attack, its events handled as the game does, wakes a sleeper ---- */
    scene(2, 5);
    m = creature(CR_GOBLIN, 3, 5);
    h = creature(CR_RAT, 6, 5);                             /* 3 from the goblin; Wis 10 */
    ai_state[h] = AI_STATE_ASLEEP;
    a = (act_t){ 0 };
    a.actor = actor_player;
    a.target = m;
    a.item = NGE_ENTITY_INVALID;
    act_do(ACT_MELEE, &a, ACT_DO);
    CHECK(a.used == 1);                                     /* a hit or a miss: as loud */
    nge_event_process(dispatch);
    CHECK(ai_heard[h] == 6 - 3 + 1 && ai_heard_x[h] == 3 && ai_heard_y[h] == 5);
    srand(seed_for_roll(AI_WAKE_DC - 3));
    ai_on_turn(h);
    CHECK(ai_get_state(h) == AI_STATE_IDLE && ai_heard[h] == 0);

    return test_summary("noise");
}
