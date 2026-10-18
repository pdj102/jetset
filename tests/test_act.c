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
#include "game/components/attack.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/feature.h"
#include "game/components/item.h"
#include "game/components/stats.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/level/level.h"

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

static int stowed(nge_entity_id_t id, nge_entity_id_t holder)
{
    return NGE_OWNER_OF(id) == holder && !NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION);
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
    nge_entity_id_t opener;
    unsigned int s;
    int roll;
    int hp;
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
    nge_map_set_block(LEVEL_TILE_DOOR_SHUT, NGE_BLOCK_NONE);   /* as the level sets them: the door blocks */
    nge_map_set_block(LEVEL_TILE_DOOR_OPEN, NGE_BLOCK_NONE);
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
    events();   /* the player's innate effect started */

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
    i1 = gen_item(IT_POTION, AREA);
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
    a.kind = SLOT_NONE;
    CHECK(act_equip(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.kind == SLOT_WEAPON);
    CHECK(item_slot[i2] == SLOT_NONE && events() == 0);
    a.kind = SLOT_NONE;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1 && a.kind == SLOT_WEAPON);
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
    i1 = gen_item(IT_POTION, AREA);
    inv_stow(i1, player);
    a = ctx(player);
    a.item = i1;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_CANT_EQUIP);
    a = ctx(player);
    a.item = sword;                                           /* on the floor */
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED);
    /* Rings: the left hand, then the right, then both are taken */
    ring1 = gen_item(IT_RING, AREA);
    ring2 = gen_item(IT_RING, AREA);
    ring3 = gen_item(IT_RING, AREA);
    inv_stow(ring1, player);
    inv_stow(ring2, player);
    inv_stow(ring3, player);
    a = ctx(player);
    a.item = ring1;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.kind == SLOT_RING_L && item_slot[ring1] == SLOT_RING_L);
    a = ctx(player);
    a.item = ring2;
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.kind == SLOT_RING_R && item_slot[ring2] == SLOT_RING_R);
    /* Rings of health: each one worn counts, recomputed by the action; hit points not raised */
    CHECK(health_hp_max[player] == 12 + 4 && health_hp[player] == 12 && effects_check(player));
    a = ctx(player);
    a.item = ring3;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == ring2);
    /* A slot asked for: taken, not one it fits, or free */
    a = ctx(player);
    a.item = ring3;
    a.kind = SLOT_RING_L;
    CHECK(act_equip(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_SLOT_TAKEN && a.target == ring1);
    a = ctx(player);
    a.item = ring3;
    a.kind = SLOT_HEAD;
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
    a.kind = SLOT_RING_L;
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
    CHECK(act_equip(&a, ACT_DO) == ACT_SUCCESS && a.kind == SLOT_WEAPON);
    events();

    /* ---- melee: the player (sword i2 wielded) at (4, 5), a goblin west of it ---- */
    CHECK(at(player, 4, 5) && item_slot[i2] == SLOT_WEAPON);
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
    CHECK(a.target == gob && a.item == i2 && a.kind == DMG_SLASH && a.amount >= 1 && a.amount <= 8);
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
    /* Its own attack without a weapon: fists, blunt, which the goblin resists */
    item_slot[i2] = SLOT_NONE;
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = gob;
    CHECK(act_melee(&a, ACT_DO) == ACT_SUCCESS && a.item == NGE_ENTITY_INVALID && a.kind == DMG_BLUNT);
    CHECK(a.amount == 1 && health_hp[gob] == 199);            /* 1d2 resisted: rounded up to 1 */
    CHECK(events() == 2 && seen[0].p2 == NGE_EVENT_NONE && EV_DMG_OUTCOME(seen[1].p2) == DMG_RESISTED);
    /* A kill: EV_DIED, the goblin destroyed */
    health_hp[gob] = 1;
    srand(seed_for_roll(20));
    a = ctx(player);
    a.target = gob;
    CHECK(act_do(ACT_MELEE, &a, ACT_DO) == ACT_SUCCESS && health_hp[gob] == 0);
    CHECK(events() == 3 && seen[2].id == EV_DIED && NGE_ENTITY_STATE(gob) == NGE_ENTITY_DYING);
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
    far = gen_item(IT_POTION, AREA);
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

    /* ---- use: a stowed item with a use effect, used up ---- */
    effect_cure(player, STATUS_POISONED);
    health_hp[player] = 3;
    thing = gen_item(IT_POTION, AREA);
    inv_stow(thing, player);
    events();
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_CHECK) == ACT_SUCCESS && a.used == 0 && a.kind == EF_HEALING);
    CHECK(NGE_ENTITY_IS_ALIVE(thing) && health_hp[player] == 3 && events() == 0);
    CHECK(act_use(&a, ACT_DO) == ACT_SUCCESS && a.reason == ACT_R_NONE && a.used == 1);
    CHECK(NGE_ENTITY_STATE(thing) == NGE_ENTITY_DYING && health_hp[player] == 3 + 6);
    CHECK(events() == 3 && is(0, thing, EV_USED, player) && is(1, player, EV_HEALED, NGE_EVENT_NONE) &&
          seen[1].p1 == 6 && is(2, player, EV_EFFECT_STARTED, NGE_EVENT_NONE) && seen[2].p1 == EF_HEALING);
    /* Used up already: not carried */
    a = ctx(player);
    a.item = thing;
    CHECK(act_use(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_NOT_CARRIED && a.used == 0);
    /* Food, by number: poisoned */
    thing = gen_item(IT_FOOD, AREA);
    inv_stow(thing, player);
    a = ctx(player);
    a.item = thing;
    CHECK(act_do(ACT_USE, &a, ACT_DO) == ACT_SUCCESS && a.kind == EF_POISON && STATUS_HAS(player, STATUS_POISONED));
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
    CHECK(FEATURE_IS_OPEN(door) && NGE_MAP_TILE(8, 1).tile_id == LEVEL_TILE_DOOR_OPEN);
    CHECK(events() == 1 && is(0, door, EV_OPENED, opener));
    CHECK(act_open(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ALREADY_OPEN && a.used == 0);
    /* Open, it can be walked through */
    a = move(opener, 1, 0);
    CHECK(act_move(&a, ACT_DO) == ACT_SUCCESS && at(opener, 8, 1));
    events();                                                  /* EV_MOVED only: no EV_STOOD_ON message for a door */
    /* Back out; closing it with an item in the doorway: something is in the way */
    a = move(opener, -1, 0);
    act_move(&a, ACT_DO);
    thing = gen_item(IT_POTION, AREA);
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
    CHECK(NGE_MAP_TILE(8, 1).tile_id == LEVEL_TILE_DOOR_SHUT && events() == 1 && is(0, door, EV_CLOSED, opener));
    a = move(opener, 1, 0);
    CHECK(act_move(&a, ACT_CHECK) == ACT_FAIL && a.target == door);
    a = ctx(opener);
    a.target = door;
    CHECK(act_close(&a, ACT_DO) == ACT_FAIL && a.reason == ACT_R_ALREADY_CLOSED);
    CHECK(act_do(ACT_OPEN, &a, ACT_DO) == ACT_SUCCESS && FEATURE_IS_OPEN(door));   /* by number */
    events();
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

    /* ---- every reason is refused, but a miss ---- */
    ok = 1;
    for (r1 = 0; r1 < ACT_R_COUNT; r1++) {
        ok = ok && act_attempted((uint8_t)r1) == (r1 == ACT_R_MISSED);   /* includes ACT_R_CANT_USE */
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

    return test_summary("act");
}
