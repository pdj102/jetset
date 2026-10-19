#ifndef DEV_H
#define DEV_H

/**
 * @file dev.h
 * @brief Development aids, on in debug builds (CSpect), off with NDEBUG
 *        (hardware). -DGAME_DEV_KIT=0 or 1 overrides it.
 *
 * With GAME_DEV_KIT: a new game starts with a sword, three rings of health,
 * a healing potion, an antidote, a dead rat, an elixir and a potion of
 * strength in the player's pack, and the player resistant to fire, immune
 * to acid and vulnerable to cold (game/actors.c); X hurts the player,
 * cycling through the damage types (game/player); Shift+L on the title
 * runs the Layer 2 UI prototype (game/proto).
 */

#ifndef GAME_DEV_KIT
#ifdef NDEBUG
#define GAME_DEV_KIT  0
#else
#define GAME_DEV_KIT  1
#endif
#endif

#endif
