#ifndef AI_H
#define AI_H

/**
 * @file ai.h
 * @brief The AI: the game decides the turns of an entity with the AI
 *        component (every monster), through the same actions as the
 *        player's commands (game/actions/act.h). See docs/ai.md.
 *
 * A state machine, with three concerns kept apart:
 * - state (AI_STATE_*): what the entity is doing now, one byte each. Each
 *   state has one handler, shared by every brain, which runs on the
 *   entity's turn and may do one action and return a stimulus;
 * - brain (AI_BRAIN_*): how it chooses, named by its kind's record (an
 *   ai_kind_t part): a transition table, state and stimulus (AI_STIM_*)
 *   to the next state, constant data. A new brain is a row of data;
 * - capabilities (AI_CAN_*): what it can do this turn, worked out by
 *   ai_can() from its components and abilities when asked, never stored,
 *   so they follow equipment and effects.
 *
 * Turns: its timer (TIMER_AI, game/timers.h) fires every `speed` ticks,
 * and the game calls ai_on_turn() for an AI entity on the current level.
 * It does at most one action that uses the turn; not acting is waiting.
 *
 * Events: ai_on_event() turns events about an alive AI entity into
 * stimuli (attacked: AI_STIM_HURT). Reacting changes only the state,
 * never acts: an entity acts on its own turn. A change of state posts
 * EV_AI_CHANGED (game/events.h).
 *
 * The component lives with the module, as the effects' does
 * (game/effects): its data, each entity's state and brain, is the
 * module's page data (page 131), read only by its code.
 *
 * Banking: banked (NGE_BANKED, page 130, with the data in page 131):
 * don't call these while game code has its own page mapped in slot 6 or
 * 7. ai_on_event() is in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "game/components/components.h"

typedef uint8_t ai_state_t;   /**< AI_STATE_*. */
typedef uint8_t ai_stim_t;    /**< AI_STIM_*. */
typedef uint8_t ai_brain_t;   /**< AI_BRAIN_*. */
typedef uint8_t ai_can_t;     /**< A mask of AI_CAN_*. */

/* States */
#define AI_STATE_NONE    0   /**< No state: never an entity's. */
#define AI_STATE_ASLEEP  1   /**< Does nothing; wakes when hurt. */
#define AI_STATE_IDLE    2   /**< Awake with nothing to do: waits (later: wanders, looks). */
#define AI_STATE_COUNT   3
#define AI_STATE_NO_CHANGE  AI_STATE_NONE   /**< In a brain's table: stay in the current state. */

/* Stimuli */
#define AI_STIM_NONE   0   /**< Nothing new: from a state's handler. Changes no state. */
#define AI_STIM_HURT   1   /**< Attacked: hit, missed or damaged. */
#define AI_STIM_COUNT  2

/* Brains */
#define AI_BRAIN_NONE     0   /**< In a kind record: no AI. */
#define AI_BRAIN_DEFAULT  1   /**< The shared one: wakes when hurt. */
#define AI_BRAIN_COUNT    2

/* Capabilities: bits of ai_can() */
#define AI_CAN_MELEE   0x01   /**< Attacks in melee: its own attack or a wielded weapon's. */
#define AI_CAN_PICKUP  0x02   /**< Picks up items: an inventory. */
#define AI_CAN_EQUIP   0x04   /**< Wields and wears them: an inventory with an equipment slot. */
#define AI_CAN_USE     0x08   /**< Uses them (drinks a potion): an inventory and Int AI_INT_USE. */
#define AI_CAN_OPEN    0x10   /**< Opens and closes doors: Int AI_INT_OPEN. */
#define AI_CAN_RANGED  0x20   /**< Reserved: a ranged attack (later). */
#define AI_CAN_CAST    0x40   /**< Reserved: casts spells, with mana (later). */

/* The least Intelligence for the AI's own rules (the player needs none) */
#define AI_INT_USE   5
#define AI_INT_OPEN  6

/** The AI part of a creature's kind record. */
typedef struct ai_kind {
    ai_brain_t brain;   /**< AI_BRAIN_*; AI_BRAIN_NONE for no AI. */
    ai_state_t state;   /**< AI_STATE_* it starts in. */
} ai_kind_t;

/** The kind record k names a brain: the creature has AI. */
#define AI_KIND_HAS(k)  ((k)->brain != AI_BRAIN_NONE)

/** Entity id has the AI component. */
#define AI_IS(id)  NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_AI)

/**
 * @brief Give an entity in use the AI component, with the brain and the
 *        start state of k: the game decides its turns.
 * @param k In common memory (a copy on the stack). Its brain isn't
 *          AI_BRAIN_NONE; its state isn't AI_STATE_NONE.
 */
void ai_add(nge_entity_id_t id, const ai_kind_t *k) NGE_BANKED;

/** @brief Remove the AI component. */
void ai_remove(nge_entity_id_t id) NGE_BANKED;

/** @brief An AI entity's state (AI_STATE_*). */
ai_state_t ai_get_state(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief What an alive entity can do now (AI_CAN_*), from its components
 *        and current abilities: what the AI would consider. The action's
 *        own check still decides (a door may be locked).
 */
ai_can_t ai_can(nge_entity_id_t id) NGE_BANKED;

/** @brief An AI entity's turn (TIMER_AI): its state's handler. */
void ai_on_turn(nge_entity_id_t id) NGE_BANKED;

/**
 * @brief React to events about alive AI entities: their stimuli, which may
 *        change their state (EV_AI_CHANGED). In common memory: events
 *        that don't concern the AI cost no banked call.
 */
void ai_on_event(const nge_event_t *e);

#endif
