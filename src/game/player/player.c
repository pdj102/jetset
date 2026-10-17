/*
 * The player's handlers in common memory: the dispatch functions (game.c)
 * call them for every timer firing and event. The commands and screens are
 * banked, in player_bank.c.
 */
#include "game/player/player.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_timer.h"
#include "map/nge_map.h"

#include "game/actors.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/screen.h"

void player_on_turn(nge_entity_id_t id)
{
    (void)id;
    nge_util_assert(effects_check(id));   /* the derived values are up to date */
    nge_timer_stop();   /* until a command uses the turn */
}

void player_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* a local: see messages_on_event() */

    if (target != actor_player) {
        return;
    }
    switch (e->id) {
    case EV_MOVED:
        nge_map_centre_camera(NGE_LOCATION_X(target), NGE_LOCATION_Y(target));
        break;
    case EV_DAMAGED:
    case EV_HEALED:
        screen_show_hp(health_hp[target], health_hp_max[target]);
        break;
    case EV_ATTRS_CHANGED:
        screen_show_hp(health_hp[target], health_hp_max[target]);
        screen_show_ac(health_ac[target]);
        screen_show_status(status_mask[target]);
        break;
    default:
        break;
    }
}
