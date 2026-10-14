#include "game/game.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/systems/nge_render.h"
#include "hw/nge_frame.h"
#include "map/nge_map.h"

#include "game/actors.h"
#include "game/level.h"
#include "game/screen.h"

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

/* ---- Turns ---- */

/* After each command that takes time: later, the monsters' turn */
static void end_turn(void)
{
    game_turn++;
    screen_show_turn(game_turn);
}

/* ---- States: entry actions and per-frame updates ---- */

static void title_enter(void)
{
    screen_message("NGE ROGUE. Press any key to start.");
}

static void title_update(void)
{
    if (nge_key_get() == NGE_KEY_NONE) {
        return;
    }
    screen_ask_name();   /* blocking: the screen stands still while the player types */
    screen_draw_stats(game_turn);
    screen_message("Welcome to the dungeon.");
    screen_message("Move: cursor keys or h j k l.");
    screen_message("i inventory, f fire, Q quit.");
    game_set_state(GAME_PLAY);
}

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

/* Entry actions, by state; NULL for none */
static void (*const enter_fn[GAME_STATES])(void) = {
    title_enter,
    NULL,
    inventory_enter,
    target_enter,
    dead_enter,
};

void game_set_state(game_state_t state)
{
    nge_util_assert(state < GAME_STATES);

    nge_key_flush();
    game_state = state;
    if (enter_fn[state] != NULL) {
        enter_fn[state]();
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
    nge_render_update();    /* build the sprites for the next frame */
    nge_ecs_check_guard();
}

void game_run(void)
{
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
