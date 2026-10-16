/*
 * The player: what their keys do, their turn, and the view following them
 * (see player.h). In common memory: called every frame of play.
 */
#include "game/player.h"

#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_timer.h"
#include "map/nge_map.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/events.h"
#include "game/game.h"
#include "game/messages.h"
#include "game/screen.h"

/* Characters of the movement keys (nge_key_ascii): cursor keys */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9
#define CHAR_DOWN   10
#define CHAR_UP     11

void player_on_turn(nge_entity_id_t id)
{
    (void)id;
    nge_timer_stop();   /* until a command uses the turn */
}

uint8_t player_key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy)
{
    *dx = 0;
    *dy = 0;
    switch (nge_key_ascii(ev)) {
    case CHAR_LEFT:
    case 'h':
        *dx = -1;
        return 1;
    case CHAR_RIGHT:
    case 'l':
        *dx = 1;
        return 1;
    case CHAR_UP:
    case 'k':
        *dy = -1;
        return 1;
    case CHAR_DOWN:
    case 'j':
        *dy = 1;
        return 1;
    default:
        return 0;
    }
}

/* The move command: on a refusal the player is told why, and keeps their turn */
static void move(int8_t dx, int8_t dy)
{
    act_t a = { 0 };

    a.actor = actor_player;
    a.dx = dx;
    a.dy = dy;
    if (act_move(&a, ACT_DO) == ACT_FAIL) {
        messages_act_failed(&a);
    }
    if (a.used) {
        nge_timer_start();   /* the next frame's time passing runs the other actors' turns */
    }
}

void player_command(nge_key_event_t ev)
{
    int8_t dx;
    int8_t dy;

    if (player_key_direction(ev, &dx, &dy)) {
        move(dx, dy);
        return;
    }
    switch (nge_key_ascii(ev)) {
    case 'i':
        game_set_state(GAME_INVENTORY);
        break;
    case 'f':
        game_set_state(GAME_TARGET);
        break;
    case 'Q':
        screen_message("You quit.");
        game_set_state(GAME_DEAD);
        break;
    default:
        break;   /* other keys: no command, no turn */
    }
}

void player_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* a local: see messages_on_event() */

    if (e->id == EV_MOVED && target == actor_player) {
        nge_map_centre_camera(NGE_LOCATION_X(target), NGE_LOCATION_Y(target));
    }
}
