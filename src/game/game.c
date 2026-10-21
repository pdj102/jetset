/*
 * The main loop, the play loop and the frames: common memory. The title
 * (game/title/title_bank.c) and the choosers are banked and called
 * directly, never through function pointers: a pointer to a banked
 * function doesn't switch banks (N-BANK-6). The dispatch functions
 * game_on_event() and game_on_timer() are here too: nge calls them through
 * pointers, so they must be in common memory.
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
#include "game/anim/anim.h"
#include "game/dev.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/screen.h"
#include "game/timers.h"
#include "game/title/title.h"

/*
 * Advances of the clock (nge_timer_advance()) per frame at most, so a long
 * stretch of the world's time doesn't freeze the screen: it carries on next
 * frame
 */
#define CLOCK_ADVANCES_MAX  32

static uint16_t shown_turn;   /* the turn count on the screen */
static uint16_t frames;       /* frames since the start: a release build's random seed */
static uint8_t  frame_open;   /* 1 between game_frame_start() and game_frame_end() */
static uint8_t  ended;        /* 1 once game_over() has ended the game */
static uint8_t  end_reason;   /* GAME_OVER_*: why it ended */

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
    anim_on_event(e);    /* first: the animations start in the frame the action happened */
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

/* ---- Frames ---- */

void game_frame_start(void)
{
    if (frame_open) {
        game_frame_end();   /* a frame a chooser left open: its sprites built first */
    }
    nge_frame_wait_vblank();
    frames++;
    frame_open = 1;
    /* First thing in the blanking period, so nothing changes mid-frame */
    nge_render_upload();
    screen_overlay_upload();
    nge_map_update();
    nge_input_update();
}

void game_frame_end(void)
{
    nge_render_update();    /* build the sprites for the next frame */
    frame_open = 0;
}

nge_key_event_t game_wait_key(uint16_t frames_max)
{
    nge_key_event_t ev;
    uint16_t waited;

    for (waited = 0;; waited++) {
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE || (frames_max != 0 && waited == frames_max)) {
            return ev;
        }
        game_frame_start();   /* ends the last frame first */
    }
}

/* ---- Turns ---- */

/*
 * The world may act: it isn't the player's turn, no change of level is
 * waiting (nothing more happens on the level being left), the game isn't
 * over, and no animation in view is playing (the world waits for the
 * screen to show what happened). The animations last: only asked when
 * the rest would let the world act
 */
static uint8_t world_can_act(void)
{
    return !player_turn && !level_pending() && !ended && !anim_holding();
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
 * happen in order: once the player dies, game over stops the clock and
 * nothing after that acts. The animations those events start in view stop
 * the next advance (world_can_act()), so everything that acted at the same
 * tick animates together, and the world goes on when they end
 */
static void pass_time(void)
{
    uint8_t advances;

    for (advances = 0; advances < CLOCK_ADVANCES_MAX && world_can_act(); advances++) {
        if (!nge_timer_advance(game_on_timer)) {
            break;   /* the clock is stopped (game over), or no entity has a timer */
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

/*
 * The turn engine: the world acts until it is the player's turn, then the
 * player acts, at most one command a frame from the key events, once the
 * animations in view have ended. A command
 * that uses the turn clears player_turn (player_act()), and the world acts
 * again from the next frame. A command may call choosers, which run frames
 * until the player answers. The two ifs aren't an if/else: when the
 * world's time runs up to the player's turn in this frame, their command
 * is taken in the same frame
 */
static void play_turn(void)
{
    nge_key_event_t ev;

    if (!player_turn) {
        pass_time();
        show_turn();
    }
    /* A key waits, queued, until the animations end: only asked about when there is one */
    if (player_turn && !ended && nge_key_waiting() != 0 && !anim_holding()) {
        ev = nge_key_get();
        player_command(ev);
    }
}

/* ---- The play loop ---- */

void game_play_help(void)
{
    screen_help("g:get i:inv u:use w:wear r:off <>:stairs");
}

void game_play_redraw(void)
{
    nge_render_set_view(LEVEL_AREA);
    screen_show_map();   /* the title's panel, or a screen's, gone */
    nge_map_invalidate();
    screen_draw_stats();
    player_show_stats();
    screen_show_turn(nge_timer_turns);
    screen_show_depth(level_depth);
    game_play_help();
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
    if (ended) {
        return;
    }
    if (depth == LEVEL_DEPTH_SURFACE) {
        game_over(GAME_OVER_ESCAPED);
        return;
    }
    level_generate(depth, actor_player, arrival);
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, depth, arrival);
    game_play_redraw();   /* the new area's sprites, the map, the stats and the depth */
    nge_event_process(game_on_event);   /* the arrival, before the clean-up releases what parking ended */
}

/* The world's step, after the turn: in the play loop only */
static void play_step(void)
{
    nge_event_process(game_on_event);   /* the frame's events: the player's command */
    if (level_pending()) {
        change_level();
    }
    level_release_dying();
    nge_ecs_check_guard();
}

/*
 * A new game: every module the game uses back to its initial state (each
 * init can be called again), the persistent entities, the player in the
 * first level. The player's name and the message window are kept
 */
static void play_start(void)
{
#if !GAME_DEV_KIT
    srand(frames);           /* once a game: each visit to a depth is new (debug builds seed per level) */
#endif
    nge_entity_init();       /* every entity free: the last game's are gone */
    nge_render_init();       /* their sprites hidden at the next upload */
    nge_timer_init();        /* turn 0, the clock running */
    nge_event_init();
    player_init();           /* not the player's turn until their timer fires */
    level_clear_request();
    ended = 0;
    actors_create();
    nge_render_follow(actor_player);   /* the camera slides with them (after nge_render_init()) */
    level_generate(1, actor_player, LEVEL_ARRIVE_ANY);   /* the first level; the camera on the player */
    shown_turn = 0;
    nge_key_flush();         /* keys typed at the title don't move the player */
    game_play_redraw();
}

/* One game, from the first level until game_over(): why it ended (GAME_OVER_*) */
static uint8_t play_run(void)
{
    play_start();
    while (!ended) {
        game_frame_start();
        play_turn();
        play_step();
        game_frame_end();
    }
    return end_reason;
}

void game_over(uint8_t reason)
{
    nge_util_assert(reason < GAME_OVER_COUNT);

    end_reason = reason;
    ended = 1;
    nge_timer_stop();   /* nothing more acts, even in this frame's remaining advances */
}

/* ---- The main loop ---- */

/* Why the game ended, until a key: the last level stays on the screen under it */
static void over_run(uint8_t reason)
{
    nge_key_flush();
    screen_message(reason == GAME_OVER_ESCAPED ? "You leave the dungeon. Press any key."
                                               : "Game over. Press any key.");
    screen_help("Any key: back to the title");
    game_wait_key(0);
}

void game_run(void)
{
    nge_event_init();
    for (;;) {
        title_run();
        over_run(play_run());
    }
}
