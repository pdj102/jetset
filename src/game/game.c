#include "game/game.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "hw/nge_frame.h"
#include "map/nge_map.h"

#include "event/nge_event.h"

#include "game/actors.h"
#include "game/components/stats.h"
#include "game/messages.h"
#include "game/level/level.h"
#include "game/screen.h"
#include "game/timers.h"
#include "game/title/title.h"

/* Characters of the movement keys (nge_key_ascii): cursor keys */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9
#define CHAR_DOWN   10
#define CHAR_UP     11

/* Clock steps (nge_timer_advance) per frame at most, so the screen keeps animating */
#define ADVANCES_PER_FRAME  32

game_state_t game_state = GAME_TITLE;

static uint16_t shown_turn;   /* the turn count on the screen */

uint8_t game_key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy)
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

/* ---- Events ---- */

/* The dispatch function: each system's reaction, in a fixed order. In common memory */
static void game_on_event(const nge_event_t *e)
{
    messages_on_event(e);
}

/* ---- Timers ---- */

/*
 * The dispatch function: the handler for each timer kind (game/timers.h).
 * In common memory
 */
static uint8_t game_on_timer(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    switch (kind) {
    case TIMER_PLAYER:
        return NGE_TIMER_WAIT;   /* until the player's command */
    case TIMER_GLOW:
        return actors_on_glow(id, cycles);
    case TIMER_MONSTER:
        return stats_speed[id];   /* no AI yet: it waits a move */
    default:
        nge_util_abort("bad timer kind");
        return NGE_TIMER_STOP;
    }
}

/* ---- Turns ---- */

/*
 * Time passes until the clock waits for the player, each step's events
 * handled before the next, up to ADVANCES_PER_FRAME steps a frame
 */
static void pass_time(void)
{
    uint8_t steps = ADVANCES_PER_FRAME;

    while (steps-- > 0 && nge_timer_waiting() == NGE_ENTITY_INVALID &&
           nge_timer_advance(game_on_timer)) {
        nge_event_process(game_on_event);
    }
    if (nge_timer_turns != shown_turn) {
        shown_turn = nge_timer_turns;
        screen_show_turn(shown_turn);
    }
}

/* After a command that takes time: its events, then time passes for its cost */
static void player_acted(uint8_t cost)
{
    nge_event_process(game_on_event);
    nge_timer_set_delay(actor_player, cost);
    pass_time();
}

/* ---- States: entry actions and per-frame updates ---- */

/* Time passes until the player's turn, then at most one command per frame, from the key events */
static void play_update(void)
{
    nge_key_event_t ev;
    int8_t dx;
    int8_t dy;

    pass_time();
    if (nge_timer_waiting() != actor_player) {
        return;
    }
    ev = nge_key_get();
    if (ev == NGE_KEY_NONE) {
        return;
    }
    if (game_key_direction(ev, &dx, &dy)) {
        if (actors_move_player(dx, dy)) {
            player_acted(COST_MOVE);
        }
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

static void inventory_enter(void)
{
    screen_message("You are carrying nothing. (any key)");
}

static void inventory_update(void)
{
    if (nge_key_get() != NGE_KEY_NONE) {
        game_set_state(GAME_PLAY);
    }
}

static void target_enter(void)
{
    screen_message("Fire in which direction?");
}

static void target_update(void)
{
    nge_key_event_t ev = nge_key_get();
    int8_t dx;
    int8_t dy;

    if (ev == NGE_KEY_NONE) {
        return;
    }
    if (game_key_direction(ev, &dx, &dy)) {
        screen_message("You have nothing to fire.");
    } else {
        screen_message("Never mind.");
    }
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
        inventory_enter();
        break;
    case GAME_TARGET:
        target_enter();
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
 * handled). What they own goes too: for now only effects, so destroy them
 * (later, carried items will be dropped instead)
 */
static void release_dying(void)
{
    nge_entity_id_t id;
    nge_entity_id_t owned;
    uint8_t i;

    while ((id = nge_entity_next_dying()) != NGE_ENTITY_INVALID) {
        NGE_OWNER_FOR_EACH_OWNED(i, owned, id) {
            nge_entity_destroy(owned);
        }
        /* The dying ones it owns wait on the dying stack: pass them on until released */
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
            inventory_update();
            break;
        case GAME_TARGET:
            target_update();
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
