#include "game/messages.h"

#include <string.h>

#include "game/actors.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/screen.h"

#define MESSAGE_MAX  41   /* a message window line, and its NUL */
#define NAME_MAX     20   /* a kind's name, and its NUL */

void messages_say_named(const char *before, nge_entity_id_t id, const char *after)
{
    char line[MESSAGE_MAX];
    char name[NAME_MAX];

    gen_name(id, name, sizeof(name));
    line[0] = '\0';
    strncat(line, before, sizeof(line) - 1);
    strncat(line, name, sizeof(line) - 1 - strlen(line));
    strncat(line, after, sizeof(line) - 1 - strlen(line));
    screen_message(line);
}

void messages_on_event(const nge_event_t *e)
{
    /*
     * The fields in locals: SDCC (z88dk, Oct 2026) reused HL for
     * actor_player while it still expected it to point at e, so
     * messages_say_named(..., e->target, ...) passed the player instead
     */
    nge_entity_id_t target = e->target;
    nge_entity_id_t source = e->source;

    switch (e->id) {
    case EV_STOOD_ON:
        if (source == actor_player) {
            messages_say_named("You see a ", target, " here.");
        }
        break;
    case EV_PICKED_UP:
        if (source == actor_player) {
            messages_say_named("You pick up a ", target, ".");
        }
        break;
    case EV_DROPPED:
        if (source == actor_player) {
            messages_say_named("You drop a ", target, ".");
        }
        break;
    case EV_EQUIPPED:
        if (source == actor_player) {
            messages_say_named("You equip a ", target, ".");
        }
        break;
    case EV_UNEQUIPPED:
        if (source == actor_player) {
            messages_say_named("You take off a ", target, ".");
        }
        break;
    case EV_GLOWED:
        if (target == actor_player) {
            screen_message("A warm glow surrounds you.");
        }
        break;
    case EV_GLOW_FADED:
        if (target == actor_player) {
            screen_message("The warm glow fades.");
        }
        break;
    default:
        break;
    }
}
