#include "game/messages.h"

#include "game/actors.h"
#include "game/events.h"
#include "game/screen.h"

void messages_on_event(const nge_event_t *e)
{
    switch (e->id) {
    case EV_BUMPED:
        if (e->target == actor_player) {
            screen_message("You bump into a wall.");
        }
        break;
    case EV_STOOD_ON:
        if (e->source == actor_player) {
            screen_message(e->target == actor_potion ? "You see a potion here."
                                                     : "You see something here.");
        }
        break;
    default:
        break;
    }
}
