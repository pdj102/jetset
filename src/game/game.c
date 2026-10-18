#include "game/game.h"

#include <stddef.h>
#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "hw/nge_frame.h"
#include "input/nge_input.h"
#include "map/nge_map.h"

#include "event/nge_event.h"

#include "game/actors.h"
#include "game/components/health.h"
#include "game/dev.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/messages/messages.h"
#include "game/level/level.h"
#include "game/player/player.h"
#include "game/screen.h"
#include "game/timers.h"
#include "game/title/title.h"
#include "game/ui/ui.h"

/* Clock steps (nge_timer_advance) per frame at most, so the screen keeps animating */
#define ADVANCES_PER_FRAME  32

game_state_t game_state = GAME_TITLE;

static uint16_t shown_turn;   /* the turn count on the screen */
static uint16_t frames;       /* frames since the start: a release build's random seed */
static uint8_t  over_reason;  /* GAME_OVER_*: why the game ended */

/* ---- Events ---- */

/* The game's own reaction: the player dying ends the game */
static void death_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* a local: see messages_on_event() */

    if (e->id == EV_DIED && target == actor_player) {
        game_over(GAME_OVER_DIED);
    }
}

/* The dispatch function: each system's reaction, in a fixed order. In common memory */
static void game_on_event(const nge_event_t *e)
{
    player_on_event(e);
    messages_on_event(e);
    death_on_event(e);   /* last: the death message comes before "Game over" */
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
    case TIMER_EFFECT:
        if (level_holds(id)) {   /* off the level, frozen: only effects that last for ever are left there */
            effects_on_timer(id, cycles);
        }
        break;
    case TIMER_MONSTER:
        break;              /* no AI yet: it waits (later: only if level_holds() it) */
    default:
        nge_util_abort("bad timer kind");
        break;
    }
}

/* ---- Turns ---- */

/*
 * Time passes until the clock stops for the player's turn, each step's
 * events handled before the next, up to ADVANCES_PER_FRAME steps a frame.
 * It stops too once a change of level is asked for: nothing more happens
 * on the level being left
 */
static void pass_time(void)
{
    uint8_t steps = ADVANCES_PER_FRAME;

    while (steps-- > 0 && !level_pending() && nge_timer_advance(game_on_timer)) {
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
 * init can be called again), the persistent entities, the player in the
 * first level, then play. The player's name and the message window are
 * kept. A passing state: it never runs a frame update
 */
static void new_game_enter(void)
{
#if !GAME_DEV_KIT
    srand(frames);           /* once a game: each visit to a depth is new (debug builds seed per level) */
#endif
    nge_entity_init();       /* every entity free: the last game's are gone */
    nge_render_init();       /* their sprites hidden at the next upload */
    nge_render_set_view(LEVEL_AREA);
    nge_timer_init();        /* turn 0, the clock running */
    nge_event_init();
    level_clear_request();
    actors_create();
    level_generate(1, actor_player, LEVEL_ARRIVE_ANY);   /* the first level; the camera on the player */
    shown_turn = 0;
    game_set_state(GAME_PLAY);
}

/*
 * Entering play, from a new game or back from a screen: everything a
 * screen may have covered or changed is drawn again. The sprites of the
 * level's area, the map (a screen may have drawn over its window), the
 * stats (equipping may change them) and the play help
 */
static void play_enter(void)
{
    nge_render_set_view(LEVEL_AREA);
    nge_map_invalidate();
    screen_draw_stats();
    player_show_stats();
    screen_show_turn(nge_timer_turns);
    screen_show_depth(level_depth);
    screen_help("g:get i:inv u:use w:wear r:off <>:stairs");
}

static void over_enter(void)
{
    screen_message(over_reason == GAME_OVER_ESCAPED ? "You leave the dungeon. Press any key."
                                                    : "Game over. Press any key.");
    screen_help("Any key: back to the title");
}

static void over_update(void)
{
    if (nge_key_get() != NGE_KEY_NONE) {
        game_set_state(GAME_TITLE);
    }
}

void game_set_state(game_state_t state)
{
    nge_util_assert(state < GAME_STATE_COUNT);

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
    case GAME_PLAY:
        play_enter();
        break;
    case GAME_INVENTORY:
        ui_items_enter(UI_INVENTORY);
        break;
    case GAME_DROP:
        ui_items_enter(UI_DROP);
        break;
    case GAME_EQUIP:
        ui_items_enter(UI_EQUIP);
        break;
    case GAME_UNEQUIP:
        ui_items_enter(UI_UNEQUIP);
        break;
    case GAME_USE:
        ui_items_enter(UI_USE);
        break;
    case GAME_FIRE:
        player_direction_enter(PLAYER_PROMPT_FIRE);
        break;
    case GAME_CLOSE:
        player_direction_enter(PLAYER_PROMPT_CLOSE);
        break;
    case GAME_OVER:
        over_enter();
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
 * Carry out a change of level asked for this frame (level_request()), now
 * that its events are handled: none of them names an entity the old level
 * releases. Not once the game is over (the player died in the same turn).
 * The new level's events (EV_ARRIVED, effects ended by parking) are
 * handled straight away, before the clean-up releases what they name
 */
static void change_level(void)
{
    uint8_t depth;
    uint8_t arrival;

    level_take_request(&depth, &arrival);
    if (game_state == GAME_OVER) {
        return;
    }
    if (depth == 0) {
        game_over(GAME_OVER_ESCAPED);
        return;
    }
    level_generate(depth, actor_player, arrival);
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, depth, arrival);
    game_set_state(GAME_PLAY);   /* the new area's sprites, the map, the stats and the depth drawn */
    nge_event_process(game_on_event);
}

/* 3. After the state's update */
static void post_update(void)
{
    nge_event_process(game_on_event);   /* events posted outside turns */
    if (level_pending()) {
        change_level();
    }
    level_release_dying();
    nge_render_update();    /* build the sprites for the next frame */
    nge_ecs_check_guard();
}

void game_over(uint8_t reason)
{
    over_reason = reason;
    nge_timer_stop();   /* no more turns */
    game_set_state(GAME_OVER);
}

void game_run(void)
{
    nge_event_init();
    game_set_state(GAME_TITLE);

    while (1) {
        nge_frame_wait_vblank();
        frames++;
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
            ui_items_update(UI_INVENTORY);
            break;
        case GAME_DROP:
            ui_items_update(UI_DROP);
            break;
        case GAME_EQUIP:
            ui_items_update(UI_EQUIP);
            break;
        case GAME_UNEQUIP:
            ui_items_update(UI_UNEQUIP);
            break;
        case GAME_USE:
            ui_items_update(UI_USE);
            break;
        case GAME_FIRE:
            player_direction_update(PLAYER_PROMPT_FIRE);
            break;
        case GAME_CLOSE:
            player_direction_update(PLAYER_PROMPT_CLOSE);
            break;
        case GAME_OVER:
            over_update();
            break;
        default:
            nge_util_abort("bad game state");
            break;
        }

        post_update();
    }
}
