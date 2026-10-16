#ifndef MESSAGES_H
#define MESSAGES_H

/**
 * @file messages.h
 * @brief The message window's reactions to events, and to the player's
 *        refused actions: what the player is told.
 */

#include "event/nge_event.h"
#include "game/actions/act.h"

/** @brief Add a message for the events the player should hear about. */
void messages_on_event(const nge_event_t *e);

/** @brief Tell the player why their action failed (a->reason), if they need telling. */
void messages_act_failed(const act_t *a);

#endif
