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
#include "game/components/item.h"
#include "game/components/kind.h"
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

/*
 * Do an action for the player: on a refusal they are told why and keep
 * their turn; if the turn is used, the clock runs again (the next frame's
 * time passing runs the other actors' turns)
 */
static void act(uint8_t action, act_t *a)
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
#define LIST_MAX   26   /* items in a list: one per letter */

/* Which items a list shows */
#define LIST_STOWED      0   /* in the pack, in ID order */
#define LIST_EQUIPPABLE  1   /* in the pack, fitting a slot the player has */
#define LIST_EQUIPPED    2   /* in use, in slot order */

/* Where an equipped item is, after its name in lists */
static const char *const slot_words[SLOT_COUNT] = {
    [SLOT_NONE]   = "",
    [SLOT_WEAPON] = " (in hand)",
    [SLOT_SHIELD] = " (on arm)",
    [SLOT_HEAD]   = " (on head)",
    [SLOT_BODY]   = " (worn)",
    [SLOT_HANDS]  = " (on hands)",
    [SLOT_FEET]   = " (on feet)",
    [SLOT_NECK]   = " (around neck)",
    [SLOT_RING_L] = " (left hand)",
    [SLOT_RING_R] = " (right hand)",
    [SLOT_LIGHT]  = " (lit)",
};

/* The screen's list (for its letters): set when a screen opens */
static uint8_t list_which;

/* The items in a list, into ids (LIST_MAX at most). Returns how many */
static uint8_t collect(uint8_t which, nge_entity_id_t *ids)
{
    nge_entity_id_t id = NGE_ENTITY_INVALID;
    uint8_t n = 0;
    uint8_t slot;

    if (which == LIST_EQUIPPED) {
        for (slot = 1; slot < SLOT_COUNT; slot++) {
            id = inv_equipped(actor_player, slot);
            if (id != NGE_ENTITY_INVALID) {
                ids[n++] = id;
            }
        }
        return n;
    }
    while (n < LIST_MAX && (id = inv_next(actor_player, id)) != NGE_ENTITY_INVALID) {
        if (which == LIST_STOWED || (gen_item_fits(kind_index[id]) & inv_slots[actor_player]) != 0) {
            ids[n++] = id;
        }
    }
    return n;
}

/*
 * Show a list, lettered: "a) sword  b) ring (left hand)", as many to a
 * message line as fit. Returns how many items it has
 */
static uint8_t list_items(uint8_t which)
{
    nge_entity_id_t ids[LIST_MAX];
    char line[LIST_LINE];
    char entry[LIST_LINE];
    uint8_t n = collect(which, ids);
    uint8_t i;

    line[0] = '\0';
    for (i = 0; i < n; i++) {
        entry[0] = (char)('a' + i);
        entry[1] = ')';
        entry[2] = ' ';
        gen_name(ids[i], entry + 3, NAME_MAX);
        strcat(entry, slot_words[item_slot[ids[i]]]);
        if (strlen(line) + 2 + strlen(entry) > LIST_LINE - 1) {
            screen_message(line);
            line[0] = '\0';
        }
        if (line[0] != '\0') {
            strcat(line, "  ");
        }
        strcat(line, entry);
    }
    if (n != 0) {
        screen_message(line);
    }
    return n;
}

/* The item for a letter key in the screen's list, or NGE_ENTITY_INVALID */
static nge_entity_id_t item_for_key(nge_key_event_t ev)
{
    nge_entity_id_t ids[LIST_MAX];
    char c = (char)nge_key_ascii(ev);
    uint8_t n = collect(list_which, ids);

    if (c < 'a' || c >= 'a' + n) {
        return NGE_ENTITY_INVALID;
    }
    return ids[c - 'a'];
}

/* Do action on the item chosen by a letter key in the screen's list ("Never mind." for another key) */
static void act_on_chosen(uint8_t action, nge_key_event_t ev)
{
    act_t a = { 0 };

    a.item = item_for_key(ev);
    if (a.item == NGE_ENTITY_INVALID) {
        screen_message("Never mind.");
        return;
    }
    a.kind = SLOT_NONE;   /* equip: the first free slot it fits */
    act(action, &a);
}

/*
 * Open a choosing screen on a list: show it and the question, or, if it is
 * empty, say so and return to play. Returns 1 if the screen stays open
 */
static uint8_t open_choice(uint8_t which, const char *question, const char *empty)
{
    list_which = which;
    if (list_items(which) == 0) {
        screen_message(empty);
        game_set_state(GAME_PLAY);
        return 0;
    }
    screen_message(question);
    return 1;
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

/* ---- Screens: the inventory, dropping, equipping, taking off, firing ---- */

void player_inventory_enter(void) NGE_BANKED
{
    uint8_t n = list_items(LIST_STOWED);

    n += list_items(LIST_EQUIPPED);
    if (n == 0) {
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
    open_choice(LIST_STOWED, "Drop which item?", "You have nothing to drop.");
}

void player_equip_enter(void) NGE_BANKED
{
    open_choice(LIST_EQUIPPABLE, "Equip which item?", "You have nothing to equip.");
}

void player_unequip_enter(void) NGE_BANKED
{
    open_choice(LIST_EQUIPPED, "Take off which item?", "You aren't using anything.");
}

/* The choosing screens' update: on a key, act on the item chosen and return to play */
static void choice_update(uint8_t action)
{
    nge_key_event_t ev = nge_key_get();

    if (ev == NGE_KEY_NONE) {
        return;
    }
    act_on_chosen(action, ev);
    game_set_state(GAME_PLAY);
}

void player_drop_update(void) NGE_BANKED
{
    choice_update(ACT_DROP);
}

void player_equip_update(void) NGE_BANKED
{
    choice_update(ACT_EQUIP);
}

void player_unequip_update(void) NGE_BANKED
{
    choice_update(ACT_UNEQUIP);
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
