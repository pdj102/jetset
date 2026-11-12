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
 * The visual effects (docs/vfx.md) start from events too, and the engine's
 * VFX module (vfx/nge_vfx.h) plays them: a shot or a magic bolt flying to
 * its target, a glow on a creature an effect takes hold on, a splash and a
 * white flash on one hurt, and a stand-in for one killed, shown until the
 * blow lands. Impacts wait for the bolt to their tile.
 *
 * While an animation or a VFX plays in view, anim_holding() is true: the
 * turn engine doesn't advance the world and the player's command waits
 * (game/game.c).
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
#define ANIM_LUNGE_SPEED   2   /**< Pixels a frame of a lunge: 8 frames out and back, time for a 2-pattern attack. */

#define ANIM_SHOT_SPEED    8   /**< Pixels a frame of a shot: 2 frames a tile (docs/vfx.md D5). */
#define ANIM_MAGIC_SPEED   4   /**< Pixels a frame of a magic bolt: slower, it reads as heavier. */
#define ANIM_FX_RATE       3   /**< Frames each pattern of a VFX shows: 60 ms (docs/sprite_style.md 6.7). */
#define ANIM_FLASH_FRAMES  4   /**< Frames a creature hit shows white. */
/** Frames before an impact shows: from the update the lunge reaches its peak (docs/vfx.md D10). */
#define ANIM_IMPACT_DELAY  (ANIM_LUNGE_PIXELS / ANIM_LUNGE_SPEED - 1)

/** @brief Copy the creature kinds' sequences from their kind records: once, at start-up. */
void anim_init(void) NGE_BANKED;

/**
 * @brief Start the animations and VFX an event shows: EV_MOVED, EV_HIT,
 *        EV_MISSED; EV_FIRED, EV_CAST, EV_AIMED, EV_EFFECT_STARTED,
 *        EV_DAMAGED, EV_DIED.
 */
void anim_on_event(const nge_event_t *e) NGE_BANKED;

/**
 * @brief End every VFX now (nge_vfx_clear()), and forget an area's centre:
 *        on a change of level and a new game (docs/vfx.md R14).
 */
void anim_vfx_clear(void) NGE_BANKED;

/** @brief An animation or VFX the world waits for is playing. Ask it only when deciding whether something may act. */
uint8_t anim_holding(void);

#endif
