#include "game/game.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/systems/nge_render.h"
#include "hw/nge_frame.h"
#include "map/nge_map.h"

#include "event/nge_event.h"

#include "game/actors.h"
#include "game/messages.h"
#include "game/level/level.h"
#include "game/screen.h"
#include "game/title/title.h"

/* Characters of the movement keys (nge_key_ascii): cursor keys */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9
#define CHAR_DOWN   10
#define CHAR_UP     11

game_state_t game_state = GAME_TITLE;
uint16_t game_turn;

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

/* ---- Turns ---- */

/* After each command that takes time: its events, then (later) the monsters' turn */
static void end_turn(void)
{
    nge_event_process(game_on_event);
    game_turn++;
    screen_show_turn(game_turn);
}

/* ---- States: entry actions and per-frame updates ---- */

/* The player's turn: at most one command per frame, from the key events */
static void play_update(void)
{
    nge_key_event_t ev = nge_key_get();
    int8_t dx;
    int8_t dy;

    if (ev == NGE_KEY_NONE) {
        return;
    }
    if (game_key_direction(ev, &dx, &dy)) {
        if (actors_move_player(dx, dy)) {
            end_turn();
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
    nge_map_update(&level_map);
    nge_input_update();
}

/* 3. After the state's update */
static void post_update(void)
{
    nge_event_process(game_on_event);   /* events posted outside turns */
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
