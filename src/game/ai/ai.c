/*
 * The AI module's common part: the event filter, called for every event.
 * Only events about an alive AI entity reach the banked code (ai_bank.c).
 */
#include "game/ai/ai.h"

#include "game/ai/ai_internal.h"
#include "game/events.h"

void ai_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* locals: see T-ZSDCC-1 and messages_on_event() */
    nge_entity_id_t source = e->source;

    switch (e->id) {
    case EV_HIT:
    case EV_MISSED:
    case EV_DAMAGED:
        /* Alive: an attack that kills leaves it dying, and it doesn't wake */
        if (target != NGE_EVENT_NONE && NGE_ENTITY_IS_ALIVE(target) && AI_IS(target)) {
            ai_stimulus(target, AI_STIM_HURT, source);
        }
        break;
    default:
        break;
    }
}
