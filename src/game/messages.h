#ifndef MESSAGES_H
#define MESSAGES_H

/**
 * @file messages.h
 * @brief The message window's reactions to events: what the player is told.
 */

#include "event/nge_event.h"

/** @brief Add a message for the events the player should hear about. */
void messages_on_event(const nge_event_t *e);

#endif
