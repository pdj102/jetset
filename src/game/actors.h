#ifndef ACTORS_H
#define ACTORS_H

/**
 * @file actors.h
 * @brief The player, and the game's other persistent entities.
 *
 * The player is an ECS entity with a sprite drawn by the render system,
 * placed in each level by level_generate() (owned by the level's area, with
 * a location on the map). The sprites are 16x16 patterns, so each covers
 * exactly one map tile: 4-bit patterns, each shown through a bank of the
 * sprite palette (BANK_*).
 */

#include <stdint.h>

#include "ecs/nge_entity.h"

/* Render layers: higher layers are drawn on top */
#define LAYER_OBJECTS   1
#define LAYER_MONSTERS  2
#define LAYER_PLAYER    3

/*
 * The sprite palette's banks (docs/sprite_import.md 5.2): every bank has the
 * same 16 slots, so a sprite shows in any bank, NGE_SPRITE_PALETTE(BANK_*),
 * in that bank's colours. Banks 3-15 are colourways.
 */
#define BANK_PLAYER   0    /* the player's own colours */
#define BANK_FLASH    1    /* every slot white: a hit flash */
#define BANK_SHADOW   2    /* every slot dark violet: a silhouette */
#define BANK_RED      3
#define BANK_ORANGE   4    /* fire */
#define BANK_GOLD     5
#define BANK_GREEN    6
#define BANK_LIME     7    /* poison */
#define BANK_BLUE     8
#define BANK_ICE      9
#define BANK_PURPLE   10
#define BANK_FLESH    11
#define BANK_EARTH    12
#define BANK_IRON     13
#define BANK_BONE     14
#define BANK_DARK     15
#define BANK_COUNT    16

extern nge_entity_id_t actor_player;

/** @brief Load the sprite patterns and palette: once, after nge_sprite_init(). */
void actors_load_graphics(void);

/**
 * @brief Create the player (not yet placed: see level_generate()) and the
 *        other persistent entities (a table in actors.c: each in its
 *        depth's area, not placed, NGE_ENTITY_PERSISTENT), for a new game
 *        (play_start(), game/game.c). The player is entity 0. The quest
 *        item waits on the bottom depth (game/quest/quest.h). With
 *        GAME_DEV_KIT, a persistent goblin waits on depth 2. With GAME_DEV_KIT
 *        (game/dev.h, debug builds), the player starts with a sword, three
 *        rings and one of each potion and food in their pack and some
 *        resistances, for trying things out.
 * @param name The player's name (game/components/pc.h): 1 to PC_NAME_MAX
 *        characters, in common memory.
 *
 * Call after nge_entity_init(), nge_render_init() and nge_timer_init().
 */
void actors_create(const char *name);

#endif
