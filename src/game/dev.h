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
 * cycling through the damage types (game/player); V pans the view a pixel
 * at a time while a movement key is held, to check the pixel camera; M
 * spends 1 of the player's MP, or restores them all at 0, to check the
 * stats window follows (game/components/mana.h).
 */

#ifndef GAME_DEV_KIT
#ifdef NDEBUG
#define GAME_DEV_KIT  0
#else
#define GAME_DEV_KIT  1
#endif
#endif

#endif
