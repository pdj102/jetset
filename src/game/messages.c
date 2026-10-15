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
    case EV_BLOCKED:
        if (e->source == actor_player) {
            screen_message("Something is in the way.");
        }
        break;
    case EV_STOOD_ON:
        if (e->source == actor_player) {
            screen_message(e->target == actor_potion ? "You see a potion here."
                                                     : "You see something here.");
        }
        break;
    case EV_GLOWED:
        if (e->target == actor_player) {
            screen_message("A warm glow surrounds you.");
        }
        break;
    case EV_GLOW_FADED:
        if (e->target == actor_player) {
            screen_message("The warm glow fades.");
        }
        break;
    default:
        break;
    }
}
