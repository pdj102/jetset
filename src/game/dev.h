#ifndef DEV_H
#define DEV_H

/**
 * @file dev.h
 * @brief Development aids, on in debug builds (CSpect), off with NDEBUG
 *        (hardware). -DGAME_DEV_KIT=0 or 1 overrides it.
 *
 * With GAME_DEV_KIT: a new game starts with a sword, three rings of health,
 * a healing potion, an antidote, a dead rat, an elixir and a potion of
 * strength in the player's pack, with a short bow, arrows, darts and a
 * flask of oil to fire, a wand and a parchment to aim, a lock pick (no
 * keys: one in the pack would open a locked door when bumped), a ring of
 * frailty for curses, and the player resistant to fire, immune
 * to acid and vulnerable to cold (game/actors.c); X hurts the player,
 * cycling through the damage types (game/player); L locks the nearest
 * closed door, with an iron lock (game/player); V pans the view a pixel
 * at a time while a movement key is held, to check the pixel camera; M
 * spends 1 of the player's MP, or restores them all at 0, to check the
 * stats window follows (game/components/mana.h); E puts the player up a
 * level and gives 10 gold (game/components/pc.h); rats and goblins act
 * half as often, speed 20 (game/gen/gen_creature_pdata.c), and look (x)
 * names a monster's AI state, "You see a goblin (hunting)."
 * (game/messages), to watch the AI (game/ai/ai.h); bandits start awake,
 * and a new game has one out of sight with a short bow and arrows at its
 * feet (level_dev_bandit()), to watch a monster arm itself and shoot,
 * and a goblin shaman out of sight, asleep, to watch a monster cast.
 */

#ifndef GAME_DEV_KIT
#ifdef NDEBUG
#define GAME_DEV_KIT  0
#else
#define GAME_DEV_KIT  1
#endif
#endif

#endif
