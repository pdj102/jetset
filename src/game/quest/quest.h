#ifndef QUEST_H
#define QUEST_H

/**
 * @file quest.h
 * @brief The quest item (docs/winning.md): the one item of kind IT_QUEST,
 *        made once a game by actors_create() (game/actors.c), persistent,
 *        never destroyed. The player wins by climbing out of depth 1 with
 *        it (act_ascend(), game/actions/act.h).
 *
 * It is always on one depth, 1 to LEVEL_DEPTH_BOTTOM: carried by the
 * player, held by a creature, or owned by a level area (game/level/level.h:
 * placed when the player enters that depth, parked while they are away).
 *
 * Banking: quest_item is in common memory; the functions are banked
 * (NGE_BANKED, page 128): don't call them while game code has its own page
 * mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "game/dev.h"

/** The quest item; NGE_ENTITY_INVALID before the first game. Never freed during a game, so the ID stays valid. */
extern nge_entity_id_t quest_item;

/** @brief A new game's quest item: id, just made. The player hasn't found it yet. */
void quest_set(nge_entity_id_t id) NGE_BANKED;

/** @brief 1 if the quest item's owners lead to actor: stowed or equipped, or in something it carries. */
uint8_t quest_carried(nge_entity_id_t actor) NGE_BANKED;

/** @brief On the player's first pick-up of the quest item in a game, post EV_QUEST_FOUND. */
void quest_on_event(const nge_event_t *e) NGE_BANKED;

#if GAME_DEV_KIT
/**
 * @brief Assert where the quest item is (docs/winning.md R14): alive and
 *        persistent; its owners lead to an entity on the current level or
 *        to an area of depth 1 to LEVEL_DEPTH_BOTTOM; placed if owned by the
 *        current level's area, not placed if owned by another. After each
 *        change of level.
 */
void quest_check(void) NGE_BANKED;
#endif

#endif
