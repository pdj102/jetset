/*
 * game/ui: the screens (the inventory, the title) on the fake Layer 2's two
 * buffers, and the player's commands that use them.
 */
#include "test.h"

#include <string.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "input/nge_input.h"
#include "input/nge_input_internal.h"
#include "layer2/nge_layer2.h"
#include "map/nge_map.h"

#include "data/tile_data.h"

#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/feature.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/game.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/events.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/ui/screen.h"
#include "game/ui/screen_inv.h"
#include "game/ui/screen_main.h"
#include "game/ui/screen_title.h"

#define AREA     NGE_OWNER_AREA(0)
#define EV(k)    ((nge_key_event_t)(k))

/* What game.c and actors.c provide in the game (not linked here) */
nge_entity_id_t actor_player;
static int waits;     /* game_wait_key() calls: a prompt waited for a key */
static int paused;    /* game_pause_frame() calls: frames a screen ran */

void game_over(uint8_t reason)
{
    (void)reason;
}

/* Frames: only the development V key (player_bank.c) runs them, and no test presses it */
void game_frame_start(void)
{
}

void game_frame_end(void)
{
}

void game_play_help(void)
{
}

void game_play_redraw(void)
{
}

/* The keys the player presses, one a frame (or one a game_wait_key()), set up with press() */
#define SCRIPT_MAX  40
static nge_key_event_t script[SCRIPT_MAX];
static int script_n;
static int script_i;

static void press(nge_key_event_t ev)
{
    CHECK(script_n < SCRIPT_MAX);
    script[script_n++] = ev;
}

/* The next key pressed, or SPACE (which closes any screen) when none is left: a screen waited for nothing */
static nge_key_event_t next_key(void)
{
    if (script_i < script_n) {
        return script[script_i++];
    }
    CHECK(script_i < script_n);
    return EV(NGE_KEY_SPACE);
}

/* Each buffer as it was in the last frame a screen ran, and which was shown */
static test_l2_cell_t seen[2][32][40];
static uint8_t seen_buffer;

/*
 * A paused frame: the key arrives with it, as nge_input_update() would
 * queue it. Frames with no key left are fine (closing runs one), but a
 * screen still waiting after STARVED_MAX of them waited for nothing: SPACE
 * closes it
 */
#define STARVED_MAX  100
static int starved;

void game_pause_frame(void)
{
    paused++;
    memcpy(seen, test_layer2_buf, sizeof(seen));
    seen_buffer = test_layer2_buffer;
    if (nge_input_count != 0) {
        return;
    }
    if (script_i < script_n || ++starved == STARVED_MAX) {
        nge_input_queue[(nge_input_head + nge_input_count) & (NGE_KEY_QUEUE - 1)] = next_key();
        nge_input_count++;
    }
}

nge_key_event_t game_wait_key(uint16_t frames)
{
    (void)frames;
    waits++;
    memcpy(seen, test_layer2_buf, sizeof(seen));
    return next_key();
}

/* All keys pressed were taken */
#define KEYS_USED()  (script_i == script_n)

static char text[NGE_TILEMAP_WIDTH + 1];

/* Columns x1 to x2 of row y of buffer b as it was in the last frame a screen ran */
static const char *seen_cols(uint8_t b, uint8_t y, uint8_t x1, uint8_t x2)
{
    uint8_t x;

    for (x = x1; x <= x2; x++) {
        text[x - x1] = (char)seen[b][y][x].c;
    }
    text[x2 - x1 + 1] = '\0';
    return text;
}

/* The overlay screen's row y, whole, and its list panel's (columns 0-27), as last seen */
#define OVER_ROW(y)  seen_cols(1, (y), 0, 39)
#define LIST_ROW(i)  seen_cols(1, (uint8_t)(2 + (i)), 0, 27)
#define DETAILS(i)   seen_cols(1, (uint8_t)(2 + (i)), 28, 39)
#define STATUS()     seen_cols(1, 29, 0, 39)
#define HELP()       seen_cols(1, 31, 0, 39)
#define HIGHLIGHTED(i)  (seen[1][2 + (i)][0].paper == SCREEN_HIGHLIGHT && seen[1][2 + (i)][27].paper == SCREEN_HIGHLIGHT)

/* Row y of the main screen (buffer 0), now */
static const char *row(uint8_t y)
{
    test_layer2_row(y, 0, 39, text);
    return text;
}

/* Text starts with s */
static int starts(const char *t, const char *s)
{
    return strncmp(t, s, strlen(s)) == 0;
}

/* The last message: the message window (rows 26-30) fills from the top, then scrolls */
static int said(const char *s)
{
    int y;

    for (y = 30; y >= 26; y--) {
        if (row((uint8_t)y)[0] != ' ') {
            return starts(text, s);
        }
    }
    return 0;
}

/* A screen ran and closed: the overlay shown while it waited, the main screen shown now, every key taken */
#define RAN_AND_CLOSED()  (paused > 0 && seen_buffer == SCREEN_BUFFER_OVERLAY && \
                           test_layer2_buffer == SCREEN_BUFFER_MAIN && KEYS_USED())

/* Start a test of a screen: no frames run yet, no keys left */
static void fresh(void)
{
    paused = 0;
    starved = 0;
    waits = 0;
    script_n = 0;
    script_i = 0;
}

static int stowed(nge_entity_id_t id)
{
    return NGE_OWNER_OF(id) == actor_player && item_slot[id] == SLOT_NONE &&
           !NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION);
}

int main(void)
{
    nge_entity_id_t sword;
    nge_entity_id_t ring1;
    nge_entity_id_t ring2;
    nge_entity_id_t door;
    nge_entity_id_t chosen;
    nge_entity_id_t many[29];
    static test_l2_cell_t main_before[32][40];
    uint8_t x;
    uint8_t y;

    test_init();

    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    screen_init();
    nge_map_init(LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);    /* floor everywhere */
    nge_map_clear_blocks(NGE_BLOCK_SOLID);
    nge_map_set_block('.', NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_SHUT, NGE_BLOCK_NONE);
    nge_map_set_block(LOOK_DOOR_OPEN, NGE_BLOCK_NONE);
    for (y = 0; y < LEVEL_HEIGHT; y++) {
        for (x = 0; x < LEVEL_WIDTH; x++) {
            NGE_MAP_TILE(x, y).tile_id = '.';
        }
    }
    actor_player = gen_creature(CR_PLAYER, AREA);
    nge_location_set(actor_player, 5, 5);
    nge_render_set_view(AREA);
    nge_event_flush();   /* the player's innate effect started */

    /* ---- the stats window: the player's stats, from the base record ---- */
    screen_draw_stats();
    player_show_stats();
    test_layer2_row(2, 28, 39, text);
    CHECK(strcmp(text, "HP   12/ 12 ") == 0);
    test_layer2_row(3, 28, 39, text);
    CHECK(strcmp(text, "AC       10 ") == 0);
    test_layer2_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      16 ") == 0);
    test_layer2_row(10, 28, 39, text);
    CHECK(strcmp(text, "Cha      10 ") == 0);
    test_layer2_row(12, 28, 39, text);
    CHECK(strncmp(text, "Gold", 4) == 0);
    effect_apply(EF_STRENGTH, actor_player, NGE_ENTITY_INVALID);
    player_show_stats();
    test_layer2_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      17 ") == 0);
    nge_event_flush();

    /* ---- the inventory, empty: titled, says so, SPACE goes back; the main screen untouched ---- */
    memcpy(main_before, test_layer2_buf[0], sizeof(main_before));
    fresh();
    press(EV(NGE_KEY_A));                      /* not a key of the screen: it stays */
    press(EV(NGE_KEY_SPACE));
    player_turn = 1;
    player_command(EV(NGE_KEY_I));
    CHECK(RAN_AND_CLOSED() && paused == 3);   /* a frame a key, and the frame that shows the main screen */
    CHECK(strcmp(OVER_ROW(0), "          Worldly Possessions           ") == 0);   /* centred */
    CHECK(starts(LIST_ROW(0), " You are carrying nothing."));
    CHECK(starts(DETAILS(26), "Pack  0/20"));
    CHECK(starts(HELP(), "jk:select SPACE:back"));
    CHECK(seen[1][0][0].paper == SCREEN_PAPER);                     /* the whole display covered */
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);
    CHECK(player_turn && nge_event_count() == 0);
    CHECK(nge_render_area == AREA);                                 /* the sprites aren't hidden: buffer 1 covers them */

    /* ---- browse: what is in use (by slot), then the pack (by ID); the cursor highlighted ---- */
    sword = gen_item(IT_SWORD, actor_player);
    ring1 = gen_item(IT_RING, actor_player);
    ring2 = gen_item(IT_RING, actor_player);
    item_slot[ring1] = SLOT_RING_L;
    inv_capacity[actor_player] = 20;
    fresh();
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen) == SCREEN_INV_BACK && RAN_AND_CLOSED());
    CHECK(starts(LIST_ROW(0), " ring of health (left)"));
    CHECK(starts(LIST_ROW(1), " sword "));
    CHECK(starts(LIST_ROW(2), " ring of health "));
    CHECK(strcmp(LIST_ROW(3), "                            ") == 0);
    CHECK(HIGHLIGHTED(0) && !HIGHLIGHTED(1));
    CHECK(starts(DETAILS(0), "ring of heal") && starts(DETAILS(1), "th "));   /* the name, wrapped */
    CHECK(starts(DETAILS(26), "Pack  2/20"));                       /* the ring in use doesn't count */
    CHECK(starts(HELP(), "jk:select r:off SPACE:back"));            /* in use: only taking it off */
    /* The cursor: down (the cursor key or j), the highlight and the help follow */
    fresh();
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_J));
    press(EV(NGE_KEY_K));
    press(EV(NGE_KEY_SPACE));
    screen_inv_run(SCREEN_INV_BROWSE, &chosen);
    CHECK(RAN_AND_CLOSED() && HIGHLIGHTED(1) && !HIGHLIGHTED(0) && !HIGHLIGHTED(2));
    CHECK(starts(DETAILS(0), "sword "));
    CHECK(starts(HELP(), "jk:select w:wear d:drop SPACE:back"));

    /* ---- browse: an action key gives the action and the item; nothing is done to it here ---- */
    fresh();
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_D));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen) == ACT_DROP && chosen == sword && RAN_AND_CLOSED());
    CHECK(stowed(sword) && nge_event_count() == 0);
    /* Refused: why, on the status line; the screen stays */
    fresh();
    press(EV(NGE_KEY_D));                       /* the ring in use */
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen) == SCREEN_INV_BACK && RAN_AND_CLOSED());
    CHECK(starts(STATUS(), "You must take that off first."));
    CHECK(item_slot[ring1] == SLOT_RING_L && nge_event_count() == 0);
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);   /* not in the message window */
    /* Moving on clears it */
    fresh();
    press(EV(NGE_KEY_U));                       /* a ring does nothing when used */
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_SPACE));
    screen_inv_run(SCREEN_INV_BROWSE, &chosen);
    CHECK(strcmp(STATUS(), "                                        ") == 0);

    /* ---- i, then the command does it: d drops the sword, the turn used, the message on the main screen ---- */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_D));
    player_command(EV(NGE_KEY_I));
    CHECK(RAN_AND_CLOSED());
    CHECK(NGE_OWNER_OF(sword) == AREA && NGE_LOCATION_X(sword) == 5 && !player_turn);
    nge_event_process(messages_on_event);
    CHECK(said("You drop a sword."));
    /* ENTER: the obvious action, taking off what is in use */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_ENTER));
    player_command(EV(NGE_KEY_I));
    CHECK(RAN_AND_CLOSED() && item_slot[ring1] == SLOT_NONE && !player_turn);
    nge_event_flush();
    inv_stow(sword, actor_player);   /* back in the pack */

    /* ---- choose: d, w, r and u list what the action applies to, lettered; a letter chooses ---- */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_E));                       /* past the list: nothing */
    press(EV(NGE_KEY_C));
    player_command(EV(NGE_KEY_D));
    CHECK(RAN_AND_CLOSED());
    CHECK(starts(OVER_ROW(0), "             Shed a Burden"));         /* (40 - 13) / 2 */
    CHECK(starts(LIST_ROW(0), " a) sword") && starts(LIST_ROW(1), " b) ring of health"));
    CHECK(starts(LIST_ROW(2), " c) ring of health") && starts(LIST_ROW(3), "    "));
    CHECK(HIGHLIGHTED(2));                      /* the letter moved the cursor */
    CHECK(starts(HELP(), "Letter/ENTER: drop it  SPACE: back"));
    CHECK(NGE_OWNER_OF(ring2) == AREA && !player_turn);
    nge_event_flush();
    /* SPACE: nothing done, the turn kept */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_SPACE));
    player_command(EV(NGE_KEY_D));
    CHECK(RAN_AND_CLOSED() && stowed(sword) && player_turn);
    /* The cursor and ENTER choose too */
    fresh();
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_ENTER));
    CHECK(screen_inv_run(ACT_DROP, &chosen) == ACT_DROP && chosen == ring1);
    /* A potion: it fits no slot */
    ring2 = gen_item(IT_POTION, actor_player);
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_B));
    player_command(EV(NGE_KEY_W));
    CHECK(starts(OVER_ROW(0), "              Arm Thyself"));
    CHECK(starts(LIST_ROW(0), " a) sword") && starts(LIST_ROW(1), " b) ring of health"));
    CHECK(starts(LIST_ROW(2), "     "));       /* not the potion */
    CHECK(item_slot[ring1] != SLOT_NONE && !player_turn);
    player_turn = 1;
    fresh();
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_W));
    CHECK(item_slot[sword] == SLOT_WEAPON && !player_turn);
    /* Take off: in slot order */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_B));
    player_command(EV(NGE_KEY_R));
    CHECK(starts(OVER_ROW(0), "                 Divest"));
    CHECK(starts(LIST_ROW(0), " a) sword (in hand)") && starts(LIST_ROW(1), " b) ring of health (left)"));
    CHECK(item_slot[ring1] == SLOT_NONE && item_slot[sword] == SLOT_WEAPON && !player_turn);
    /* Refused: why, on the status line; the screen stays, the turn kept */
    player_turn = 1;
    inv_capacity[actor_player] = (uint8_t)inv_count(actor_player);
    fresh();
    press(EV(NGE_KEY_A));
    press(EV(NGE_KEY_SPACE));
    player_command(EV(NGE_KEY_R));
    CHECK(RAN_AND_CLOSED() && item_slot[sword] == SLOT_WEAPON && player_turn);
    CHECK(starts(STATUS(), "You can't carry any more."));
    inv_capacity[actor_player] = 20;
    nge_event_flush();

    /* ---- use: only what does something when used; a letter uses it up ---- */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_U));
    CHECK(starts(OVER_ROW(0), "                Partake"));          /* (40 - 7) / 2 */
    CHECK(starts(LIST_ROW(0), " a) healing potion") && starts(LIST_ROW(1), "     "));   /* not the ring */
    CHECK(NGE_ENTITY_STATE(ring2) == NGE_ENTITY_DYING && !player_turn);
    nge_event_process(messages_on_event);
    CHECK(starts(row(27), "You drink the healing potion.") && said("You feel much better."));   /* the window not full yet */
    fresh();
    player_command(EV(NGE_KEY_U));
    CHECK(paused == 0 && said("You have nothing to use."));   /* not opened: no frame ran */
    /* Food: eaten; the poison's start text */
    ring2 = gen_item(IT_FOOD, actor_player);
    fresh();
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_U));
    nge_event_process(messages_on_event);
    CHECK(starts(row(29), "You eat the dead rat.") && said("You feel sick."));
    effect_cure(actor_player, STATUS_POISONED);
    nge_event_process(messages_on_event);
    CHECK(said("The poison wears off."));
    /* Healing says nothing itself: regeneration is silent (the effect's text speaks for a potion) */
    health_damage(actor_player, NGE_ENTITY_INVALID, 2, DMG_SLASH);
    health_heal(actor_player, 1);
    nge_event_process(messages_on_event);
    CHECK(said("You take 2 slashing damage."));

    /* ---- a choosing screen with nothing to choose: says so, isn't opened ---- */
    nge_entity_destroy(ring1);
    nge_entity_destroy(ring2);                                     /* the food */
    item_slot[sword] = SLOT_NONE;
    nge_entity_destroy(sword);
    nge_render_set_view(AREA);
    fresh();
    CHECK(screen_inv_run(ACT_DROP, &chosen) == SCREEN_INV_BACK);
    CHECK(paused == 0 && test_layer2_buffer == SCREEN_BUFFER_MAIN);
    CHECK(said("You have nothing to drop."));
    CHECK(screen_inv_run(ACT_UNEQUIP, &chosen) == SCREEN_INV_BACK && said("You aren't using anything."));

    /* ---- a list longer than its panel (27 rows) scrolls with the cursor ---- */
    inv_capacity[actor_player] = 30;
    for (x = 0; x < 29; x++) {
        many[x] = gen_item((x == 28) ? IT_POTION : IT_RING, actor_player);
    }
    fresh();
    for (x = 0; x < 27; x++) {
        press(EV(NGE_KEY_DOWN));
    }
    press(EV(NGE_KEY_SPACE));
    screen_inv_run(SCREEN_INV_BROWSE, &chosen);
    CHECK(RAN_AND_CLOSED());
    CHECK(HIGHLIGHTED(26) && starts(LIST_ROW(26), " ring of health"));   /* item 27 on the last row */
    CHECK(starts(DETAILS(26), "Pack 29/30"));
    fresh();
    for (x = 0; x < 28; x++) {
        press(EV(NGE_KEY_DOWN));
    }
    press(EV(NGE_KEY_ENTER));                   /* the potion, the last, shown on the last row */
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen) == ACT_USE && chosen == many[28]);
    CHECK(HIGHLIGHTED(26) && starts(LIST_ROW(26), " healing potion"));
    for (x = 0; x < 29; x++) {
        nge_entity_destroy(many[x]);
    }
    inv_capacity[actor_player] = 20;

    /* ---- the title: any key, then the name typed, ENTER; a greeting on the main screen ---- */
    fresh();
    press(EV(NGE_KEY_A));
    press(EV(NGE_KEY_B));
    press(EV(NGE_KEY_O));
    press(EV(NGE_KEY_ENTER));
    screen_title_run();
    CHECK(RAN_AND_CLOSED());
    CHECK(strcmp(OVER_ROW(11), "               NGE ROGUE                ") == 0);
    CHECK(starts(seen_cols(1, 29, 0, 39), " What is your name? bo"));
    CHECK(starts(HELP(), "Type a name, then ENTER"));
    CHECK(strcmp(screen_name, "bo") == 0);
    CHECK(said("i inv, d drop, f fire, x look, Q quit."));

    /* ---- doors: bump to open, c and a direction to close ---- */
    fresh();
    door = gen_feature(FT_DOOR, AREA);
    feature_place(door, 6, 5);                                     /* east of the player, closed */
    nge_event_flush();
    player_turn = 1;
    player_command(EV(NGE_KEY_L));                                 /* 'l': east, into the door */
    CHECK(FEATURE_IS_OPEN(door) && NGE_LOCATION_X(actor_player) == 5 && !player_turn);
    nge_event_process(messages_on_event);
    CHECK(said("You open the door."));
    /* Walking into the open doorway and back: the map shows it, no "You see a door here." */
    player_command(EV(NGE_KEY_L));
    CHECK(NGE_LOCATION_X(actor_player) == 6);
    nge_event_process(messages_on_event);
    CHECK(said("You open the door."));
    player_command(EV(NGE_KEY_H));
    nge_event_flush();
    player_turn = 1;
    press(EV(NGE_KEY_L));
    player_command(EV(NGE_KEY_C));
    CHECK(said("Close in which direction?") && starts(seen_cols(0, 31, 0, 39), "Direction: close   Other keys: back"));
    CHECK(!FEATURE_IS_OPEN(door) && !player_turn);
    nge_event_process(messages_on_event);
    CHECK(said("You close the door."));
    /* Refused: told why, the turn kept */
    player_turn = 1;
    press(EV(NGE_KEY_L));
    player_command(EV(NGE_KEY_C));
    CHECK(said("The door is already closed.") && player_turn);
    press(EV(NGE_KEY_H));                                          /* west: nothing there */
    player_command(EV(NGE_KEY_C));
    CHECK(said("There is nothing there.") && player_turn);
    press(EV(NGE_KEY_A));                                          /* not a direction */
    player_command(EV(NGE_KEY_C));
    CHECK(said("Never mind.") && player_turn);
    press(EV(NGE_KEY_L));
    player_command(EV(NGE_KEY_F));
    CHECK(said("You have nothing to fire.") && player_turn);
    {
        int8_t dx;
        int8_t dy;

        CHECK(player_direction_key(EV(NGE_KEY_K), &dx, &dy) && dx == 0 && dy == -1);
        CHECK(player_direction_key(EV(NGE_KEY_RIGHT), &dx, &dy) && dx == 1 && dy == 0);
        CHECK(!player_direction_key(EV(NGE_KEY_ENTER), &dx, &dy) && said("Never mind."));
    }

    /* ---- stairs: > and < (SYMBOL SHIFT T and R), refused off stairs; arriving and leaving said ---- */
    nge_event_flush();
    player_turn = 1;
    player_command(NGE_KEY_T | (NGE_MOD_SYM << 8));
    CHECK(said("There are no stairs that way here.") && player_turn && !level_pending());
    player_command(NGE_KEY_R | (NGE_MOD_SYM << 8));
    CHECK(said("There are no stairs that way here.") && player_turn);
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, 2, LEVEL_ARRIVE_UP_STAIRS);
    nge_event_process(messages_on_event);
    CHECK(said("You go down to level 2."));
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, 1, LEVEL_ARRIVE_DOWN_STAIRS);
    nge_event_process(messages_on_event);
    CHECK(said("You go up to level 1."));
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, 3, LEVEL_ARRIVE_ANY);
    nge_event_process(messages_on_event);
    CHECK(said("You find yourself on level 3."));
    nge_event_post(door, EV_LEFT_LEVEL, NGE_EVENT_NONE, 1, 0);       /* anything named: the door will do */
    nge_event_process(messages_on_event);
    CHECK(said("The door goes down and away."));

    /* ---- the player's turn: their timer starts it, a new game resets it ---- */
    player_init();
    CHECK(!player_turn);
    player_on_turn(actor_player);
    CHECK(player_turn);
    player_init();
    CHECK(!player_turn);

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(screen_inv_run(ACT_COUNT + 1, &chosen));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(screen_inv_run(ACT_MOVE, &chosen));   /* not an item action */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(screen_close());                      /* none open */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(player_ask_direction(PLAYER_PROMPT_COUNT, NULL, NULL));
    CHECK(test_halt_count == 1);

    return test_summary("ui");
}
