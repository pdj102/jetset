#ifndef ACTORS_H
#define ACTORS_H

/**
 * @file actors.h
 * @brief The player, and a demo effect on the player.
 *
 * The player is an ECS entity with a sprite drawn by the render system,
 * placed in each level by level_generate() (owned by the level's area, with
 * a location on the map). The sprites are 8x8 figures in the top-left of
 * 16x16 patterns, so each covers exactly one map tile.
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/* Render layers: higher layers are drawn on top */
#define LAYER_OBJECTS   1
#define LAYER_MONSTERS  2
#define LAYER_PLAYER    3

extern nge_entity_id_t actor_player;

/** @brief Load the sprite patterns and palette: once, after nge_sprite_init(). */
void actors_load_graphics(void);

/**
 * @brief Create the player (not yet placed: see level_generate()) and the
 *        demo effect, for a new game (GAME_NEW_GAME, game/game.h). With
 *        GAME_DEV_KIT (game/dev.h, debug builds), the player starts with a
 *        sword and three rings in their pack and some resistances, for
 *        trying things out.
 *
 * Call after nge_entity_init(), nge_render_init() and nge_timer_init().
 */
void actors_create(void);

/**
 * @brief The demo effect's timer handler (TIMER_GLOW, game/timers.h): posts
 *        EV_GLOWED for its owner and heals it 2 hp, or on its last cycle
 *        posts EV_GLOW_FADED and destroys the effect.
 */
void actors_on_glow(nge_entity_id_t id, uint8_t cycles);

#endif
