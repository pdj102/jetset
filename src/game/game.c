/*
 * The main loop and the game states: common memory. Each state's entry
 * action and frame update are called from the two switches here, directly,
 * never through function pointers, so banked states (the title,
 * game/title/title_bank.c) work: a pointer to a banked function doesn't
 * switch banks (N-BANK-6). The dispatch functions game_on_event() and
 * game_on_timer() are here too: nge calls them through pointers, so they
 * must be in common memory.
 */
#include "game/game.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "hw/nge_frame.h"
#include "input/nge_input.h"
#include "map/nge_map.h"

#include "game/actors.h"
#include "game/dev.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/screen.h"
#include "game/timers.h"
#include "game/title/title.h"
#include "game/ui/ui.h"

/*
 * Advances of the clock (nge_timer_advance()) per frame at most, so a long
 * stretch of the world's time doesn't freeze the screen: it carries on next
 * frame
 */
#define CLOCK_ADVANCES_MAX  32

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
 * The dispatch function: nge_timer_advance() calls it for each entity
 * timer that fires, with the timer's kind, and it calls that kind's
 * handler (game/timers.h). In common memory, as nge calls it through a
 * pointer
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
 * The world may act: it isn't the player's turn, and no change of level is
 * waiting (nothing more happens on the level being left)
 */
static uint8_t world_can_act(void)
{
    return !player_turn && !level_pending();
}

/*
 * The world acts: time passes until the world can't act any more, or this
 * frame's CLOCK_ADVANCES_MAX advances run out (it carries on next frame).
 *
 * The clock (ecs/systems/nge_timer.h): each entity with a timer fires every
 * period ticks (an actor every `speed` ticks, an effect every period).
 * Each nge_timer_advance():
 * 1. moves the clock straight on to the next tick at which a timer is due,
 *    however many ticks away, not one tick at a time;
 * 2. fires every timer due at that tick, in entity ID order: there may be
 *    several (the player and two monsters at speed 10);
 * 3. for each, calls game_on_timer() with the entity and its timer's kind,
 *    which calls the kind's handler (a monster's turn, an effect's tick,
 *    player_on_turn() setting player_turn). The rest of the tick still
 *    fires after the player's timer, so everything due at the same time
 *    acts before the player's turn.
 *
 * The events of each advance are handled before the next, so consequences
 * happen in order: once the player dies, game over pauses the clock and
 * nothing after that acts
 */
static void pass_time(void)
{
    uint8_t advances;

    for (advances = 0; advances < CLOCK_ADVANCES_MAX && world_can_act(); advances++) {
        if (!nge_timer_advance(game_on_timer)) {
            break;   /* the clock is paused (game over), or no entity has a timer */
        }
        nge_event_process(game_on_event);   /* this advance's consequences, before the next */
    }
}

/* The turn count on the screen, redrawn when it has changed */
static void show_turn(void)
{
    if (nge_timer_turns != shown_turn) {
        shown_turn = nge_timer_turns;
        screen_show_turn(shown_turn);
    }
}

/* ---- States: entry actions and per-frame updates ---- */

/*
 * The play state's turn engine: the world acts until it is the player's
 * turn, then the player acts, at most one command a frame from the key
 * events. A command that uses the turn clears player_turn (player_act()),
 * and the world acts again from the next frame. The two ifs aren't an
 * if/else: when the world's time runs up to the player's turn in this
 * frame, their command is taken in the same frame
 */
static void play_update(void)
{
    nge_key_event_t ev;

    if (!player_turn) {
        pass_time();
        show_turn();
    }
    if (player_turn) {
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE) {
            player_command(ev);
        }
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
    player_init();           /* not the player's turn until their timer fires */
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
    game_play_help();
}

void game_play_help(void)
{
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

/*
 * 1. Wait for the blanking period, then, before anything else so nothing
 * changes mid-frame: upload the sprites, redraw the map if it changed, and
 * read the keyboard
 */
void game_frame_start(void)
{
    nge_frame_wait_vblank();
    frames++;
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
    if (depth == LEVEL_DEPTH_SURFACE) {
        game_over(GAME_OVER_ESCAPED);
        return;
    }
    level_generate(depth, actor_player, arrival);
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, depth, arrival);
    game_set_state(GAME_PLAY);   /* the new area's sprites, the map, the stats and the depth drawn */
    nge_event_process(game_on_event);   /* the arrival, before the clean-up releases what parking ended */
}

/* 3. After the state's update */
static void post_update(void)
{
    nge_event_process(game_on_event);   /* the state's events: the player's command, a screen's choice */
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
    nge_timer_stop();   /* pause the clock: nothing more acts, even in this frame's remaining advances */
    game_set_state(GAME_OVER);
}

void game_run(void)
{
    nge_event_init();
    game_set_state(GAME_TITLE);

    while (1) {
        game_frame_start();

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
