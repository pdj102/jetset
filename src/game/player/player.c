/*
 * The player's handlers in common memory: the dispatch functions (game.c)
 * call them for every timer firing and event. The commands and screens are
 * banked, in player_bank.c.
 */
#include "game/player/player.h"

#include "core/nge_util.h"

#include "game/actors.h"
#include "game/components/health.h"
#include "game/components/mana.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/ui/screen_main.h"

uint8_t player_turn;

void player_init(void)
{
    player_turn = 0;
}

void player_on_turn(nge_entity_id_t id)
{
    (void)id;
    nge_util_assert(effects_check(id));   /* the derived values are up to date */
    player_turn = 1;   /* until a command uses the turn */
}

void player_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* a local: see messages_on_event() */

    player_on_pc_event(e);   /* every event: it chooses, in banked code, to keep common memory free */
    if (target != actor_player) {
        return;
    }
    switch (e->id) {
    case EV_DAMAGED:
    case EV_HEALED:
        screen_show_hp(health_hp[target], health_hp_max[target]);
        break;
    case EV_MANA_SPENT:
    case EV_MANA_RESTORED:
        screen_show_mp(mana_mp[target], mana_mp_max[target]);
        break;
    case EV_ATTRS_CHANGED:
        player_show_stats();
        break;
    default:
        break;
    }
}
