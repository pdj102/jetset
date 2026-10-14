#ifndef LEVEL_H
#define LEVEL_H

/**
 * @file level.h
 * @brief The current level: one area with its map, shown in the map view.
 *
 * The map is resident in slot 1 (page LEVEL_MAP_PAGE) while the level is
 * active, so collision checks and drawing read it directly. Map cells are
 * single tiles, drawn with the font's ASCII tiles in the classic roguelike
 * style: '|' and '-' walls, '.' floor, '+' doors, '#' corridors.
 */

#include <stdint.h>

#include "ecs/nge_owner.h"
#include "map/nge_map.h"

#define LEVEL_AREA      NGE_OWNER_AREA(0)   /**< Owner of the level's entities. */
#define LEVEL_MAP_SLOT  1                   /**< $2000: the ROM's upper half swapped out. */
#define LEVEL_MAP_PAGE  100                 /**< Game range (see src/nge/banks.inc). */
#define LEVEL_WIDTH     60                  /**< Map size in tiles: larger than the view. */
#define LEVEL_HEIGHT    40

/** The level's map, shown in the map view. */
extern nge_map_t level_map;

/**
 * @brief Map the level's page into slot 1, build the test layout and set up
 *        the map in the map view, with sprites clipped to it.
 *
 * Call after screen_init() and nge_sprite_init().
 */
void level_init(void);

/** @brief 1 if the tile at (x, y) can be walked on: floor, door or corridor. */
uint8_t level_is_open(uint8_t x, uint8_t y);

#endif
