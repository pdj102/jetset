/*
 * The player's commands and screens: banked in page 114 with their
 * literals (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). They run when a key is pressed, not every frame,
 * and pass their literals only to common code (the screen and messages).
 */
#ifdef __SDCC
#pragma codeseg PAGE_114
#pragma constseg PAGE_114
#endif

#include "game/player/player.h"

#include <string.h>

#include "ecs/systems/nge_timer.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/components/inventory.h"
#include "game/game.h"
#include "game/gen/gen.h"
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

/*
 * Do an action for the player: on a refusal they are told why and keep
 * their turn; if the turn is used, the clock runs again (the next frame's
 * time passing runs the other actors' turns)
 */
static void act(uint8_t action, act_t *a)
{
    a->actor = actor_player;
    if (act_do(action, a, ACT_DO) == ACT_FAIL) {
        messages_act_failed(a);
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
    act(ACT_MOVE, &a);
}

static void pick_up(void)
{
    act_t a = { 0 };

    a.target = NGE_ENTITY_INVALID;   /* the first item here */
    act(ACT_PICKUP, &a);
}

#define LIST_LINE  41   /* a message window line, and its NUL */
#define NAME_MAX   20   /* a kind's name, and its NUL */

/* List what the player has stowed, lettered in ID order: "a) sword  b) potion". Returns how many */
static uint8_t list_items(void)
{
    char line[LIST_LINE];
    char entry[NAME_MAX + 4];
    nge_entity_id_t id = NGE_ENTITY_INVALID;
    uint8_t n = 0;

    line[0] = '\0';
    while ((id = inv_next(actor_player, id)) != NGE_ENTITY_INVALID) {
        /* "a) sword  ": the letter, the name, two spaces between entries */
        entry[0] = (char)('a' + n);
        entry[1] = ')';
        entry[2] = ' ';
        gen_name(id, entry + 3, NAME_MAX);
        if (strlen(line) + 2 + strlen(entry) > LIST_LINE - 1) {
            screen_message(line);
            line[0] = '\0';
        }
        if (line[0] != '\0') {
            strcat(line, "  ");
        }
        strcat(line, entry);
        n++;
    }
    if (n != 0) {
        screen_message(line);
    }
    return n;
}

/* The item for a letter key in that list, or NGE_ENTITY_INVALID */
static nge_entity_id_t item_for_key(nge_key_event_t ev)
{
    char c = (char)nge_key_ascii(ev);
    nge_entity_id_t id = NGE_ENTITY_INVALID;
    uint8_t n;

    if (c < 'a' || c > 'z') {
        return NGE_ENTITY_INVALID;
    }
    for (n = (uint8_t)(c - 'a' + 1); n > 0; n--) {
        id = inv_next(actor_player, id);
        if (id == NGE_ENTITY_INVALID) {
            break;
        }
    }
    return id;
}

static void drop(nge_entity_id_t item)
{
    act_t a = { 0 };

    a.item = item;
    act(ACT_DROP, &a);
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

/* ---- Screens: the inventory, dropping, firing ---- */

void player_inventory_enter(void) NGE_BANKED
{
    if (list_items() == 0) {
        screen_message("You are carrying nothing.");
    }
}

void player_inventory_update(void) NGE_BANKED
{
    if (nge_key_get() != NGE_KEY_NONE) {
        game_set_state(GAME_PLAY);
    }
}

void player_drop_enter(void) NGE_BANKED
{
    if (list_items() == 0) {
        screen_message("You have nothing to drop.");
        game_set_state(GAME_PLAY);
        return;
    }
    screen_message("Drop which item?");
}

void player_drop_update(void) NGE_BANKED
{
    nge_key_event_t ev = nge_key_get();
    nge_entity_id_t item;

    if (ev == NGE_KEY_NONE) {
        return;
    }
    item = item_for_key(ev);
    if (item == NGE_ENTITY_INVALID) {
        screen_message("Never mind.");
    } else {
        drop(item);
    }
    game_set_state(GAME_PLAY);
}

void player_target_enter(void) NGE_BANKED
{
    screen_message("Fire in which direction?");
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
