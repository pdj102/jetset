/*
 * The player's commands, and the firing screen: banked in page 114 with their
 * literals (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). They run when a key is pressed, not every frame,
 * and pass their literals only to common code (the screen and messages).
 */
#ifdef __SDCC
#pragma codeseg PAGE_114
#pragma constseg PAGE_114
#endif

#include "game/player/player.h"

#include "ecs/systems/nge_timer.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/components/inventory.h"
#include "game/game.h"
#include "game/messages.h"
#include "game/screen.h"

/* Characters of the movement keys (nge_key_ascii): cursor keys */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9
#define CHAR_DOWN   10
#define CHAR_UP     11

/* The direction of a movement key (the cursor keys, also CAPS+5-8, or h j k l): 1 and (dx, dy), or 0 */
static uint8_t key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy)
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

/* Tell the player why their action failed (a->reason), if they need telling */
static void say_failed(const act_t *a)
{
    /* The fields in locals: see messages_on_event() */
    uint8_t reason = a->reason;
    nge_entity_id_t target = a->target;

    switch (reason) {
    case ACT_R_BLOCKED:
        screen_message("You bump into a wall.");
        break;
    case ACT_R_BLOCKED_CREATURE:
        messages_say_named("There is a ", target, " in the way.");
        break;
    case ACT_R_NOTHING_HERE:
        screen_message("There is nothing here to pick up.");
        break;
    case ACT_R_FULL:
        screen_message("You can't carry any more.");
        break;
    case ACT_R_EQUIPPED:
        screen_message("You must take that off first.");
        break;
    case ACT_R_ALREADY_EQUIPPED:
        screen_message("You are already using that.");
        break;
    case ACT_R_CANT_EQUIP:
        screen_message("You can't equip that.");
        break;
    case ACT_R_NO_SLOT:
        screen_message("You have nowhere to put that.");
        break;
    case ACT_R_SLOT_TAKEN:
        messages_say_named("You are already using a ", target, ".");
        break;
    case ACT_R_NOT_EQUIPPED:
        screen_message("You aren't using that.");
        break;
    default:
        break;
    }
}

void player_act(uint8_t action, act_t *a) NGE_BANKED
{
    a->actor = actor_player;
    if (act_do(action, a, ACT_DO) == ACT_FAIL) {
        say_failed(a);
    }
    if (a->used) {
        nge_timer_start();
    }
}

static void move(int8_t dx, int8_t dy)
{
    act_t a = { 0 };

    a.dx = dx;
    a.dy = dy;
    player_act(ACT_MOVE, &a);
}

static void pick_up(void)
{
    act_t a = { 0 };

    a.target = NGE_ENTITY_INVALID;   /* the first item here */
    player_act(ACT_PICKUP, &a);
}

void player_command(nge_key_event_t ev) NGE_BANKED
{
    int8_t dx;
    int8_t dy;

    if (key_direction(ev, &dx, &dy)) {
        move(dx, dy);
        return;
    }
    switch (nge_key_ascii(ev)) {
    case 'g':
    case ',':
        pick_up();
        break;
    case 'i':
        game_set_state(GAME_INVENTORY);
        break;
    case 'd':
        game_set_state(GAME_DROP);
        break;
    case 'w':
        game_set_state(GAME_EQUIP);
        break;
    case 'r':
        game_set_state(GAME_UNEQUIP);
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

/* ---- The firing screen ---- */

void player_target_enter(void) NGE_BANKED
{
    screen_message("Fire in which direction?");
    screen_help("Direction: fire   Other keys: back");
}

void player_target_update(void) NGE_BANKED
{
    nge_key_event_t ev = nge_key_get();
    int8_t dx;
    int8_t dy;

    if (ev == NGE_KEY_NONE) {
        return;
    }
    if (key_direction(ev, &dx, &dy)) {
        screen_message("You have nothing to fire.");
    } else {
        screen_message("Never mind.");
    }
    game_set_state(GAME_PLAY);
}
