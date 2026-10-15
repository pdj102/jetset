#include "game/messages.h"

#include "game/actors.h"
#include "game/events.h"
#include "game/items.h"
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
            if (!NGE_ENTITY_HAS_COMPONENTS(e->target, COMPONENT_ITEM)) {
                screen_message("You see something here.");
            } else if (item_kind[e->target] == ITEM_SWORD) {
                screen_message("You see a sword here.");
            } else {
                screen_message("You see a potion here.");
            }
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
