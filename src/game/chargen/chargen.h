#ifndef CHARGEN_H
#define CHARGEN_H

/**
 * @file chargen.h
 * @brief Character creation (docs/character_creation.md): the classes, and
 *        making the player of one.
 *
 * A class is a creature kind with CREATURE_PLAYER (abilities, HP, mana,
 * spells: game/gen/gen_creature_pdata.c) and a record here naming the
 * class, its default name and its kit. A kit item is equipped only if its
 * record says so (a cursed ring in the Dev's kit stays in the pack).
 *
 *     Fighter  CR_FIGHTER  short sword, leather armour, iron shield
 *     Mage     CR_MAGE     dagger, sling, stones
 *     Dev      CR_PLAYER   sword, plate armour +2, short bow, arrows, and
 *                          the rest of the development kit (debug builds
 *                          only: GAME_DEV_KIT, game/dev.h)
 *
 * Banking: banked (NGE_BANKED, page 140, with the class table and the
 * creation screen). Don't call these while game code has its own page
 * mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "game/dev.h"

/** The classes offered: PC_CLASS_FIGHTER up to this, the Dev in debug builds only. */
#if GAME_DEV_KIT
#define CHARGEN_CLASSES  3
#else
#define CHARGEN_CLASSES  2
#endif

/** Longest class name ("Fighter"), without its NUL. */
#define CHARGEN_NAME_MAX  7

/**
 * @brief Empty the world (every entity, sprite, timer and event gone: the
 *        nge inits a new game needs) and make the player of class cls as
 *        entity 0, actor_player, with its kit equipped and carried. The
 *        events the making posted are thrown away.
 * @param cls A class offered: below CHARGEN_CLASSES.
 * @param name The player's name, 1 to PC_NAME_MAX characters, in common
 *        memory; or empty for the class's default name.
 *
 * Call after nge_ecs_init(); the game's own modules (player, level) are
 * set up by the caller afterwards.
 */
void chargen_make(uint8_t cls, const char *name) NGE_BANKED;

/**
 * @brief The creation screen, an overlay (game/ui/screen.h): the classes
 *        offered, and the chosen one's character (the real player, made
 *        again by chargen_make() on each move); up and down (j k) choose,
 *        N changes the name, ENTER starts. Then the welcome messages.
 * @param cls The class to open on, below CHARGEN_CLASSES: the last chosen.
 * @param name In common memory, PC_NAME_MAX + 1 bytes: empty for the
 *        class's default name; takes the name typed, kept for every class
 *        and the next game.
 * @return The class chosen. The player is made: entity 0, actor_player.
 */
uint8_t chargen_run(uint8_t cls, char *name) NGE_BANKED;

/**
 * @brief Copy class cls's name ("Fighter") into buf.
 * @param buf In common memory, CHARGEN_NAME_MAX + 1 bytes.
 */
void chargen_class_name(uint8_t cls, char *buf) NGE_BANKED;

#endif
