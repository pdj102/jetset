#ifndef ACTORS_H
#define ACTORS_H

/**
 * @file actors.h
 * @brief The level's entities: the player and a test object.
 *
 * Each is an ECS entity owned by the level's area, with a location on the
 * map and a sprite drawn by the render system. The sprites are 8x8 figures
 * in the top-left of 16x16 patterns, so each covers exactly one map tile.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/* Render layers: higher layers are drawn on top */
#define LAYER_OBJECTS  1
#define LAYER_PLAYER   2

extern nge_entity_id_t actor_player;
extern nge_entity_id_t actor_potion;

/**
 * @brief Load the sprite graphics and create the player and the test object.
 *
 * Call after nge_sprite_init(), level_init(), nge_ecs_init() and nge_render_init().
 */
void actors_init(void);

/**
 * @brief Move the player one tile by (dx, dy), if the tile is open; the
 *        camera follows. Posts EV_MOVED and an EV_STOOD_ON for each entity
 *        on the new tile, or EV_BUMPED for a wall (see game/events.h).
 * @return 1 if the player moved (the move takes a turn), 0 if not.
 */
uint8_t actors_move_player(int8_t dx, int8_t dy);

#endif
