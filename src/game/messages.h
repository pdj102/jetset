#ifndef MESSAGES_H
#define MESSAGES_H

/**
 * @file messages.h
 * @brief The message window's reactions to events: what the player is told.
 */

#include "ecs/nge_entity.h"
#include "event/nge_event.h"

/** @brief Add a message for the events the player should hear about. */
void messages_on_event(const nge_event_t *e);

/**
 * @brief Show before + the name of id's kind (gen_name()) + after, cut to
 *        one message line: "There is a " rat " in the way.". before and
 *        after may be banked literals of the caller's page.
 */
void messages_say_named(const char *before, nge_entity_id_t id, const char *after);

#endif
