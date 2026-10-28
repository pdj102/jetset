#ifndef ANIM_H
#define ANIM_H

/**
 * @file anim.h
 * @brief Action animations: what an entity shows when it acts
 *        (docs/animations.md).
 *
 * Actions stay instant: the location, the damage and the events happen at
 * once, and the animations start from the events afterwards, to show what
 * already happened. The render system (ecs/systems/nge_render.h) plays
 * them: a slide from the old tile after a move, a lunge towards the target
 * after a melee attack, and the kind's sequence for the action, if it has
 * one (gen_creature_seqs()). A sequence ends in the entity's rest animation
 * by itself: nothing here restores it.
 *
 * While one plays in view, anim_holding() is true: the turn engine doesn't
 * advance the world and the player's command waits (game/game.c).
 *
 * Banking: anim_init() and anim_on_event() are banked (NGE_BANKED, page
 * 126, with the kinds' sequences in page 127): don't call them while game
 * code has its own page mapped in slot 6 or 7. anim_holding() is in common
 * memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "event/nge_event.h"

#define ANIM_SLIDE_SPEED   4   /**< Pixels a frame of a slide: 4 frames a tile, the key repeat rate. */
#define ANIM_LUNGE_PIXELS  8   /**< How far a lunge goes towards the target: half a tile. */
#define ANIM_LUNGE_SPEED   4   /**< Pixels a frame of a lunge: 4 frames out and back. */

/** @brief Copy the creature kinds' sequences from their kind records: once, at start-up. */
void anim_init(void) NGE_BANKED;

/** @brief Start the animations an event shows: EV_MOVED, EV_HIT, EV_MISSED. */
void anim_on_event(const nge_event_t *e) NGE_BANKED;

/** @brief An animation the world waits for is playing. Ask it only when deciding whether something may act. */
uint8_t anim_holding(void);

#endif
