#ifndef LEVEL_H
#define LEVEL_H

/**
 * @file level.h
 * @brief The current level: one depth of the dungeon, its area and map,
 *        shown in the map view; going from one level to another.
 *
 * Depths run from 1 (the top) to LEVEL_DEPTH_BOTTOM; 0 is the surface,
 * leaving the dungeon (the game's job, not a level). Each depth's entities
 * are owned by its own area, NGE_OWNER_AREA(depth); only the current
 * level, level_depth, has a map, and LEVEL_AREA is its area. Levels aren't
 * kept: each visit generates a new one (docs/level_transitions.md).
 *
 * Its map is the map (map/nge_map.h: nge_map, nge_map_tiles), in slot 1, so
 * collision checks and drawing read it directly. Map cells are
 * single tiles, drawn with the font's ASCII tiles in the classic roguelike
 * style: '|', '-' and '+' walls, '.' and ',' floor, ' doors, '#' corridors,
 * '<' and '>' stairs. What each tile ID blocks is in the map's block table.
 *
 * Kept entities: persistent ones (NGE_ENTITY_PERSISTENT: the player, a
 * quest object, a boss), and what persistent ones own, directly or through
 * others (the player's pack). Leaving a level destroys the rest of its
 * entities; kept ones stay owned by its area with no location, parked,
 * and are placed again when that depth is next generated. Parked entities
 * are frozen: their limited timed effects end when they are parked, and
 * the game's timer handlers skip entities not on the current level
 * (level_holds()).
 *
 * A change of level is asked for (level_request()) and carried out by the
 * game at the end of the frame, after that frame's events are handled,
 * with level_generate(): never inside the action, timer or event handler
 * that asks for it.
 *
 * Banking: level_init(), level_generate(), level_put_floor(),
 * level_send() and level_park() are banked (NGE_BANKED, page 102, with
 * their data in page 103): don't call them while game code has its own
 * page mapped in slot 6 or 7. level_can_enter(), level_holds(), the
 * requests and level_release_dying() are in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_block.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"

#define LEVEL_DEPTH_BOTTOM  10   /**< The deepest level: no down stairs. */
#define LEVEL_AREA      NGE_OWNER_AREA(level_depth)   /**< Owner of the current level's entities. */
#define LEVEL_WIDTH     60                  /**< Map size in tiles: larger than the view. */
#define LEVEL_HEIGHT    40

/* How the player arrives on a level (level_generate(), level_request()) */
#define LEVEL_ARRIVE_ANY          0   /**< On random free floor: a fall, a teleport. */
#define LEVEL_ARRIVE_UP_STAIRS    1   /**< On its up stairs: came down. */
#define LEVEL_ARRIVE_DOWN_STAIRS  2   /**< On its down stairs: came up. */

/** The current level's depth: 1 to LEVEL_DEPTH_BOTTOM once a game has started, 0 before. Read-only. */
extern uint8_t level_depth;

/**
 * With GAME_DEV_KIT: level_generate() seeds rand() with level_dev_seed +
 * depth. 0 at start; a development setting, not game state (a new game
 * keeps it): change it to see other dungeons, each the same every run.
 */
extern uint16_t level_dev_seed;

/* Map tiles features are drawn with (game/components/feature.h): they block nothing, the feature does */
#define LEVEL_TILE_DOOR_SHUT  '+'    /**< A closed door. */
#define LEVEL_TILE_DOOR_OPEN  '\''  /**< An open door. */
#define LEVEL_TILE_STAIRS_UP   '<'  /**< Stairs up. */
#define LEVEL_TILE_STAIRS_DOWN '>'  /**< Stairs down. */

/**
 * @brief Set up the map in the map view (nge_map_init(), which maps its
 *        page into slot 1), what each tile blocks, and clip sprites to it.
 *
 * Call once, after screen_init() and nge_sprite_init().
 */
void level_init(void) NGE_BANKED;

/**
 * @brief Leave the current level and generate the level for a dungeon
 *        depth, with the player in it.
 *
 * Leaving: takes the player (with all they own) out of the current level's
 * area, destroys and releases every entity it owns that isn't kept,
 * including what they own (nothing is dropped), and parks the kept ones
 * left in it (level_park()). Then builds the map: up to
 * 10 rooms (LEVEL_ROOMS_MIN-LEVEL_ROOMS_MAX) in a grid, joined by corridors
 * so every room can be reached, with a door entity (open or closed) in
 * about a third of the doorways (game/components/feature.h) and plain
 * floor in the rest; up stairs in one room and, above the bottom, down
 * stairs in another; 4 to 10 random items and 2 to 5 random monsters for
 * the depth (game/gen/gen.h) on free floor in the rooms; then the
 * entities parked in the depth's area on free floor; and the player, owned
 * by the new LEVEL_AREA with the camera on them, on the stairs arrival
 * names or on free floor.
 *
 * With GAME_DEV_KIT (debug builds) it seeds rand() with level_dev_seed +
 * depth before building: a depth's map and random entities are always the same, as
 * what depends on play (the parked entities, the arrival) comes after them.
 *
 * Call after level_init(), nge_ecs_init(), nge_render_init() and
 * nge_timer_init(), with no events waiting that name the current level's
 * entities (they are released).
 * @param depth 1 to LEVEL_DEPTH_BOTTOM: the new level_depth.
 * @param player The player: an entity in use, kept from level to level.
 * @param arrival LEVEL_ARRIVE_*.
 */
void level_generate(uint8_t depth, nge_entity_id_t player, uint8_t arrival) NGE_BANKED;

/**
 * @brief Put the level's plain floor at (x, y): where a destroyed feature
 *        was (game.c's clean-up).
 */
void level_put_floor(uint8_t x, uint8_t y) NGE_BANKED;

/**
 * @brief Can entity id enter tile (x, y) of the level: neither the map nor
 *        another entity there blocks it (nge_block_can_enter())?
 *
 * Uses the entity's pass flags (NGE_BLOCK_NONE without the block component)
 * and ignores the entity itself.
 * @param blocker If not NULL, receives the blocking entity, or
 *                NGE_ENTITY_INVALID if the map blocks or nothing does.
 * @return 1 if it can enter, 0 if not.
 */
uint8_t level_can_enter(nge_entity_id_t id, uint8_t x, uint8_t y, nge_entity_id_t *blocker);

/** @brief 1 if entity id (in use) is on the current level: its owners lead to LEVEL_AREA. */
uint8_t level_holds(nge_entity_id_t id);

/**
 * @brief Ask for the player to go to depth, arriving as arrival
 *        (LEVEL_ARRIVE_*): the game does it at the end of the frame.
 *
 * The first request wins until the game takes it (level_take_request()).
 * depth is kept to 1-LEVEL_DEPTH_BOTTOM, except that 0 (leaving the
 * dungeon) can be reached only by going up stairs (LEVEL_ARRIVE_DOWN_STAIRS).
 */
void level_request(uint8_t depth, uint8_t arrival);

/** 1 while a change of level is waiting (level_request()). */
#define level_pending()  (level_request_waiting)
extern uint8_t level_request_waiting;   /**< Read-only: use level_pending(). */

/**
 * @brief Take the waiting request: its depth (0: leaving the dungeon) and
 *        arrival. Call only while level_pending().
 */
void level_take_request(uint8_t *depth, uint8_t *arrival);

/** @brief Drop any waiting request (a new game). */
void level_clear_request(void);

/**
 * @brief Send entity id, not the player, from the current level to depth:
 *        it leaves the level (posts EV_LEFT_LEVEL, p1: the depth).
 *
 * A persistent entity is parked in the depth's area (level_park()). Any
 * other is destroyed with what it owns, except kept entities in it, which
 * drop where it was when it is released (level_release_dying()). depth is
 * kept to 1-LEVEL_DEPTH_BOTTOM; sending it to the current depth does
 * nothing.
 * @return 1 if it left, 0 if not (the same depth).
 */
uint8_t level_send(nge_entity_id_t id, uint8_t depth) NGE_BANKED;

/**
 * @brief Park a kept entity (in use) in area: owned by it, with no
 *        location, and frozen: the timed effects it owns that would run
 *        out end now (effect_end()); those that last for ever stay, and
 *        are skipped while it is off the current level.
 */
void level_park(nge_entity_id_t id, nge_owner_t area) NGE_BANKED;

/**
 * @brief Release every dying entity (the game's clean-up, at the end of
 *        each frame, after its events are handled).
 *
 * A placed feature leaves floor (level_put_floor()). What a dying entity
 * owns: items drop on the floor where it was (if it was on the map), other
 * entities (effects) are destroyed, and the rest (items it held off the
 * map, dying entities) pass to its owner. A persistent entity owned by
 * another must be an item (asserted): anything else couldn't be dropped.
 */
void level_release_dying(void);

#endif
