/*
 * The player's handlers in common memory: the dispatch functions (game.c)
 * call them for every timer firing and event. The commands and screens are
 * banked, in player_bank.c.
 */
#include "game/player/player.h"

#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_timer.h"
#include "map/nge_map.h"

#include "game/actors.h"
#include "game/events.h"

void player_on_turn(nge_entity_id_t id)
{
    (void)id;
    nge_timer_stop();   /* until a command uses the turn */
}

void player_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* a local: see messages_on_event() */

    if (e->id == EV_MOVED && target == actor_player) {
        nge_map_centre_camera(NGE_LOCATION_X(target), NGE_LOCATION_Y(target));
    }
}
