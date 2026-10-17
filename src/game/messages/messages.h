#ifndef MESSAGES_H
#define MESSAGES_H

/**
 * @file messages.h
 * @brief The message window's reactions to events and to the player's
 *        refused commands: what the player is told.
 *
 * Banking: banked (NGE_BANKED, page 118): don't call these while game code
 * has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "event/nge_event.h"
#include "game/actions/act.h"

/** @brief Add a message for the events the player should hear about. */
void messages_on_event(const nge_event_t *e) NGE_BANKED;

/** @brief Tell the player why their action failed (a->reason, a on the caller's stack), if they need telling. */
void messages_act_failed(const act_t *a) NGE_BANKED;

#endif
