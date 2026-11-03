/*
 * game/ui: the screens (the inventory, the title, the player character) on the fake Layer 2's two
 * buffers, and the player's commands that use them.
 */
#include "test.h"

#include <stdio.h>
#include <string.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "input/nge_input.h"
#include "input/nge_input_internal.h"
#include "io/nge_text.h"
#include "layer2/nge_layer2.h"
#include "map/nge_map.h"

#include "data/tile_data.h"

#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/feature.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/game.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/events.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/quest/quest.h"
#include "game/spells/spells.h"
#include "game/ui/screen.h"
#include "game/ui/screen_inv.h"
#include "game/ui/screen_main.h"
#include "game/ui/screen_pc.h"
#include "game/ui/screen_title.h"
#include "game/ui/screen_internal.h"

#define AREA     NGE_OWNER_AREA(0)
#define EV(k)    ((nge_key_event_t)(k))

/* What game.c and actors.c provide in the game (not linked here) */
nge_entity_id_t actor_player;
static int waits;     /* game_wait_key() calls: a prompt waited for a key */
static int paused;    /* game_pause_frame() calls: frames a screen ran */

static uint8_t over;   /* game_is_over()'s answer: a test sets it */

void game_over(uint8_t reason)
{
    (void)reason;
}

uint8_t game_is_over(void)
{
    return over;
}

/* The game's dispatch, as much of it as these tests link: the player, the quest, the messages */
static void pause_on_event(const nge_event_t *e)
{
    player_on_event(e);
    quest_on_event(e);
    messages_on_event(e);
}

void game_pause_events(void)
{
    nge_event_process(pause_on_event);
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

/* The overlay screen's row y, and its list panel's row i, whole, as last seen */
#define OVER_ROW(y)  seen_cols(1, (y), 0, 39)
#define LIST_ROW(i)  seen_cols(1, (uint8_t)(2 + (i)), 0, 39)
#define STATUS()     seen_cols(1, 29, 0, 39)
#define STATUS2()    seen_cols(1, 30, 0, 39)
#define HELP()       seen_cols(1, 31, 0, 39)
#define HIGHLIGHTED(i)  (seen[1][2 + (i)][0].paper == SCREEN_HIGHLIGHT && seen[1][2 + (i)][39].paper == SCREEN_HIGHLIGHT)
#define BLANK        "                                        "

/* An inventory row as it should be: the letter, the count, the name and the details in their columns */
static const char *inv_row(const char *letter, unsigned count, const char *name, const char *details)
{
    static char expect[64];

    snprintf(expect, sizeof(expect), "%-2s%3u %-19s %-14s", letter, count, name, details);
    return expect;
}

/* The text colours as the Layer 2 stream draws them (io/nge_l2_stream.c) */
#define L2_WHITE   0xFF
#define L2_YELLOW  0xFC
#define L2_CYAN    0x1F
#define L2_RED     0xE0

/* The ink of the overlay screen's cell at row y, column x, as last seen */
#define INK(y, x)  (seen[1][(y)][(x)].ink)

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
/* A message in the window (rows 26-30), not only the last */
static int shown(const char *s)
{
    int y;

    for (y = 26; y <= 30; y++) {
        if (starts(row((uint8_t)y), s)) {
            return 1;
        }
    }
    return 0;
}

/* The quest module, then the messages: the game's order (game_on_event()) */
static void quest_and_messages(const nge_event_t *e)
{
    quest_on_event(e);
    messages_on_event(e);
}

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
           !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION);
}

int main(void)
{
    nge_entity_id_t sword;
    nge_entity_id_t ring1;
    nge_entity_id_t ring2;
    nge_entity_id_t door;
    nge_entity_id_t quest;
    nge_entity_id_t chosen;
    nge_entity_id_t potion;
    uint8_t used;
    nge_entity_id_t many[29];
    nge_entity_id_t rat;
    char name[PC_NAME_MAX + 1] = "Rogue";
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

    /* ---- the stats window: the player's stats, from the kind record ---- */
    screen_draw_stats();
    player_show_stats();
    test_layer2_row(0, 28, 39, text);
    CHECK(strcmp(text, "Name: Rogue ") == 0);
    test_layer2_row(1, 28, 39, text);
    CHECK(strcmp(text, "Level     1 ") == 0);
    test_layer2_row(2, 28, 39, text);
    CHECK(strcmp(text, "HP   12/ 12 ") == 0);
    test_layer2_row(3, 28, 39, text);
    CHECK(strcmp(text, "MP    5/  5 ") == 0);
    test_layer2_row(4, 28, 39, text);
    CHECK(strcmp(text, "AC       10 ") == 0);
    test_layer2_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      16 ") == 0);
    test_layer2_row(10, 28, 39, text);
    CHECK(strcmp(text, "Cha      10 ") == 0);
    test_layer2_row(11, 28, 39, text);
    CHECK(strcmp(text, "            ") == 0);
    test_layer2_row(12, 28, 39, text);
    CHECK(strcmp(text, "XP        0 ") == 0);
    test_layer2_row(13, 28, 39, text);
    CHECK(strcmp(text, "Gold      0 ") == 0);
    effect_apply(EF_STRENGTH, 1, actor_player, NGE_ENTITY_INVALID);
    player_show_stats();
    test_layer2_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      17 ") == 0);
    nge_event_flush();
    /* MP follow their events */
    mana_spend(actor_player, 2);
    nge_event_process(player_on_event);
    test_layer2_row(3, 28, 39, text);
    CHECK(strcmp(text, "MP    3/  5 ") == 0);
    mana_restore(actor_player, 9);
    nge_event_process(player_on_event);
    test_layer2_row(3, 28, 39, text);
    CHECK(strcmp(text, "MP    5/  5 ") == 0);
    /* XP, level and gold follow their events; gold gives XP */
    pc_gain_gold(actor_player, 12);
    nge_event_process(player_on_event);
    test_layer2_row(12, 28, 39, text);
    CHECK(strcmp(text, "XP       12 ") == 0);
    test_layer2_row(13, 28, 39, text);
    CHECK(strcmp(text, "Gold     12 ") == 0);
    pc_gain_xp(actor_player, 8);                                   /* 20: level 2 */
    nge_event_process(player_on_event);
    test_layer2_row(1, 28, 39, text);
    CHECK(strcmp(text, "Level     2 ") == 0);
    test_layer2_row(12, 28, 39, text);
    CHECK(strcmp(text, "XP       20 ") == 0);
    /* A monster the player kills gives its XP; one killed by anything else, none */
    rat = gen_creature(CR_RAT, AREA);
    nge_event_flush();
    health_damage(rat, actor_player, 99, DMG_SLASH);
    nge_event_process(player_on_event);
    CHECK(pc_xp == 25);
    test_layer2_row(12, 28, 39, text);
    CHECK(strcmp(text, "XP       25 ") == 0);
    rat = gen_creature(CR_RAT, AREA);
    nge_event_flush();
    health_damage(rat, NGE_ENTITY_INVALID, 99, DMG_POISON);        /* immune: poison doesn't kill it */
    health_damage(rat, NGE_ENTITY_INVALID, 99, DMG_FIRE);          /* a trap */
    nge_event_process(player_on_event);
    CHECK(pc_xp == 25);
    /* An arrival: the deepest depth */
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, 4, LEVEL_ARRIVE_ANY);
    nge_event_post(actor_player, EV_ARRIVED, NGE_EVENT_NONE, 3, LEVEL_ARRIVE_ANY);
    nge_event_process(player_on_event);
    CHECK(pc_depth_max == 4);

    /* ---- the inventory, empty: titled, the pack's count, says so, SPACE goes back; the main screen untouched ---- */
    memcpy(main_before, test_layer2_buf[0], sizeof(main_before));
    fresh();
    press(EV(NGE_KEY_A));                      /* aim, with nothing to aim: it stays */
    press(EV(NGE_KEY_SPACE));
    player_turn = 1;
    player_command(EV(NGE_KEY_I));
    CHECK(SCREEN_INV_MAX == 20 + SLOT_COUNT - 1);   /* a full pack and every slot in use */
    CHECK(RAN_AND_CLOSED() && paused == 3);   /* a frame a key, and the frame that shows the main screen */
    CHECK(strcmp(OVER_ROW(0), "          Worldly Possessions Pack  0/20") == 0);   /* centred; the count at the end */
    CHECK(starts(LIST_ROW(0), " You are carrying nothing."));
    CHECK(starts(HELP(), "jk:select SPACE:back"));
    CHECK(seen[1][0][0].paper == SCREEN_PAPER);                     /* the whole display covered */
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);
    CHECK(player_turn && nge_event_count() == 0);
    CHECK(nge_render_area == AREA);                                 /* the sprites aren't hidden: buffer 1 covers them */

    /* ---- browse: count, name and details in columns; in use first (by slot), in its ink; the cursor highlighted ---- */
    sword = gen_item(IT_SWORD, actor_player);
    ring1 = gen_item(IT_RING_HEALTH_2, actor_player);
    ring2 = gen_item(IT_RING_HEALTH_2, actor_player);
    item_slot[ring1] = SLOT_RING_L;
    inv_capacity[actor_player] = 20;
    fresh();
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && !used && RAN_AND_CLOSED());
    CHECK(strcmp(LIST_ROW(0), inv_row("", 1, "ring of health +2", "")) == 0);
    CHECK(strcmp(LIST_ROW(1), inv_row("", 1, "sword", "1d8 [+1,+0]")) == 0);
    CHECK(strcmp(LIST_ROW(2), inv_row("", 1, "ring of health +2", "")) == 0);
    CHECK(strcmp(LIST_ROW(3), BLANK) == 0);
    CHECK(INK(2, 6) == L2_CYAN && INK(3, 6) == L2_WHITE && INK(3, 26) == L2_WHITE);   /* in use; stowed */
    CHECK(HIGHLIGHTED(0) && !HIGHLIGHTED(1));
    CHECK(strcmp(OVER_ROW(0) + 30, "Pack  2/20") == 0);             /* the ring in use doesn't count */
    CHECK(starts(HELP(), "jk:select r:off SPACE:back"));            /* in use: only taking it off */
    /* The cursor: down (the cursor key or j), the highlight and the help follow */
    fresh();
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_J));
    press(EV(NGE_KEY_K));
    press(EV(NGE_KEY_SPACE));
    screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
    CHECK(RAN_AND_CLOSED() && HIGHLIGHTED(1) && !HIGHLIGHTED(0) && !HIGHLIGHTED(2));
    CHECK(starts(HELP(), "jk:select w:wear d:drop SPACE:back"));
    /* Refused: why, on the status line; the screen stays, nothing done */
    fresh();
    press(EV(NGE_KEY_D));                       /* the ring in use */
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && !used && RAN_AND_CLOSED());
    CHECK(starts(STATUS(), "You must take that off first."));
    CHECK(item_slot[ring1] == SLOT_RING_L && nge_event_count() == 0);
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);   /* not in the message window */
    /* Moving on clears it */
    fresh();
    press(EV(NGE_KEY_U));                       /* a ring does nothing when used */
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_SPACE));
    screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
    CHECK(strcmp(STATUS(), BLANK) == 0);

    /* ---- browse: wear, take off, drop and use are done here, the screen staying open; one turn for them all ---- */
    potion = gen_item(IT_HEAL_POTION, actor_player);   /* the list: ring1 (in use), sword, ring2, potion */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_DOWN));                    /* the sword */
    press(EV(NGE_KEY_W));                       /* wielded: first now, the cursor with it */
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_DOWN));                    /* ring2 */
    press(EV(NGE_KEY_W));                       /* the right hand: sword, ring1, ring2, potion */
    press(EV(NGE_KEY_DOWN));                    /* the potion */
    press(EV(NGE_KEY_U));                       /* drunk: gone, the cursor on the last, ring2 */
    press(EV(NGE_KEY_D));                       /* in use: refused, nothing done */
    press(EV(NGE_KEY_R));                       /* taken off: to the pack, still last */
    press(EV(NGE_KEY_D));                       /* dropped: the cursor on the new last, ring1 */
    press(EV(NGE_KEY_SPACE));
    player_command(EV(NGE_KEY_I));
    CHECK(RAN_AND_CLOSED());
    CHECK(item_slot[sword] == SLOT_WEAPON && item_slot[ring1] == SLOT_RING_L);
    CHECK(NGE_OWNER_OF(ring2) == AREA && NGE_LOCATION_X(ring2) == 5);
    CHECK(NGE_ENTITY_STATE(potion) == NGE_ENTITY_DYING);
    CHECK(!player_turn && nge_event_count() == 0);                  /* one turn; every event handled in the screen */
    CHECK(strcmp(LIST_ROW(0), inv_row("", 1, "sword", "1d8 [+1,+0]")) == 0 && INK(2, 6) == L2_CYAN);
    CHECK(strcmp(LIST_ROW(1), inv_row("", 1, "ring of health +2", "")) == 0 && HIGHLIGHTED(1));
    CHECK(strcmp(LIST_ROW(2), BLANK) == 0);
    CHECK(strcmp(OVER_ROW(0) + 30, "Pack  0/20") == 0);
    CHECK(starts(STATUS(), "You drop a ring of health +2.") && strcmp(STATUS2(), BLANK) == 0);
    CHECK(starts(HELP(), "jk:select r:off SPACE:done"));            /* something done: SPACE leaves with it */
    CHECK(said("You drop a ring of health +2.") && shown("You take off a ring of health +2."));   /* the window too */
    /* A use's two messages: one a line on the status line */
    potion = gen_item(IT_HEAL_POTION, actor_player);
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_ENTER));                   /* the obvious action: use it */
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used && RAN_AND_CLOSED());
    CHECK(starts(STATUS(), "You drink the healing potion.") && starts(STATUS2(), "You feel much better."));
    CHECK(NGE_ENTITY_STATE(potion) == NGE_ENTITY_DYING && player_turn);   /* the command, not the screen, ends the turn */
    /* ENTER on what is in use takes it off; the screen stays */
    fresh();
    press(EV(NGE_KEY_ENTER));
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used && RAN_AND_CLOSED());
    CHECK(stowed(sword) && item_slot[ring1] == SLOT_RING_L);
    CHECK(starts(STATUS(), "You take off a sword."));
    /* The game over while the screen is up (R3a): it closes after the action */
    potion = gen_item(IT_HEAL_POTION, actor_player);
    over = 1;
    fresh();
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_DOWN));
    press(EV(NGE_KEY_U));                       /* no SPACE: it closes by itself */
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used && RAN_AND_CLOSED());
    CHECK(NGE_ENTITY_STATE(potion) == NGE_ENTITY_DYING);
    over = 0;
    /* Back as the choosing tests expect: all three in the pack */
    item_slot[ring1] = SLOT_NONE;
    inv_stow(ring2, actor_player);
    effects_recompute(actor_player);
    nge_event_flush();

    /* ---- choose: d, w, r and u list what the action applies to, lettered; a letter chooses ---- */
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_E));                       /* past the list: nothing */
    press(EV(NGE_KEY_C));
    player_command(EV(NGE_KEY_D));
    CHECK(RAN_AND_CLOSED());
    CHECK(starts(OVER_ROW(0), "             Shed a Burden"));         /* (40 - 13) / 2 */
    CHECK(strcmp(LIST_ROW(0), inv_row("a)", 1, "sword", "1d8 [+1,+0]")) == 0);
    CHECK(starts(LIST_ROW(1), "b)  1 ring of health"));
    CHECK(starts(LIST_ROW(2), "c)  1 ring of health") && strcmp(LIST_ROW(3), BLANK) == 0);
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
    CHECK(screen_inv_run(ACT_DROP, &chosen, &used) == ACT_DROP && chosen == ring1 && !used);
    /* A potion: it fits no slot */
    ring2 = gen_item(IT_HEAL_POTION, actor_player);
    fresh();
    player_turn = 1;
    press(EV(NGE_KEY_B));
    player_command(EV(NGE_KEY_W));
    CHECK(starts(OVER_ROW(0), "              Arm Thyself"));
    CHECK(starts(LIST_ROW(0), "a)  1 sword") && starts(LIST_ROW(1), "b)  1 ring of health"));
    CHECK(strcmp(LIST_ROW(2), BLANK) == 0);     /* not the potion */
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
    CHECK(starts(LIST_ROW(0), "a)  1 sword") && starts(LIST_ROW(1), "b)  1 ring of health +2"));
    CHECK(INK(2, 6) == L2_CYAN && INK(3, 6) == L2_CYAN);                 /* both in use */
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
    CHECK(starts(LIST_ROW(0), "a)  1 healing potion") && strcmp(LIST_ROW(1), BLANK) == 0);   /* not the ring */
    CHECK(NGE_ENTITY_STATE(ring2) == NGE_ENTITY_DYING && !player_turn);
    nge_event_process(messages_on_event);
    CHECK(starts(row(27), "You drink the healing potion.") && said("You feel much better."));   /* the window not full yet */
    fresh();
    player_command(EV(NGE_KEY_U));
    CHECK(paused == 0 && said("You have nothing to use."));   /* not opened: no frame ran */
    /* Food: eaten; the poison's start text */
    ring2 = gen_item(IT_DEAD_RAT, actor_player);
    fresh();
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_U));
    nge_event_process(messages_on_event);
    CHECK(starts(row(29), "You eat the dead rat.") && said("You feel sick."));
    effect_cure(actor_player, STATUS_POISONED);
    nge_event_process(messages_on_event);
    CHECK(said("The poison wears off."));
    /* A parchment is read; "an" ahead of a vowel */
    ring2 = gen_item(IT_SP_LIGHT, actor_player);
    fresh();
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_U));
    nge_event_process(messages_on_event);
    CHECK(shown("You read the light parchment.") && said("The room lights up."));
    ring2 = gen_item(IT_APPLE, AREA);
    nge_location_set(ring2, NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));
    nge_event_post(ring2, EV_DROPPED, actor_player, 0, 0);
    nge_event_process(messages_on_event);
    CHECK(said("You drop an apple."));
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
    CHECK(screen_inv_run(ACT_DROP, &chosen, &used) == SCREEN_INV_BACK && !used);
    CHECK(paused == 0 && test_layer2_buffer == SCREEN_BUFFER_MAIN);
    CHECK(said("You have nothing to drop."));
    CHECK(screen_inv_run(ACT_UNEQUIP, &chosen, &used) == SCREEN_INV_BACK && said("You aren't using anything."));

    /* ---- the details: an attack's dice and bonuses, ammo's bonuses, AC, a count's meaning; stacks counted ---- */
    {
        static const struct { uint8_t kind; unsigned count; const char *name; const char *details; } rows[] = {
            { IT_RUSTY_SWORD,    1, "rusty sword",        "1d8 [-1,-1]" },
            { IT_LONG_BOW_2,     1, "long bow +2",        "1d8 [+3,+2]" },
            { IT_ARROWS,        15, "sheaf of arrows",    "[+0,+0]" },       /* no attack part */
            { IT_ARROWS_1,      10, "sheaf of arrows +1", "[+1,+1]" },       /* bonuses only */
            { IT_DARTS,          8, "handful of darts",   "1d3 [+0,+0]" },   /* thrown: dice of its own */
            { IT_HELMET,         1, "iron helmet",        "AC 2" },
            { IT_WAND_STRIKING,  1, "wand of striking",   "6 charges" },
            { IT_TORCH,          1, "torch",              "100 turns" },
            { IT_LOCK_PICK,      1, "lock pick",          "5 uses" },
            { IT_GOLD_SMALL,    10, "handful of gold",    "" },
            { IT_HEAL_POTION,    1, "healing potion",     "" },
        };
        nge_entity_id_t ids[sizeof(rows) / sizeof(rows[0])];
        uint8_t i;

        for (i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
            ids[i] = gen_item(rows[i].kind, actor_player);
        }
        fresh();
        press(EV(NGE_KEY_SPACE));
        screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
        for (i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
            CHECK(strcmp(LIST_ROW(i), inv_row("", rows[i].count, rows[i].name, rows[i].details)) == 0);
        }
        /* One charge; a bonus of two digits fits its column */
        item_count[ids[6]] = 1;
        fresh();
        press(EV(NGE_KEY_SPACE));
        screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
        CHECK(strcmp(LIST_ROW(6), inv_row("", 1, "wand of striking", "1 charge")) == 0);
        item_count[ids[6]] = 6;
        /* Wield the rusty sword, then aim the wand: the aim is returned, the wield's turn used */
        fresh();
        press(EV(NGE_KEY_W));
        for (i = 0; i < 6; i++) {
            press(EV(NGE_KEY_DOWN));
        }
        press(EV(NGE_KEY_A));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == ACT_AIM && chosen == ids[6] && used);
        CHECK(item_slot[ids[0]] == SLOT_WEAPON && INK(2, 6) == L2_CYAN);
        nge_event_flush();
        item_slot[ids[0]] = SLOT_NONE;
        for (i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
            nge_entity_destroy(ids[i]);
        }
        effects_recompute(actor_player);
        nge_event_flush();
    }

    /* ---- a list longer than its panel (27 rows) scrolls with the cursor ---- */
    inv_capacity[actor_player] = 30;
    for (x = 0; x < 29; x++) {
        many[x] = gen_item((x == 28) ? IT_HEAL_POTION : IT_RING_HEALTH_2, actor_player);
    }
    fresh();
    for (x = 0; x < 27; x++) {
        press(EV(NGE_KEY_DOWN));
    }
    press(EV(NGE_KEY_SPACE));
    screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
    CHECK(RAN_AND_CLOSED());
    CHECK(HIGHLIGHTED(26) && starts(LIST_ROW(26), "    1 ring of health"));   /* item 27 on the last row */
    CHECK(strcmp(OVER_ROW(0) + 30, "Pack 29/30") == 0);
    fresh();
    for (x = 0; x < 28; x++) {
        press(EV(NGE_KEY_DOWN));
    }
    press(EV(NGE_KEY_ENTER));                   /* the potion, the last, shown on the last row: drunk */
    press(EV(NGE_KEY_SPACE));
    CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used);
    CHECK(NGE_ENTITY_STATE(many[28]) == NGE_ENTITY_DYING);
    CHECK(HIGHLIGHTED(26) && starts(LIST_ROW(26), "    1 ring of health"));   /* the new last, still shown */
    nge_event_flush();
    for (x = 0; x < 28; x++) {
        nge_entity_destroy(many[x]);
    }
    inv_capacity[actor_player] = 20;

    /* ---- the title: any key, then the name typed, ENTER; a greeting on the main screen ---- */
    fresh();
    press(EV(NGE_KEY_A));
    press(EV(NGE_KEY_B));
    press(EV(NGE_KEY_O));
    press(EV(NGE_KEY_ENTER));
    screen_title_run(name);
    CHECK(RAN_AND_CLOSED());
    CHECK(strcmp(OVER_ROW(11), "               NGE ROGUE                ") == 0);
    CHECK(starts(seen_cols(1, 29, 0, 39), " What is your name? bo"));
    CHECK(starts(HELP(), "Type a name, then ENTER"));
    CHECK(strcmp(name, "bo") == 0);
    CHECK(said("i inv, d drop, f fire, x look, Q quit."));
    /* A new experience level's message */
    nge_event_post(actor_player, EV_LEVEL_GAINED, NGE_EVENT_NONE, 2, 0);
    nge_event_process(messages_on_event);
    CHECK(said("Welcome to experience level 2."));

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

    /* ---- locks: bumped without a key it says so; with one it unlocks, and stays shut; p chooses a key or a pick ---- */
    {
        nge_entity_id_t key;

        player_command(EV(NGE_KEY_P));                             /* nothing to choose: no screen */
        CHECK(said("You have no key or pick.") && player_turn);
        feature_lock(door, KEY_IRON);
        player_command(EV(NGE_KEY_L));
        CHECK(said("The door is locked.") && player_turn && FEATURE_IS_LOCKED(door) && NGE_LOCATION_X(actor_player) == 5);
        key = gen_item(IT_KEY_BRASS, actor_player);
        player_command(EV(NGE_KEY_L));                             /* it doesn't fit: not tried */
        CHECK(said("The door is locked.") && player_turn && FEATURE_IS_LOCKED(door));
        nge_entity_destroy(key);
        key = gen_item(IT_KEY_IRON, actor_player);
        player_command(EV(NGE_KEY_L));
        CHECK(!FEATURE_IS_LOCKED(door) && !FEATURE_IS_OPEN(door) && !player_turn);
        nge_event_process(messages_on_event);
        CHECK(said("You unlock the door."));
        /* p, a letter, a direction */
        player_turn = 1;
        feature_lock(door, KEY_IRON);
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the screen's closing frame */
        press(EV(NGE_KEY_L));
        player_command(EV(NGE_KEY_P));
        CHECK(said("Unlock in which direction?") && !FEATURE_IS_LOCKED(door) && !player_turn);
        nge_event_process(messages_on_event);
        /* On what isn't locked: told, the turn kept */
        player_turn = 1;
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the screen's closing frame */
        press(EV(NGE_KEY_L));
        player_command(EV(NGE_KEY_P));
        CHECK(said("The door isn't locked.") && player_turn);
        nge_entity_destroy(key);
        level_release_dying();
        /* L, the dev kit's: the nearest closed door is locked, and no turn used */
        player_command(NGE_KEY_L | (NGE_MOD_CAPS << 8));
        CHECK(FEATURE_IS_LOCKED(door) && player_turn);
        feature_unlock(door);
    }
    player_command(EV(NGE_KEY_F));                                 /* said at once, with no cursor */
    CHECK(said("You have nothing to attack with.") && player_turn);
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

    /* ---- the quest item: found once, needed to leave, the win (docs/winning.md) ---- */
    quest = gen_item(IT_QUEST, actor_player);
    nge_entity_set_flags(quest, NGE_ENTITY_PERSISTENT);
    quest_set(quest);
    screen_message("-");
    nge_event_post(quest, EV_PICKED_UP, door, 0, 0);           /* not the player: nothing */
    nge_event_process(quest_and_messages);
    CHECK(said("-"));
    nge_event_post(quest, EV_PICKED_UP, actor_player, 0, 0);
    nge_event_process(quest_and_messages);
    CHECK(said("Now climb back out of the dungeon.") && shown("You pick up a crown."));
    nge_event_post(quest, EV_PICKED_UP, actor_player, 0, 0);   /* again: only the pick-up */
    nge_event_process(quest_and_messages);
    CHECK(said("You pick up a crown."));
    {
        act_t a = { 0 };

        a.actor = actor_player;
        a.reason = ACT_R_NO_QUEST;
        messages_act_failed(&a);
        CHECK(said("You can't leave without the crown."));
        a.reason = ACT_R_NO_LINE;
        messages_act_failed(&a);
        CHECK(said("Something is in the way."));
        a.reason = ACT_R_NO_AMMO;
        messages_act_failed(&a);
        CHECK(said("You have no ammunition ready."));
        a.reason = ACT_R_WRONG_AMMO;
        messages_act_failed(&a);
        CHECK(said("That ammunition doesn't fit."));
        a.reason = ACT_R_CANT_AIM;
        messages_act_failed(&a);
        CHECK(said("You can't aim that."));
        a.reason = ACT_R_NO_CHARGES;
        messages_act_failed(&a);
        CHECK(said("It has no charges left."));
        a.reason = ACT_R_CURSED;
        messages_act_failed(&a);
        CHECK(said("It is cursed: it won't come off."));
        a.reason = ACT_R_WRONG_KEY;
        messages_act_failed(&a);
        CHECK(said("That doesn't fit the lock."));
        a.reason = ACT_R_PICK_FAILED;
        messages_act_failed(&a);
        CHECK(said("The lock holds."));
        a.reason = ACT_R_NO_MANA;
        messages_act_failed(&a);
        CHECK(said("You haven't the mana."));
        a.reason = ACT_R_NO_SPELL;
        messages_act_failed(&a);
        CHECK(said("You don't know that spell."));
    }
    messages_game_over(GAME_OVER_WON);
    CHECK(said("Press any key.") && shown("You escape with the crown. You win!"));
    messages_game_over(GAME_OVER_DIED);
    CHECK(said("Game over. Press any key."));

    /* ---- aim: an item from its list, then a tile; the player's own, here ---- */
    {
        nge_entity_id_t staff;
        uint8_t charges;
        int rows_to_staff;
        int i;

        fresh();
        player_turn = 1;
        player_command(EV(NGE_KEY_A));                             /* nothing listed: no screen */
        CHECK(said("You have nothing to aim.") && player_turn);
        staff = gen_item(IT_STAFF_HEALING, actor_player);
        charges = item_count[staff];
        press(EV(NGE_KEY_A));                                      /* the list's first */
        press(EV(NGE_KEY_SPACE));                                  /* taken by the screen's closing frame */
        press(EV(NGE_KEY_Q));                                      /* the cursor cancelled: no turn, no charge */
        player_command(EV(NGE_KEY_A));
        CHECK(player_turn && item_count[staff] == charges);
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_ENTER));                                  /* the cursor starts on the player */
        player_command(EV(NGE_KEY_A));
        CHECK(!player_turn && item_count[staff] == charges - 1);
        nge_event_process(messages_on_event);
        player_turn = 1;
        /* The inventory's a key, and its ENTER: what can be aimed is aimed */
        rows_to_staff = 0;
        for (i = 0; i < NGE_ENTITY_MAX; i++) {                     /* in use first, then the pack by ID */
            if (NGE_ENTITY_IS_ALIVE(i) && NGE_OWNER_OF(i) == actor_player &&
                (item_slot[i] != SLOT_NONE || i < staff)) {
                rows_to_staff++;
            }
        }
        for (i = 0; i < rows_to_staff; i++) {
            press(EV(NGE_KEY_DOWN));
        }
        press(EV(NGE_KEY_ENTER));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_ENTER));
        player_command(EV(NGE_KEY_I));
        CHECK(!player_turn && item_count[staff] == charges - 2);
        nge_event_flush();
        player_turn = 1;
        for (i = 0; i < rows_to_staff; i++) {
            press(EV(NGE_KEY_DOWN));
        }
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_ENTER));
        player_command(EV(NGE_KEY_I));
        CHECK(!player_turn && item_count[staff] == charges - 3);
        nge_event_flush();
        player_turn = 1;
        /* A wand with no charges is listed, and refused with the screen open */
        item_count[staff] = 0;
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_A));
        CHECK(player_turn && item_count[staff] == 0);
        nge_entity_destroy(staff);
        nge_event_flush();
    }

    /* ---- cast: m lists the spells known, then a bolt asks for a tile; backing out uses no turn ---- */
    {
        nge_entity_id_t rat;
        uint8_t px = NGE_LOCATION_X(actor_player);
        uint8_t py = NGE_LOCATION_Y(actor_player);

        fresh();
        nge_event_flush();
        mana_restore(actor_player, 255);
        nge_event_flush();
        player_turn = 1;
        CHECK(spell_knows(actor_player, SP_MAGIC_BOLT) && mana_mp[actor_player] == mana_mp_max[actor_player]);
        press(EV(NGE_KEY_SPACE));                                  /* back from the list */
        player_command(EV(NGE_KEY_M));
        CHECK(RAN_AND_CLOSED() && player_turn);
        CHECK(strstr(OVER_ROW(0), " Cast a Spell ") != NULL && strstr(OVER_ROW(2), "Spell") != NULL &&
              strstr(OVER_ROW(2), "Level") != NULL && strstr(OVER_ROW(2), "MP") != NULL);
        CHECK(starts(OVER_ROW(3), " a) Magic Bolt") && INK(3, 4) == L2_WHITE && starts(OVER_ROW(4), " b) Cure Wounds"));
        CHECK(starts(seen_cols(1, 3, 20, 28), "    1   1"));      /* its level and MP */
        CHECK(starts(OVER_ROW(26), " MP "));
        /* A bolt: the list's first, the closing frame, then the cursor cancelled */
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_Q));
        player_command(EV(NGE_KEY_M));
        CHECK(player_turn && mana_mp[actor_player] == mana_mp_max[actor_player]);
        /* At a rat west of the player (a door is east): cast, the turn used, an MP spent, said */
        rat = gen_creature(CR_RAT, AREA);
        nge_location_set(rat, (uint8_t)(px - 1), py);
        nge_event_flush();
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_LEFT));
        press(EV(NGE_KEY_ENTER));
        player_command(EV(NGE_KEY_M));
        CHECK(!player_turn && mana_mp[actor_player] == mana_mp_max[actor_player] - 1);
        nge_event_process(messages_on_event);
        CHECK(shown("You cast Magic Bolt."));
        player_turn = 1;
        /* A self spell (known from a level gained above): no tile asked for */
        spell_learn(actor_player, SP_HASTE);
        nge_event_process(messages_on_event);
        CHECK(said("You learn Haste."));
        fresh();
        press(EV(NGE_KEY_B));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_M));
        CHECK(!player_turn && mana_mp[actor_player] == mana_mp_max[actor_player] - 2);
        nge_event_flush();
        player_turn = 1;
        /* No MP: the spell is red, choosing it says why on the status line, and the screen stays open */
        mana_spend(actor_player, mana_mp[actor_player]);
        nge_event_flush();
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_M));
        CHECK(RAN_AND_CLOSED() && player_turn && INK(3, 4) == L2_RED && starts(STATUS(), "You haven't the mana."));
        /* A monster seen casting is said; one the player knows nothing of isn't learned aloud */
        nge_event_post(rat, EV_CAST, actor_player, SP_HOLD, 0);
        nge_event_process(messages_on_event);
        CHECK(said("You cast Hold."));
        nge_event_post(rat, EV_SPELL_LEARNED, NGE_EVENT_NONE, SP_HOLD, 0);
        nge_event_process(messages_on_event);
        CHECK(said("You cast Hold."));
        {
            nge_entity_id_t shaman = gen_creature(CR_GOBLIN_SHAMAN, AREA);

            nge_location_set(shaman, (uint8_t)(px - 2), py);
            nge_event_flush();
            nge_event_post(actor_player, EV_CAST, shaman, SP_MAGIC_BOLT, 0);
            nge_event_process(messages_on_event);
            CHECK(said("The goblin shaman casts Magic Bolt."));
            nge_entity_destroy(shaman);
        }
        /* Knowing none: said, and no screen */
        NGE_ENTITY_REMOVE_COMPONENT(actor_player, COMPONENT_SPELLS);
        fresh();
        player_command(EV(NGE_KEY_M));
        CHECK(said("You know no spells.") && player_turn && paused == 0);
        NGE_ENTITY_ADD_COMPONENT(actor_player, COMPONENT_SPELLS);
        nge_entity_destroy(rat);
        mana_restore(actor_player, 255);
        nge_event_flush();
    }

    /* ---- the player's turn: their timer starts it, a new game resets it ---- */
    player_init();
    CHECK(!player_turn);
    player_on_turn(actor_player);
    CHECK(player_turn);
    player_init();
    CHECK(!player_turn);

    /* ---- the player character screen ---- */
    CHECK(screen_pc_ink(10, 10, 0) == NGE_INK_WHITE && screen_pc_ink(11, 10, 0) == NGE_INK_CYAN &&
          screen_pc_ink(9, 10, 0) == NGE_INK_RED);
    CHECK(screen_pc_ink(10, 10, 1) == NGE_INK_WHITE && screen_pc_ink(5, 10, 1) == NGE_INK_CYAN &&
          screen_pc_ink(20, 10, 1) == NGE_INK_RED);                /* speed: lower is better */
    /* C (CAPS SHIFT): opens and closes it, no turn used, the main screen untouched */
    effects_recompute(actor_player);
    nge_event_flush();
    memcpy(main_before, test_layer2_buf[0], sizeof(main_before));
    fresh();
    press(EV(NGE_KEY_A));                                           /* not a key of the screen: it stays */
    press(EV(NGE_KEY_SPACE));
    player_turn = 1;
    player_command(NGE_KEY_C | (NGE_MOD_CAPS << 8));
    CHECK(RAN_AND_CLOSED() && paused == 3);
    CHECK(player_turn && nge_event_count() == 0);
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);
    CHECK(strstr(OVER_ROW(0), " Rogue the Fighter ") != NULL && INK(0, 20) == L2_YELLOW);
    CHECK(starts(HELP(), "SPACE:back"));
    CHECK(seen[1][0][0].paper == SCREEN_PAPER && seen[1][30][39].paper == SCREEN_PAPER);   /* rows 0-30 covered */
    {
        char want[48];
        uint8_t a;
        int ok = 1;

        /* The character, left; the abilities, right: at their bases, all white */
        snprintf(want, sizeof(want), "Level       %6u", pc_level);
        CHECK(strcmp(seen_cols(1, 2, 0, 17), want) == 0);
        snprintf(want, sizeof(want), "XP          %6u", pc_xp);
        CHECK(strcmp(seen_cols(1, 3, 0, 17), want) == 0);
        snprintf(want, sizeof(want), "Next        %6u", pc_xp_needed((uint8_t)(pc_level + 1)));
        CHECK(strcmp(seen_cols(1, 4, 0, 17), want) == 0);
        snprintf(want, sizeof(want), "Gold        %6u", pc_gold);
        CHECK(strcmp(seen_cols(1, 5, 0, 17), want) == 0);
        snprintf(want, sizeof(want), "Depth       %6u", level_depth);
        CHECK(strcmp(seen_cols(1, 6, 0, 17), want) == 0);
        CHECK(strcmp(seen_cols(1, 7, 0, 17), "Deepest          4") == 0);
        CHECK(strcmp(seen_cols(1, 2, 20, 39), "      Now  Mod Base ") == 0);
        for (a = 0; a < ABIL_COUNT; a++) {
            snprintf(want, sizeof(want), " %.3s %4u  %+3d %4u ", a == 0 ? "Str" : a == 1 ? "Dex" : a == 2 ? "Con" :
                     a == 3 ? "Int" : a == 4 ? "Wis" : "Cha", stats_ability[a][actor_player],
                     STATS_MOD(stats_ability[a][actor_player]), stats_ability_base[a][actor_player]);
            ok = ok && strcmp(seen_cols(1, (uint8_t)(3 + a), 20, 39), want) == 0 && INK(3 + a, 28) == L2_WHITE;
        }
        CHECK(ok);
        /* The attributes: HP and MP with now, value and base; speed's word; the attack */
        CHECK(strcmp(seen_cols(1, 10, 0, 24), "         Now  Value  Base") == 0);
        snprintf(want, sizeof(want), "HP       %3u  %5u  %4u", health_hp[actor_player],
                 health_hp_max[actor_player], health_hp_max_base[actor_player]);
        CHECK(strcmp(seen_cols(1, 11, 0, 24), want) == 0 && INK(11, 18) == L2_WHITE);
        snprintf(want, sizeof(want), "MP       %3u  %5u  %4u", mana_mp[actor_player], mana_mp_max[actor_player],
                 mana_mp_max[actor_player]);
        CHECK(strcmp(seen_cols(1, 12, 0, 24), want) == 0);
        snprintf(want, sizeof(want), "Speed         %5u  %4u  normal", stats_speed[actor_player],
                 stats_speed_base[actor_player]);
        CHECK(strcmp(seen_cols(1, 14, 0, 32), want) == 0);
        CHECK(starts(OVER_ROW(15), "Attack  ") && strstr(OVER_ROW(15), " to hit  ") != NULL);
    }
    CHECK(starts(OVER_ROW(17), "Damage  Bl Sl Pi Fi Co Po Ac Ma"));
    CHECK(starts(OVER_ROW(22), "Status  None"));
    CHECK(starts(OVER_ROW(24), "Effect           Turns  Save"));
    CHECK(starts(OVER_ROW(25), "Regeneration") && strstr(OVER_ROW(25), "always") != NULL);   /* innate */
    CHECK(starts(OVER_ROW(26), "Mana flow") && strstr(OVER_ROW(26), "always") != NULL);
    CHECK(starts(OVER_ROW(30), "Cyan: better than base  Red: worse") && INK(30, 0) == L2_CYAN &&
          INK(30, 24) == L2_RED);
    /* Values away from their bases (as effects would set them, after the poison's recompute): better cyan, worse red */
    CHECK(effect_apply(EF_POISON, 1, actor_player, NGE_ENTITY_INVALID));
    nge_event_flush();
    stats_ability[ABIL_STR][actor_player] = (uint8_t)(stats_ability_base[ABIL_STR][actor_player] + 1);
    stats_ability[ABIL_INT][actor_player] = (uint8_t)(stats_ability_base[ABIL_INT][actor_player] - 2);
    health_hp_max[actor_player] = (uint8_t)(health_hp_max_base[actor_player] + 2);
    stats_speed[actor_player] = 5;                                  /* faster: a lower number */
    health_resist[actor_player] |= DMG_BIT(DMG_FIRE);
    health_vuln[actor_player] |= DMG_BIT(DMG_COLD);
    fresh();
    press(EV(NGE_KEY_SPACE));
    screen_pc_run();
    CHECK(RAN_AND_CLOSED());
    CHECK(INK(3, 28) == L2_CYAN && INK(3, 33) == L2_CYAN && INK(3, 38) == L2_WHITE);   /* Str: value, mod; base */
    CHECK(INK(6, 28) == L2_RED && INK(6, 33) == L2_RED);                                 /* Int */
    CHECK(INK(11, 18) == L2_CYAN && INK(11, 24) == L2_WHITE);                            /* HP's value; base */
    CHECK(INK(14, 18) == L2_CYAN && strstr(OVER_ROW(14), " fast") != NULL);
    CHECK(seen[1][19][17].c == '*' && INK(19, 17) == L2_CYAN);                          /* resist fire */
    CHECK(seen[1][20][20].c == '*' && INK(20, 20) == L2_RED);                           /* vulnerable to cold */
    CHECK(starts(OVER_ROW(22), "Status  Poisoned") && INK(22, 8) == L2_RED);            /* a condition */
    CHECK(strcmp(seen_cols(1, 25, 0, 29), "Poison              12  Con 10") == 0);       /* 4 firings, 3 turns apart */
    CHECK(starts(OVER_ROW(26), "Regeneration  ") && starts(OVER_ROW(27), "Mana flow  ") &&
          starts(OVER_ROW(28), "    "));                                                  /* in EF_* order */
    /* A second regeneration (it stacks): one row, counted */
    CHECK(effect_apply(EF_REGEN, 1, actor_player, NGE_ENTITY_INVALID));
    nge_event_flush();
    fresh();
    press(EV(NGE_KEY_SPACE));
    screen_pc_run();
    CHECK(starts(OVER_ROW(26), "Regeneration x2") && strstr(OVER_ROW(26), "always") != NULL &&
          starts(OVER_ROW(27), "Mana flow  ") && starts(OVER_ROW(28), "    "));
    {
        nge_entity_id_t id;
        nge_entity_id_t last = NGE_ENTITY_INVALID;
        uint8_t i;

        NGE_OWNER_FOR_EACH_OWNED(i, id, actor_player) {
            if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_EFFECT) && effect_kind_of(id) == EF_REGEN) {
                last = id;
            }
        }
        effect_end(last);                                           /* back to the innate one */
        nge_event_flush();
    }
    /* At the highest level: no next level */
    {
        uint8_t level = pc_level;

        pc_level = PC_LEVEL_MAX;
        fresh();
        press(EV(NGE_KEY_SPACE));
        screen_pc_run();
        CHECK(strcmp(seen_cols(1, 4, 0, 17), "Next           max") == 0);
        pc_level = level;
    }
    effect_cure(actor_player, STATUS_POISONED);
    effects_recompute(actor_player);
    nge_event_flush();
    CHECK(effects_check(actor_player));

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(screen_inv_run(ACT_COUNT + 1, &chosen, &used));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(screen_inv_run(ACT_MOVE, &chosen, &used));   /* not an item action */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(screen_close());                      /* none open */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(player_ask_direction(PLAYER_PROMPT_COUNT, NULL, NULL));
    CHECK(test_halt_count == 1);

    return test_summary("ui");
}
