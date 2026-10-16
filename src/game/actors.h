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
 *        demo effect, for a new game (GAME_NEW_GAME, game/game.h).
 *
 * Call after nge_entity_init(), nge_render_init() and nge_timer_init().
 */
void actors_create(void);

/**
 * @brief Move the player one tile by (dx, dy), if it can enter the tile
 *        (level_can_enter()); the camera follows. Posts EV_MOVED and an
 *        EV_STOOD_ON for each entity on the new tile; or EV_BLOCKED (target:
 *        the blocking entity) or EV_BUMPED (a wall) if it can't (see
 *        game/events.h).
 * @return 1 if the player moved (the move takes a turn), 0 if not.
 */
uint8_t actors_move_player(int8_t dx, int8_t dy);

/**
 * @brief The demo effect's timer handler (TIMER_GLOW, game/timers.h): posts
 *        EV_GLOWED for its owner, or on its last cycle EV_GLOW_FADED and
 *        destroys the effect.
 * @return NGE_TIMER_REPEAT, or NGE_TIMER_STOP on the last cycle.
 */
uint8_t actors_on_glow(nge_entity_id_t id, uint8_t cycles);

#endif
