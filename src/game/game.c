#include "game/game.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "hw/nge_frame.h"
#include "input/nge_input.h"
#include "map/nge_map.h"

#include "event/nge_event.h"

#include "game/actors.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/messages.h"
#include "game/level/level.h"
#include "game/player/player.h"
#include "game/screen.h"
#include "game/timers.h"
#include "game/title/title.h"

/* Clock steps (nge_timer_advance) per frame at most, so the screen keeps animating */
#define ADVANCES_PER_FRAME  32

game_state_t game_state = GAME_TITLE;

static uint16_t shown_turn;   /* the turn count on the screen */

/* ---- Events ---- */

/* The dispatch function: each system's reaction, in a fixed order. In common memory */
static void game_on_event(const nge_event_t *e)
{
    player_on_event(e);
    messages_on_event(e);
}

/* ---- Timers ---- */

/*
 * The dispatch function: the handler for each timer kind (game/timers.h).
 * In common memory
 */
static void game_on_timer(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    switch (kind) {
    case TIMER_PLAYER:
        player_on_turn(id);
        break;
    case TIMER_GLOW:
        actors_on_glow(id, cycles);
        break;
    case TIMER_MONSTER:
        break;              /* no AI yet: it waits */
    default:
        nge_util_abort("bad timer kind");
        break;
    }
}

/* ---- Turns ---- */

/*
 * Time passes until the clock stops for the player's turn, each step's
 * events handled before the next, up to ADVANCES_PER_FRAME steps a frame
 */
static void pass_time(void)
{
    uint8_t steps = ADVANCES_PER_FRAME;

    while (steps-- > 0 && nge_timer_advance(game_on_timer)) {
        nge_event_process(game_on_event);
    }
    if (nge_timer_turns != shown_turn) {
        shown_turn = nge_timer_turns;
        screen_show_turn(shown_turn);
    }
}

/* ---- States: entry actions and per-frame updates ---- */

/* Time passes until the player's turn, then at most one command per frame, from the key events */
static void play_update(void)
{
    nge_key_event_t ev;

    pass_time();
    if (!nge_timer_stopped()) {
        return;   /* not the player's turn yet: more time passes next frame */
    }
    ev = nge_key_get();
    if (ev != NGE_KEY_NONE) {
        player_command(ev);
    }
}

/*
 * A new game: every module the game uses back to its initial state (each
 * init can be called again), a new player in the first level, then play.
 * The player's name and the message window are kept. A passing state: it
 * never runs a frame update
 */
static void new_game_enter(void)
{
    nge_entity_init();       /* every entity free: the last game's are gone */
    nge_render_init();       /* their sprites hidden at the next upload */
    nge_render_set_view(LEVEL_AREA);
    nge_timer_init();        /* turn 0, the clock running */
    nge_event_init();
    actors_create();
    level_generate(1, actor_player);   /* the first level; the camera on the player */
    shown_turn = 0;
    screen_draw_stats(nge_timer_turns);
    game_set_state(GAME_PLAY);
}

static void dead_enter(void)
{
    screen_message("Game over. Press any key.");
}

static void dead_update(void)
{
    if (nge_key_get() != NGE_KEY_NONE) {
        game_set_state(GAME_TITLE);
    }
}

void game_set_state(game_state_t state)
{
    nge_util_assert(state < GAME_STATES);

    nge_key_flush();
    game_state = state;

    /*
     * The entry action: called directly, not through a table of function
     * pointers, so banked states (the title) work: a pointer to a banked
     * function doesn't switch banks
     */
    switch (state) {
    case GAME_TITLE:
        title_enter();
        break;
    case GAME_NEW_GAME:
        new_game_enter();   /* ends by changing to GAME_PLAY */
        break;
    case GAME_INVENTORY:
        player_inventory_enter();
        break;
    case GAME_DROP:
        player_drop_enter();
        break;
    case GAME_EQUIP:
        player_equip_enter();
        break;
    case GAME_UNEQUIP:
        player_unequip_enter();
        break;
    case GAME_TARGET:
        player_target_enter();
        break;
    case GAME_DEAD:
        dead_enter();
        break;
    default:
        break;   /* no entry action */
    }
}

/* ---- The frame loop ---- */

/* 1. Before the state's update: first thing in the blanking period, so nothing changes mid-frame */
static void pre_update(void)
{
    nge_render_upload();
    nge_map_update();
    nge_input_update();
}

/*
 * Release the entities destroyed this frame (after their events are
 * handled). What they own: items fall to the floor where they were (if
 * they were on the map), effects are destroyed with them
 */
static void release_dying(void)
{
    nge_entity_id_t id;
    nge_entity_id_t owned;
    uint8_t i;

    while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
        inv_drop_all(id);
        NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
            if (!ITEM_IS(owned)) {
                nge_entity_destroy(owned);
            }
        }
        /* The rest (items it held off the map, dying effects) pass to its owner */
        nge_owner_move_all(id, NGE_OWNER_OF(id));
        nge_entity_release(id);
    }
}

/* 3. After the state's update */
static void post_update(void)
{
    nge_event_process(game_on_event);   /* events posted outside turns */
    release_dying();
    nge_render_update();    /* build the sprites for the next frame */
    nge_ecs_check_guard();
}

void game_run(void)
{
    nge_event_init();
    game_set_state(GAME_TITLE);

    while (1) {
        nge_frame_wait_vblank();
        pre_update();

        switch (game_state) {
        case GAME_TITLE:
            title_update();
            break;
        case GAME_NEW_GAME:
            break;   /* a passing state: its entry action has already moved on */
        case GAME_PLAY:
            play_update();
            break;
        case GAME_INVENTORY:
            player_inventory_update();
            break;
        case GAME_DROP:
            player_drop_update();
            break;
        case GAME_EQUIP:
            player_equip_update();
            break;
        case GAME_UNEQUIP:
            player_unequip_update();
            break;
        case GAME_TARGET:
            player_target_update();
            break;
        case GAME_DEAD:
            dead_update();
            break;
        default:
            nge_util_abort("bad game state");
            break;
        }

        post_update();
    }
}
