#ifndef LEVEL_H
#define LEVEL_H

/**
 * @file level.h
 * @brief The current level: one area with its map, shown in the map view.
 *
 * Its map is the map (map/nge_map.h: nge_map, nge_map_tiles), in slot 1, so
 * collision checks and drawing read it directly. Map cells are
 * single tiles, drawn with the font's ASCII tiles in the classic roguelike
 * style: '|', '-' and '+' walls, '.' and ',' floor, ' doors, '#' corridors.
 * What each tile ID blocks is in the map's block table.
 *
 * level_generate() builds a level for a dungeon depth: rooms joined by
 * corridors, every room reachable, items in the rooms and the player in one.
 *
 * Banking: level_init() and level_generate() are banked (NGE_BANKED, page
 * 102, with their data in page 103): don't call them while game code has
 * its own page mapped in slot 6 or 7. level_can_enter() is in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_block.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"

#define LEVEL_AREA      NGE_OWNER_AREA(0)   /**< Owner of the level's entities. */
#define LEVEL_WIDTH     60                  /**< Map size in tiles: larger than the view. */
#define LEVEL_HEIGHT    40

/**
 * @brief Set up the map in the map view (nge_map_init(), which maps its
 *        page into slot 1), what each tile blocks, and clip sprites to it.
 *
 * Call once, after screen_init() and nge_sprite_init().
 */
void level_init(void) NGE_BANKED;

/**
 * @brief Generate the level for a dungeon depth, and put the player in it.
 *
 * Destroys the entities the level's area owns except the player (the
 * game releases them at the end of the frame), then builds the map: up to
 * 10 rooms (LEVEL_ROOMS_MIN-LEVEL_ROOMS_MAX) in a grid, joined by corridors
 * so every room can be reached; places 4 to 10 placeholder items
 * (game/items.h) on free floor in the rooms; and moves the player onto free
 * floor in a room, owned by LEVEL_AREA, with the camera on them.
 *
 * Seeds rand() with depth, so a depth's level is always the same (for
 * development).
 *
 * Call after level_init(), nge_ecs_init() and nge_render_init().
 * @param player The player: an entity in use, kept from level to level.
 */
void level_generate(uint8_t depth, nge_entity_id_t player) NGE_BANKED;

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

#endif
