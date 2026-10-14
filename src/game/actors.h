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

#include "ecs/nge_entity.h"
#include "input/nge_input.h"

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
 * @brief Carry out a player command from a key event: the cursor keys
 *        (and CAPS+5-8) or h j k l move one tile, blocked by walls; the
 *        camera follows. Other keys are ignored.
 */
void actors_player_command(nge_key_event_t ev);

#endif
