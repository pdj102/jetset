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

#include "game/attrs/attrs_internal.h"
#include "game/components/attack.h"
#include "game/components/health.h"
#include "game/components/feature.h"
#include "game/components/inventory_internal.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/game.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/effects/effects_defs.h"
#include "game/events.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/quest/quest.h"
#include "game/spells/spells.h"
#include "game/ui/screen.h"
#include "game/ui/screen_guide.h"
#include "game/ui/screen_inv.h"
#include "game/ui/screen_main.h"
#include "game/ui/screen_pc.h"
#include "game/ui/screen_title.h"
#include "game/chargen/chargen.h"
#include "game/ui/screen_internal.h"
#include "game/effects/effects_internal.h"

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

/*
 * The fake keyboard of the chord tests (docs/diagonals.md, 7): keys held
 * over frames. Each frame game_frame_start() reads it with the real
 * nge_input_update(), which queues the presses and the repeats. The other
 * tests give their keys through game_wait_key() and game_pause_frame()
 */
#define HOLDS_MAX  6
#define HELD       0x7FFF   /* a hold's `to`: not released */

typedef struct hold {
    uint8_t key;
    int from;   /* the first frame it is down */
    int to;     /* the first frame it is up again, or HELD */
} hold_t;

static hold_t holds[HOLDS_MAX];
static int holds_n;
static int kb_frame;   /* frames run by game_frame_start() */

/* The keys from `first` to first + 7 held this frame, as a row's bits */
static uint8_t kb_bits(uint8_t first)
{
    uint8_t bits = 0;
    int i;

    for (i = 0; i < holds_n; i++) {
        if (holds[i].key >= first && holds[i].key < first + 8 && holds[i].from <= kb_frame && kb_frame < holds[i].to) {
            bits = (uint8_t)(bits | (1 << (holds[i].key - first)));
        }
    }
    return bits;
}

static uint8_t kb_port(uint16_t port)
{
    uint8_t half_row = (uint8_t)(port >> 8);
    uint8_t r;

    for (r = 0; r < 8; r++) {
        if (half_row == (uint8_t)~(1 << r)) {
            return (uint8_t)~kb_bits((uint8_t)(r * 8));   /* active low */
        }
    }
    return 0xFF;
}

static uint8_t kb_reg(uint8_t reg)
{
    switch (reg) {
    case 0xB0:
        return kb_bits(64);
    case 0xB1:
        return kb_bits(72);
    default:
        return test_regs[reg];
    }
}

/* Hold key k from frame `from` until frame `to` (HELD: not released) */
static void hold(uint8_t k, int from, int to)
{
    CHECK(holds_n < HOLDS_MAX);
    holds[holds_n].key = k;
    holds[holds_n].from = from;
    holds[holds_n].to = to;
    holds_n++;
}

/* Release key k from the next frame */
static void let_go(uint8_t k)
{
    int i;

    for (i = 0; i < holds_n; i++) {
        if (holds[i].key == k && holds[i].to > kb_frame) {
            holds[i].to = kb_frame + 1;
        }
    }
}

void game_frame_start(void)
{
    kb_frame++;
    nge_input_update();
}

/* Start a chord test: no key held, no event or repeat waiting, at frame 0 */
static void keys_clear(void)
{
    holds_n = 0;
    game_frame_start();
    nge_key_flush();
    kb_frame = 0;
}

/*
 * The play loop on the player's turn: frames until a key event arrives,
 * then the command for it. Returns the frames the command ran
 */
static int play_key(void)
{
    nge_key_event_t ev;
    int start;

    while ((ev = nge_key_get()) == NGE_KEY_NONE && kb_frame < 100) {
        game_frame_start();
    }
    CHECK(ev != NGE_KEY_NONE);
    start = kb_frame;
    player_turn = 1;
    player_command(ev);
    return kb_frame - start;
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

/* The line n up from the newest message (0): said() for older lines */
static int said_up(int n, const char *s)
{
    int y;

    for (y = 30; y >= 26; y--) {
        if (row((uint8_t)y)[0] != ' ') {
            return y - n >= 26 && starts(row((uint8_t)(y - n)), s);
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

/* Press DOWN n times: the cursor to row n */
static void down(int n)
{
    while (n-- > 0) {
        press(EV(NGE_KEY_DOWN));
    }
}

/* The row of the player's item id in browse mode, as listed now: in use by slot, then the pack in ID order */
static int inv_pos(nge_entity_id_t id)
{
    nge_entity_id_t e = NGE_ENTITY_INVALID;
    uint8_t slot;
    int n = 0;

    for (slot = 1; slot < SLOT_COUNT; slot++) {
        if (inv_equipped(actor_player, slot) != NGE_ENTITY_INVALID) {
            if (item_slot[id] == slot) {
                return n;
            }
            n++;
        }
    }
    while ((e = inv_next(actor_player, e)) != NGE_ENTITY_INVALID && e != id) {
        n++;
    }
    return n;
}

/* The letter of the stowed item id in the e key's list: the pack's items that can be equipped, in ID order */
static nge_key_event_t inv_letter(nge_entity_id_t id)
{
    static const uint8_t keys[] = { NGE_KEY_A, NGE_KEY_B, NGE_KEY_C, NGE_KEY_D };
    nge_entity_id_t e = NGE_ENTITY_INVALID;
    int n = 0;

    while ((e = inv_next(actor_player, e)) != NGE_ENTITY_INVALID && e != id) {
        n += (gen_item_fits(kind_index[e]) & inv_slots[actor_player]) != 0;
    }
    CHECK(n < (int)sizeof(keys));
    return EV(keys[n]);
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
    static test_l2_cell_t main_before[32][40];
    uint8_t x;
    uint8_t y;

    test_init();
    effects_apply_no_save = 1;   /* the saving throw when applied is tested on its own (items_extensions.md R14) */
    test_port_read_hook = kb_port;   /* the fake keyboard: no key held until a chord test holds one */
    test_reg_read_hook = kb_reg;

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
    nge_map_set_block(LOOK_TRAP, NGE_BLOCK_NONE);        /* as the level sets it: a found trap's tile is walked on */
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
    CHECK(strcmp(text, "Str      12 ") == 0);
    test_layer2_row(10, 28, 39, text);
    CHECK(strcmp(text, "Cha      12 ") == 0);
    test_layer2_row(11, 28, 39, text);
    CHECK(strcmp(text, "            ") == 0);
    test_layer2_row(12, 28, 39, text);
    CHECK(strcmp(text, "XP        0 ") == 0);
    test_layer2_row(13, 28, 39, text);
    CHECK(strcmp(text, "Gold      0 ") == 0);
    effect_apply(EF_RAISE_STR, 1, actor_player, NGE_ENTITY_INVALID);
    player_show_stats();
    test_layer2_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      13 ") == 0);
    nge_event_flush();
    /* The conditions: short tags, 3 a row, from row 17 (docs/effects_extensions.md D6) */
    screen_show_status(0);
    test_layer2_row(17, 28, 39, text);
    CHECK(strcmp(text, "            ") == 0);
    screen_show_status(STATUS_POISONED | STATUS_CONFUSED);
    test_layer2_row(17, 28, 39, text);
    CHECK(strcmp(text, "Psn Cnf     ") == 0);
    screen_show_status(STATUS_CONDITIONS);
    test_layer2_row(17, 28, 39, text);
    CHECK(strcmp(text, "Psn Bln Slp ") == 0);
    test_layer2_row(18, 28, 39, text);
    CHECK(strcmp(text, "Rt  Cnf Frt ") == 0);
    test_layer2_row(19, 28, 39, text);
    CHECK(strcmp(text, "Par         ") == 0);
    screen_show_status(STATUS_PARALYSED);
    test_layer2_row(17, 28, 39, text);
    CHECK(strcmp(text, "Par         ") == 0);
    test_layer2_row(18, 28, 39, text);
    CHECK(strcmp(text, "            ") == 0);
    screen_show_status(0);
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

    /* ---- exchange (docs/ui_inventory.md, section 12): x in browse, e from the main screen; the pack empty ---- */
    {
        nge_entity_id_t blade = gen_item(IT_SWORD, actor_player);
        nge_entity_id_t dagger = gen_item(IT_DAGGER, actor_player);
        nge_entity_id_t ra = gen_item(IT_RING_HEALTH_2, actor_player);
        nge_entity_id_t rb = gen_item(IT_RING_HEALTH_2, actor_player);
        nge_entity_id_t rc = gen_item(IT_RING_ARMOUR_1, actor_player);
        nge_entity_id_t frail;

        /* The sword in use; a full pack */
        item_slot[blade] = SLOT_WEAPON;
        effects_recompute(actor_player);
        nge_event_flush();
        inv_capacity[actor_player] = inv_count(actor_player);
        /* The help: w refused for the taken slot, so x:swap (R24) */
        fresh();
        down(inv_pos(dagger));
        press(EV(NGE_KEY_SPACE));
        screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
        CHECK(RAN_AND_CLOSED() && starts(HELP(), "jk:select x:swap d:drop SPACE:back"));
        /* On a free slot only w:wear, though x would do the same */
        fresh();
        down(inv_pos(ra));
        press(EV(NGE_KEY_SPACE));
        screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used);
        CHECK(RAN_AND_CLOSED() && starts(HELP(), "jk:select w:wear d:drop SPACE:back"));
        /* x with a full pack: one for one, the screen stays open, the cursor with the dagger, now first */
        fresh();
        player_turn = 1;
        down(inv_pos(dagger));
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_I));
        CHECK(RAN_AND_CLOSED() && !player_turn && nge_event_count() == 0);
        CHECK(item_slot[dagger] == SLOT_WEAPON && stowed(blade));
        CHECK(starts(LIST_ROW(0), "    1 dagger") && HIGHLIGHTED(0) && INK(2, 6) == L2_CYAN);
        CHECK(starts(STATUS(), "You take off a sword.") && starts(STATUS2(), "You equip a dagger."));
        CHECK(said("You equip a dagger.") && shown("You take off a sword."));
        CHECK(strcmp(OVER_ROW(0) + 30, "Pack  4/ 4") == 0);
        CHECK(starts(HELP(), "jk:select SPACE:done"));               /* a full pack: no r:off */
        /* x on what is in use: refused, as w is (R22) */
        fresh();
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_SPACE));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && !used && RAN_AND_CLOSED());
        CHECK(starts(STATUS(), "You are already using that.") && item_slot[dagger] == SLOT_WEAPON);
        /* A free ring slot: x equips (R19) */
        fresh();
        down(inv_pos(ra));
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_SPACE));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used && RAN_AND_CLOSED());
        CHECK(item_slot[ra] == SLOT_RING_L && starts(STATUS(), "You equip a ring of health +2."));
        /* One ring worn: x replaces it, without asking, and the right hand stays free (R21) */
        fresh();
        down(inv_pos(rc));
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_SPACE));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used && RAN_AND_CLOSED());
        CHECK(item_slot[rc] == SLOT_RING_L && stowed(ra) &&
              inv_equipped(actor_player, SLOT_RING_R) == NGE_ENTITY_INVALID);
        /* Both worn: the question, and b, the right one, goes */
        item_slot[rb] = SLOT_RING_R;
        effects_recompute(actor_player);
        nge_event_flush();
        fresh();
        down(inv_pos(ra));
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_B));
        press(EV(NGE_KEY_SPACE));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && used && RAN_AND_CLOSED());
        CHECK(item_slot[ra] == SLOT_RING_R && stowed(rb) && item_slot[rc] == SLOT_RING_L);
        CHECK(starts(OVER_ROW(0), "          Worldly Possessions"));     /* the question's title gone */
        CHECK(starts(STATUS(), "You take off a ring of health +2.") && HIGHLIGHTED(inv_pos(ra)));
        /* The question given up: nothing done, back to browse, the cursor where it was */
        fresh();
        down(inv_pos(rb));
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_SPACE));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && !used && RAN_AND_CLOSED());
        CHECK(stowed(rb) && HIGHLIGHTED(inv_pos(rb)) && starts(OVER_ROW(0), "          Worldly Possessions"));
        /* A cursed ring on the left: chosen, refused with its reason, the screen stays */
        frail = gen_item(IT_RING_FRAILTY, actor_player);
        item_slot[rc] = SLOT_NONE;
        item_slot[frail] = SLOT_RING_L;
        effects_recompute(actor_player);
        nge_event_flush();
        fresh();
        down(inv_pos(rb));
        press(EV(NGE_KEY_X));
        press(EV(NGE_KEY_A));                   /* the cursed one, on the left */
        press(EV(NGE_KEY_SPACE));
        CHECK(screen_inv_run(SCREEN_INV_BROWSE, &chosen, &used) == SCREEN_INV_BACK && !used && RAN_AND_CLOSED());
        CHECK(starts(STATUS(), "It is cursed: it won't come off.") && item_slot[frail] == SLOT_RING_L &&
              stowed(rb));
        /* e from the main screen: choose mode; a letter, then the question's b; it closes, one turn */
        fresh();
        player_turn = 1;
        press(inv_letter(rb));
        press(EV(NGE_KEY_B));                   /* ra, on the right */
        player_command(EV(NGE_KEY_E));
        CHECK(RAN_AND_CLOSED() && !player_turn && nge_event_count() == 0);
        CHECK(item_slot[rb] == SLOT_RING_R && stowed(ra) && said("You equip a ring of health +2."));
        /* e, the question given up: back to the list, then SPACE; no turn */
        fresh();
        player_turn = 1;
        press(inv_letter(rc));
        press(EV(NGE_KEY_SPACE));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_E));
        CHECK(RAN_AND_CLOSED() && player_turn && stowed(rc));
        CHECK(starts(OVER_ROW(0), "           Exchange Thy Gear"));       /* (40 - 17) / 2 */
        /* e on a weapon: no question; it closes, one turn */
        fresh();
        player_turn = 1;
        press(inv_letter(blade));
        player_command(EV(NGE_KEY_E));
        CHECK(RAN_AND_CLOSED() && !player_turn && item_slot[blade] == SLOT_WEAPON && stowed(dagger));

        /* Gone, for the tests below */
        item_slot[blade] = SLOT_NONE;
        item_slot[rb] = SLOT_NONE;
        item_slot[frail] = SLOT_NONE;
        nge_entity_destroy(blade);
        nge_entity_destroy(dagger);
        nge_entity_destroy(ra);
        nge_entity_destroy(rb);
        nge_entity_destroy(rc);
        nge_entity_destroy(frail);
        inv_capacity[actor_player] = 20;
        effects_recompute(actor_player);
        nge_event_flush();
    }

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
            { IT_RING_ARMOUR_1,  1, "ring of armour +1",  "AC 1" },   /* jewellery: its AC from its hook */
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

    /* ---- the title: any key, and nothing else (the name is asked at character creation) ---- */
    fresh();
    press(EV(NGE_KEY_A));
    screen_title_run();
    CHECK(RAN_AND_CLOSED() && waits == 0);
    CHECK(strcmp(OVER_ROW(11), "               NGE ROGUE                ") == 0);
    CHECK(strcmp(seen_cols(1, 29, 0, 39), BLANK) == 0);   /* no prompt */
    CHECK(starts(HELP(), "Any key: start a new game"));
    /* A new experience level's message: a Fighter's row has no slots, so only the welcome */
    CHECK(MANA_IS(actor_player) && pc_class == PC_CLASS_FIGHTER);
    nge_event_post(actor_player, EV_LEVEL_GAINED, NGE_EVENT_NONE, 2, 0);
    nge_event_process(messages_on_event);
    CHECK(said("Welcome to experience level 2."));
    /* A Mage's level that adds a slot: the welcome, then the slots (docs/spells.md, R39), whatever pc_level is now */
    pc_class = PC_CLASS_MAGE;
    nge_event_post(actor_player, EV_LEVEL_GAINED, NGE_EVENT_NONE, 3, 0);
    nge_event_process(messages_on_event);
    CHECK(said("You can learn more spells from a book.") && shown("Welcome to experience level 3."));
    /* Without mana, none: a class's table counts only for the player with mana (R36) */
    NGE_ENTITY_REMOVE_COMPONENT(actor_player, COMPONENT_MANA);
    nge_event_post(actor_player, EV_LEVEL_GAINED, NGE_EVENT_NONE, 4, 0);
    nge_event_process(messages_on_event);
    CHECK(said("Welcome to experience level 4."));
    NGE_ENTITY_ADD_COMPONENT(actor_player, COMPONENT_MANA);
    pc_class = PC_CLASS_FIGHTER;

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

    /* ---- operate: o and a direction; the kind's verb; spent, told so, the turn kept ---- */
    {
        nge_entity_id_t shrine = gen_feature(FT_SHRINE_HEALING, AREA);

        feature_place(shrine, 5, 4);                               /* north of the player */
        nge_event_flush();
        player_turn = 1;
        press(EV(NGE_KEY_K));
        player_command(EV(NGE_KEY_O));
        CHECK(said("Use what? Which direction?") && FEATURE_IS_SPENT(shrine) && !player_turn);
        nge_event_process(messages_on_event);
        CHECK(shown("You pray at the healing shrine."));   /* the healing's own message after it */
        player_turn = 1;
        press(EV(NGE_KEY_K));
        player_command(EV(NGE_KEY_O));
        CHECK(said("The healing shrine is spent.") && player_turn);
        nge_location_remove(shrine);                               /* out of the way of what follows */
        nge_entity_destroy(shrine);
        nge_event_flush();
    }

    /* ---- a trap going off at the player: by its kind's verb (docs/features.md 4.11) ---- */
    {
        nge_entity_id_t dart = gen_feature(FT_DART_TRAP, AREA);
        nge_entity_id_t pit = gen_feature(FT_PIT, AREA);
        uint8_t px = NGE_LOCATION_X(actor_player);
        uint8_t py = NGE_LOCATION_Y(actor_player);

        feature_place(dart, px, py);                               /* stood on: hidden, so the map is untouched */
        feature_place(pit, px, py);
        nge_event_post(dart, EV_TRIGGERED, actor_player, 0, 0);
        nge_event_process(messages_on_event);
        CHECK(said("You step on a dart trap!"));
        nge_event_post(pit, EV_TRIGGERED, actor_player, 0, 0);
        nge_event_process(messages_on_event);
        CHECK(said("You fall into a pit!"));
        {
            nge_entity_id_t td = gen_feature(FT_TRAPDOOR, AREA);

            feature_place(td, px, py);
            nge_event_post(td, EV_TRIGGERED, actor_player, 0, 0);
            nge_event_process(messages_on_event);
            CHECK(said("You fall through a trapdoor!"));
            nge_location_remove(td);
            nge_entity_destroy(td);
        }
        /* Off its tile, a failed disarm set it off */
        nge_location_set(pit, (uint8_t)(px + 1), py);
        nge_event_post(pit, EV_TRIGGERED, actor_player, 0, 0);
        nge_event_process(messages_on_event);
        CHECK(said("You set off a pit!"));
        nge_location_remove(dart);
        nge_location_remove(pit);
        nge_entity_destroy(dart);
        nge_entity_destroy(pit);
    }

    /* ---- known traps: a step onto one walks onto it (only D disarms), a step into a web strikes it; s searches; D asks ---- */
    {
        nge_entity_id_t dart = gen_feature(FT_DART_TRAP, AREA);
        nge_entity_id_t web = gen_feature(FT_WEB, AREA);
        uint8_t px = NGE_LOCATION_X(actor_player);
        uint8_t py = NGE_LOCATION_Y(actor_player);
        nge_tile_t north = NGE_MAP_TILE(px, py - 1);                  /* put back after: the tests below use them */
        nge_tile_t west = NGE_MAP_TILE(px - 1, py);

        feature_place(dart, px, (uint8_t)(py - 1));                /* north */
        feature_find(dart);
        feature_place(web, (uint8_t)(px - 1), py);                 /* west */
        nge_event_flush();
        player_turn = 1;
        player_command(EV(NGE_KEY_K));                             /* onto the trap: walked onto, never disarmed */
        CHECK(NGE_LOCATION_Y(actor_player) == py - 1 && !player_turn && NGE_ENTITY_IS_ALIVE(dart));
        nge_event_process(messages_on_event);
        CHECK(!shown("You disarm the dart trap.") && !shown("You fail to disarm the dart trap."));
        nge_location_set(actor_player, px, py);                    /* back: the tests below start here */
        nge_event_flush();
        player_turn = 1;
        player_command(EV(NGE_KEY_H));                             /* into the web: struck, not walked into */
        CHECK(NGE_LOCATION_X(actor_player) == px && !player_turn);
        nge_event_process(messages_on_event);
        CHECK(shown("You hit the spider web.") || shown("You miss the spider web."));
        player_turn = 1;
        player_command(EV(NGE_KEY_S));                             /* nothing hidden around */
        CHECK(!player_turn);
        nge_event_process(messages_on_event);
        CHECK(said("You find nothing."));
        player_turn = 1;
        press(EV(NGE_KEY_L));
        player_command(NGE_KEY_D | (NGE_MOD_CAPS << 8));
        CHECK(said("Disarm in which direction?") || shown("Disarm in which direction?"));
        /* Off the map, alive or already dying (disarmed, broken): the clean-up would redraw around them */
        nge_location_remove(dart);
        nge_location_remove(web);
        if (NGE_ENTITY_IS_ALIVE(dart)) {
            nge_entity_destroy(dart);
        }
        if (NGE_ENTITY_IS_ALIVE(web)) {
            nge_entity_destroy(web);
        }
        NGE_MAP_TILE(px, py - 1) = north;
        NGE_MAP_TILE(px - 1, py) = west;
        nge_event_flush();
    }
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
    /* An effect's op that did nothing: said only to the player (docs/effects_extensions.md D2) */
    screen_message("-");
    nge_event_post(door, EV_FIZZLED, NGE_EVENT_NONE, OP_LEVEL_TELEPORT, 0);
    nge_event_process(messages_on_event);
    CHECK(said("-"));
    nge_event_post(actor_player, EV_FIZZLED, NGE_EVENT_NONE, OP_LEVEL_TELEPORT, 0);
    nge_event_process(messages_on_event);
    CHECK(said("Nothing happens."));

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
        a.reason = ACT_R_AFRAID;
        messages_act_failed(&a);
        CHECK(said("You are too afraid."));
        a.reason = ACT_R_STUMBLED;
        messages_act_failed(&a);
        CHECK(said("You stumble."));
    }
    /* Look: what the player sees on a tile, from their EV_LOOKED (docs/action_look.md R8 to R10) */
    {
        act_t a = { 0 };
        uint8_t px = NGE_LOCATION_X(actor_player);
        uint8_t py = NGE_LOCATION_Y(actor_player);
        uint8_t x = (uint8_t)(px + 3);
        nge_entity_id_t ring = gen_item(IT_RING_HEALTH_2, AREA);
        nge_entity_id_t sword = gen_item(IT_SWORD, AREA);
        nge_entity_id_t trap = gen_feature(FT_DART_TRAP, AREA);
        nge_entity_id_t rat = gen_creature(CR_RAT, AREA);

        CHECK(nge_location_find(AREA, x, py, COMPONENT_KIND, 0) == NGE_ENTITY_INVALID);
        nge_event_flush();
        nge_event_post(actor_player, EV_LOOKED, NGE_EVENT_NONE, x, py);
        nge_event_process(messages_on_event);
        CHECK(said("You see nothing there."));
        nge_location_set(ring, x, py);                              /* two items: two lines, in ID order */
        nge_location_set(sword, x, py);
        nge_event_post(actor_player, EV_LOOKED, NGE_EVENT_NONE, x, py);
        nge_event_process(messages_on_event);
        CHECK(said_up(1, "You see a ring of health +2.") && said("You see a sword."));
        CHECK(nge_location_find(AREA, x, (uint8_t)(py + 3), COMPONENT_KIND, 0) == NGE_ENTITY_INVALID);
        nge_location_set(actor_player, x, (uint8_t)(py + 3));      /* the player's own tile: themselves first */
        nge_location_set(ring, x, (uint8_t)(py + 3));
        nge_event_post(actor_player, EV_LOOKED, NGE_EVENT_NONE, x, (uint8_t)(py + 3));
        nge_event_process(messages_on_event);
        feature_place(trap, x, (uint8_t)(py + 1));                  /* a hidden trap isn't seen */
        nge_event_flush();
        CHECK(FEATURE_IS_HIDDEN(trap));
        nge_event_post(actor_player, EV_LOOKED, NGE_EVENT_NONE, x, (uint8_t)(py + 1));
        nge_event_process(messages_on_event);
        CHECK(said("You see nothing there."));
        feature_find(trap);                                         /* found, it is */
        nge_event_flush();
        nge_event_post(actor_player, EV_LOOKED, NGE_EVENT_NONE, x, (uint8_t)(py + 1));
        nge_event_process(messages_on_event);
        CHECK(said("You see a dart trap."));
        nge_location_set(rat, x, (uint8_t)(py + 2));                /* another's look says nothing */
        nge_event_flush();
        screen_message("Marker.");
        nge_event_post(rat, EV_LOOKED, NGE_EVENT_NONE, x, py);
        nge_event_process(messages_on_event);
        CHECK(said("Marker."));
        a.reason = ACT_R_BLIND;
        messages_act_failed(&a);
        CHECK(said("You can't see!"));
        /* Off the map: the tests below don't expect them */
        nge_location_remove(ring);
        nge_location_remove(sword);
        nge_location_remove(trap);
        nge_location_remove(rat);
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
        /* A wand of sleep at a rat in sight: the aim and the effect said (items_extensions.md R12) */
        {
            nge_entity_id_t wand = gen_item(IT_WAND_SLEEP, actor_player);
            nge_entity_id_t rat = gen_creature(CR_RAT, AREA);

            nge_location_set(rat, (uint8_t)(NGE_LOCATION_X(actor_player) - 1), NGE_LOCATION_Y(actor_player));
            nge_event_flush();
            fresh();
            player_turn = 1;
            press(EV(NGE_KEY_A));
            press(EV(NGE_KEY_SPACE));
            press(EV(NGE_KEY_LEFT));
            press(EV(NGE_KEY_ENTER));
            player_command(EV(NGE_KEY_A));
            nge_event_process(messages_on_event);
            CHECK(!player_turn && starts(row(29), "You aim the wand of sleep.") && said("The rat falls asleep."));
            /* Resisted when applied: said for a monster in sight and for the player (R14) */
            nge_event_post(rat, EV_RESISTED, actor_player, EF_SLEEP, 0);
            nge_event_process(messages_on_event);
            CHECK(said("The rat resists."));
            nge_event_post(actor_player, EV_RESISTED, NGE_EVENT_NONE, EF_POISON, 0);
            nge_event_process(messages_on_event);
            CHECK(said("You resist."));
            nge_entity_destroy(rat);
            nge_entity_destroy(wand);
            nge_event_flush();
            player_turn = 1;
        }
        /* A blink parchment is offered with nobody near, and asks where to (docs/items_aimed.md R13, R15) */
        {
            nge_entity_id_t sp = gen_item(IT_SP_BLINK, actor_player);
            uint8_t x = NGE_LOCATION_X(actor_player);
            uint8_t y = NGE_LOCATION_Y(actor_player);

            nge_event_flush();
            fresh();
            player_turn = 1;
            press(EV(NGE_KEY_A));
            press(EV(NGE_KEY_SPACE));
            press(EV(NGE_KEY_LEFT));
            press(EV(NGE_KEY_ENTER));
            player_command(EV(NGE_KEY_A));
            CHECK(!player_turn && !NGE_ENTITY_IS_ALIVE(sp) && NGE_LOCATION_X(actor_player) == x - 1 &&
                  NGE_LOCATION_Y(actor_player) == y);
            nge_event_flush();
            nge_location_set(actor_player, x, y);
            player_turn = 1;
        }
        /*
         * A panic parchment goes as soon as it is chosen, with no prompt; a
         * wand of fireballs is offered with nobody near, and asks for a tile
         * (docs/items_aimed.md R13, R15)
         */
        {
            nge_entity_id_t sp = gen_item(IT_SP_PANIC, actor_player);
            nge_entity_id_t rat = gen_creature(CR_RAT, AREA);
            nge_entity_id_t wand;
            uint8_t charges;

            nge_location_set(rat, (uint8_t)(NGE_LOCATION_X(actor_player) - 2), NGE_LOCATION_Y(actor_player));
            nge_event_flush();
            fresh();
            player_turn = 1;
            press(EV(NGE_KEY_A));
            press(EV(NGE_KEY_SPACE));
            player_command(EV(NGE_KEY_A));
            CHECK(!player_turn && !NGE_ENTITY_IS_ALIVE(sp));
            nge_entity_destroy(rat);
            nge_event_flush();
            wand = gen_item(IT_WAND_FIREBALL, actor_player);
            charges = item_count[wand];
            fresh();
            player_turn = 1;
            press(EV(NGE_KEY_A));
            press(EV(NGE_KEY_SPACE));
            press(EV(NGE_KEY_Q));                                  /* the cursor ran: cancelled */
            player_command(EV(NGE_KEY_A));
            CHECK(player_turn && item_count[wand] == charges && said("Never mind."));
            nge_entity_destroy(wand);
            nge_event_flush();
            player_turn = 1;
        }
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
        spell_learn(actor_player, SP_CURE_WOUNDS);                 /* a second spell, as a book would teach */
        nge_event_flush();
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
        /* A self spell (b, Cure Wounds, learned above): no tile asked for */
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
            /* An effect on a monster in sight: its start said, a save said, running out not (items_extensions.md R12, R13) */
            effect_apply(EF_FEAR, 1, shaman, actor_player);
            nge_event_process(messages_on_event);
            CHECK(said("The goblin shaman is terrified!"));
            nge_event_post(shaman, EV_EFFECT_ENDED, NGE_EVENT_NONE, EF_FEAR, EFFECT_END_SAVED);
            nge_event_process(messages_on_event);
            CHECK(said("The goblin shaman shakes it off."));
            nge_event_post(shaman, EV_EFFECT_ENDED, NGE_EVENT_NONE, EF_FEAR, EFFECT_END_EXPIRED);
            nge_event_process(messages_on_event);
            CHECK(said("The goblin shaman shakes it off."));   /* nothing new */
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

    /* ---- read: b chooses a book in Study mode, the book screen learns (docs/spells.md, section 10) ---- */
    {
        static const uint8_t bolt[] = { SP_MAGIC_BOLT };
        uint8_t cls = pc_class;
        uint8_t lvl = pc_level;
        nge_entity_id_t book;

        /* No book: said, and no screen */
        fresh();
        player_turn = 1;
        player_command(EV(NGE_KEY_B));
        CHECK(said("You have no spell book.") && player_turn && paused == 0);

        /* A level 1 Mage knowing Magic Bolt, with a basic spellbook */
        spell_add(actor_player, bolt, sizeof(bolt));
        pc_class = PC_CLASS_MAGE;
        pc_level = 1;
        book = gen_item(IT_BOOK_BASIC, actor_player);
        nge_event_flush();

        /* b: Study lists the book; choosing it opens the book: the rows coloured, the key and the free slots */
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the Study screen's closing frame */
        press(EV(NGE_KEY_C));                                      /* Hold: red, and why */
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_B));
        CHECK(RAN_AND_CLOSED() && player_turn);                    /* nothing learned: no turn (D22) */
        CHECK(strstr(OVER_ROW(0), "basic spellbook") != NULL && INK(0, 12) == L2_YELLOW);
        CHECK(starts(OVER_ROW(3), " a) Magic Bolt") && INK(3, 4) == L2_CYAN);
        CHECK(starts(OVER_ROW(4), " b) Cure Wounds") && INK(4, 4) == L2_WHITE);
        CHECK(starts(OVER_ROW(5), " c) Hold") && INK(5, 4) == L2_RED && starts(seen_cols(1, 5, 20, 28), "    2   2"));
        CHECK(starts(OVER_ROW(6), " d) Enfeeble") && INK(6, 4) == L2_RED);
        CHECK(starts(OVER_ROW(7), " e) Blink") && INK(7, 4) == L2_RED && strcmp(OVER_ROW(8), BLANK) == 0);
        CHECK(starts(OVER_ROW(25), " cyan: known  red: can't learn yet") && INK(25, 1) == L2_CYAN && INK(25, 14) == L2_RED);
        CHECK(starts(OVER_ROW(26), " Free slots  L1:1 L2:0 L3:0 L4:0 L5:0"));
        CHECK(starts(STATUS(), "That spell is beyond you for now."));
        CHECK(starts(HELP(), "Letter/ENTER: learn  SPACE: close"));
        /* The Study screen itself, backed out of */
        fresh();
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_B));
        CHECK(RAN_AND_CLOSED() && player_turn && strstr(OVER_ROW(0), " Study ") != NULL);
        CHECK(starts(LIST_ROW(0), "a)") && strstr(LIST_ROW(0), "basic spellbook") != NULL && strcmp(LIST_ROW(1), BLANK) == 0);
        CHECK(starts(HELP(), "Letter/ENTER: read it  SPACE: back"));

        /* A known row: why; then Cure Wounds learned: said, cyan, the foot, the screen open; closing uses the turn */
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the Study screen's closing frame */
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_B));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_B));
        CHECK(RAN_AND_CLOSED() && !player_turn && spell_knows(actor_player, SP_CURE_WOUNDS));
        CHECK(starts(STATUS(), "You learn Cure Wounds.") && INK(4, 4) == L2_CYAN && HIGHLIGHTED(2));
        CHECK(starts(OVER_ROW(26), " Free slots  L1:0 L2:0"));
        CHECK(shown("You learn Cure Wounds.") && NGE_ENTITY_IS_ALIVE(book) && NGE_OWNER_OF(book) == actor_player);
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the Study screen's closing frame */
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        player_turn = 1;
        player_command(EV(NGE_KEY_B));
        CHECK(RAN_AND_CLOSED() && player_turn && starts(STATUS(), "You know that spell already."));

        /* Level 3: Hold learned; Enfeeble turns red, its level's one slot filled: why, and the screen stays open */
        pc_level = 3;
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the Study screen's closing frame */
        press(EV(NGE_KEY_C));
        press(EV(NGE_KEY_D));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_B));
        CHECK(RAN_AND_CLOSED() && !player_turn && spell_knows(actor_player, SP_HOLD) && !spell_knows(actor_player, SP_ENFEEBLE));
        CHECK(INK(5, 4) == L2_CYAN && INK(6, 4) == L2_RED && starts(STATUS(), "No room for more spells of that level."));
        CHECK(starts(OVER_ROW(26), " Free slots  L1:1 L2:0"));
        nge_event_flush();

        /* Browse mode: b:read on the help line for a book; b, and ENTER, open it */
        fresh();
        down(inv_pos(book));
        press(EV(NGE_KEY_B));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the inventory's closing frame */
        press(EV(NGE_KEY_SPACE));
        player_turn = 1;
        player_command(EV(NGE_KEY_I));
        CHECK(RAN_AND_CLOSED() && player_turn && strstr(OVER_ROW(0), "basic spellbook") != NULL);
        fresh();
        down(inv_pos(book));
        press(EV(NGE_KEY_ENTER));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the inventory's closing frame */
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_I));
        CHECK(RAN_AND_CLOSED() && player_turn && strstr(OVER_ROW(0), "basic spellbook") != NULL);
        fresh();
        down(inv_pos(book));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_I));
        CHECK(RAN_AND_CLOSED() && strstr(HELP(), " b:read") != NULL);

        /* No mana, as a Fighter: the book refused in Study, why on the status line, the screen still open */
        NGE_ENTITY_REMOVE_COMPONENT(actor_player, COMPONENT_MANA);
        fresh();
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));
        player_command(EV(NGE_KEY_B));
        CHECK(RAN_AND_CLOSED() && player_turn && strstr(OVER_ROW(0), " Study ") != NULL &&
              starts(STATUS(), "The spells mean nothing to you."));
        NGE_ENTITY_ADD_COMPONENT(actor_player, COMPONENT_MANA);

        nge_entity_destroy(book);
        nge_event_flush();
        spell_add(actor_player, bolt, sizeof(bolt));
        pc_class = cls;
        pc_level = lvl;
    }

    /* ---- diagonals: two direction keys held together (docs/diagonals.md) ---- */
    {
        uint8_t ox = NGE_LOCATION_X(actor_player);
        uint8_t oy = NGE_LOCATION_Y(actor_player);
        uint8_t px = 30;
        uint8_t py = 20;
        int8_t dx;
        int8_t dy;
        nge_entity_id_t d2;
        nge_entity_id_t key;
        nge_entity_id_t shrine;
        nge_entity_id_t dart;

        nge_location_set(actor_player, px, py);                    /* open floor, nothing near */
        nge_event_flush();
        fresh();
        /* UP and RIGHT in the same frame: one step up-right, one turn, at once; the second press taken */
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        hold(NGE_KEY_RIGHT, 1, HELD);
        CHECK(play_key() == 0);
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1 && !player_turn);
        CHECK(nge_key_waiting() == 0);
        px++;
        py--;
        /* UP, then RIGHT two frames later: one step up-right after 2 frames; RIGHT's press taken, a key after it kept */
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        hold(NGE_KEY_RIGHT, 3, HELD);
        hold(NGE_KEY_S, 4, 5);
        CHECK(play_key() == 2);
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1 && !player_turn);
        CHECK(nge_key_waiting() == 0);
        game_frame_start();
        CHECK(nge_key_peek() == EV(NGE_KEY_S));
        px++;
        py--;
        /* UP alone, held past the window: straight up, after the window */
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        CHECK(play_key() == 4);
        CHECK(NGE_LOCATION_X(actor_player) == px && NGE_LOCATION_Y(actor_player) == py - 1 && !player_turn);
        py--;
        /* DOWN tapped, up after 2 frames: straight down in 2 frames */
        keys_clear();
        hold(NGE_KEY_DOWN, 1, 3);
        CHECK(play_key() == 2);
        CHECK(NGE_LOCATION_X(actor_player) == px && NGE_LOCATION_Y(actor_player) == py + 1 && !player_turn);
        py++;
        /* The key sets mixed: k with RIGHT; CAPS+7 with l (l comes as L, CAPS held) */
        keys_clear();
        hold(NGE_KEY_K, 1, HELD);
        hold(NGE_KEY_RIGHT, 2, HELD);
        CHECK(play_key() == 1);
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1 && nge_key_waiting() == 0);
        px++;
        py--;
        keys_clear();
        hold(NGE_KEY_CAPS, 1, HELD);
        hold(NGE_KEY_7, 1, HELD);
        hold(NGE_KEY_L, 2, HELD);
        CHECK(play_key() == 1);
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1 && nge_key_waiting() == 0);
        px++;
        py--;
        /* LEFT, RIGHT and UP held: the event's key decides its own axis; both ways held on the other give 0 */
        keys_clear();
        hold(NGE_KEY_LEFT, 1, HELD);
        hold(NGE_KEY_RIGHT, 1, HELD);
        hold(NGE_KEY_UP, 1, HELD);
        game_frame_start();
        CHECK(player_key_direction(EV(NGE_KEY_LEFT), &dx, &dy) && dx == -1 && dy == -1);
        CHECK(player_key_direction(EV(NGE_KEY_RIGHT), &dx, &dy) && dx == 1 && dy == -1);
        CHECK(player_key_direction(EV(NGE_KEY_UP), &dx, &dy) && dx == 0 && dy == -1);
        /* The cursor's reading: both keys down, a diagonal, with no frames run; one key, straight */
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        hold(NGE_KEY_RIGHT, 1, HELD);
        game_frame_start();
        CHECK(player_key_direction(EV(NGE_KEY_UP), &dx, &dy) && dx == 1 && dy == -1 && kb_frame == 1);
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        game_frame_start();
        CHECK(player_key_direction(EV(NGE_KEY_UP), &dx, &dy) && dx == 0 && dy == -1 && kb_frame == 1);

        /* Into a monster: an attack, not a step */
        rat = gen_creature(CR_RAT, AREA);
        nge_location_set(rat, (uint8_t)(px + 1), (uint8_t)(py - 1));
        nge_event_flush();
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        hold(NGE_KEY_RIGHT, 1, HELD);
        play_key();
        CHECK(NGE_LOCATION_X(actor_player) == px && NGE_LOCATION_Y(actor_player) == py && !player_turn);
        nge_event_process(messages_on_event);
        CHECK(shown("You hit the rat") || shown("You miss the rat") || shown("You kill the rat"));
        if (NGE_ENTITY_IS_ALIVE(rat)) {
            nge_location_remove(rat);
            nge_entity_destroy(rat);
        }
        level_release_dying();
        nge_event_flush();
        /* Into a closed door: it opens */
        d2 = gen_feature(FT_DOOR, AREA);
        feature_place(d2, (uint8_t)(px - 1), (uint8_t)(py + 1));
        nge_event_flush();
        keys_clear();
        hold(NGE_KEY_DOWN, 1, HELD);
        hold(NGE_KEY_LEFT, 1, HELD);
        play_key();
        CHECK(FEATURE_IS_OPEN(d2) && NGE_LOCATION_X(actor_player) == px && !player_turn);
        nge_event_flush();

        /* The prompts answer a chord: close, unlock, operate, disarm, each down-left or up-left */
        keys_clear();
        hold(NGE_KEY_DOWN, 1, HELD);
        hold(NGE_KEY_LEFT, 1, HELD);
        game_frame_start();
        fresh();
        player_turn = 1;
        press(EV(NGE_KEY_DOWN));
        player_command(EV(NGE_KEY_C));
        CHECK(!FEATURE_IS_OPEN(d2) && !player_turn && KEYS_USED());
        nge_event_flush();
        feature_lock(d2, KEY_IRON);
        key = gen_item(IT_KEY_IRON, actor_player);
        fresh();
        player_turn = 1;
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_SPACE));                                  /* taken by the screen's closing frame */
        press(EV(NGE_KEY_LEFT));
        player_command(EV(NGE_KEY_P));
        CHECK(!FEATURE_IS_LOCKED(d2) && !player_turn);
        nge_entity_destroy(key);
        level_release_dying();
        nge_location_remove(d2);
        nge_entity_destroy(d2);
        nge_event_flush();
        shrine = gen_feature(FT_SHRINE_HEALING, AREA);
        feature_place(shrine, (uint8_t)(px - 1), (uint8_t)(py + 1));
        nge_event_flush();
        fresh();
        player_turn = 1;
        press(EV(NGE_KEY_LEFT));
        player_command(EV(NGE_KEY_O));
        CHECK(FEATURE_IS_SPENT(shrine) && !player_turn);
        nge_location_remove(shrine);
        nge_entity_destroy(shrine);
        nge_event_flush();
        keys_clear();
        hold(NGE_KEY_UP, 1, HELD);
        hold(NGE_KEY_LEFT, 1, HELD);
        game_frame_start();
        dart = gen_feature(FT_DART_TRAP, AREA);
        feature_place(dart, (uint8_t)(px - 1), (uint8_t)(py - 1));
        feature_find(dart);
        nge_event_flush();
        fresh();
        player_turn = 1;
        press(EV(NGE_KEY_UP));
        player_command(NGE_KEY_D | (NGE_MOD_CAPS << 8));
        CHECK(!player_turn && KEYS_USED());
        nge_event_process(messages_on_event);
        CHECK(shown("You disarm the dart trap.") || shown("You fail to disarm the dart trap.") ||
              shown("You set off a dart trap!"));
        nge_location_remove(dart);
        if (NGE_ENTITY_IS_ALIVE(dart)) {
            nge_entity_destroy(dart);
        }
        level_release_dying();
        nge_event_flush();
        /* A key held from play repeating into a prompt doesn't answer it; a press does */
        keys_clear();
        fresh();
        press(NGE_KEY_RIGHT | (NGE_MOD_REPEAT << 8));
        press(EV(NGE_KEY_UP));
        CHECK(player_ask_direction(PLAYER_PROMPT_CLOSE, &dx, &dy) && dx == 0 && dy == -1 && KEYS_USED());

        /* Holding a chord repeats the diagonal; releasing a key, the next repeat is refused: no step, no turn */
        keys_clear();
        hold(NGE_KEY_RIGHT, 1, HELD);
        hold(NGE_KEY_UP, 2, HELD);                                 /* UP last down: UP repeats */
        CHECK(play_key() == 1);
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1);
        px++;
        py--;
        CHECK(play_key() == 0);                                    /* the repeat, up-right */
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1 && !player_turn);
        px++;
        py--;
        let_go(NGE_KEY_RIGHT);
        play_key();                                                /* UP's repeat, now straight: refused */
        CHECK(NGE_LOCATION_X(actor_player) == px && NGE_LOCATION_Y(actor_player) == py && player_turn);
        let_go(NGE_KEY_UP);
        game_frame_start();
        hold(NGE_KEY_UP, kb_frame + 1, kb_frame + 2);              /* a new press: straight up */
        play_key();
        CHECK(NGE_LOCATION_X(actor_player) == px && NGE_LOCATION_Y(actor_player) == py - 1 && !player_turn);
        py--;
        /* Walking right, then pressing UP: the next step is up-right at once (R11) */
        keys_clear();
        hold(NGE_KEY_RIGHT, 1, HELD);
        CHECK(play_key() == 4);                                    /* the press, straight after the window */
        CHECK(play_key() == 0);                                    /* a repeat */
        CHECK(NGE_LOCATION_X(actor_player) == px + 2 && NGE_LOCATION_Y(actor_player) == py);
        px = (uint8_t)(px + 2);
        hold(NGE_KEY_UP, kb_frame + 1, HELD);
        CHECK(play_key() == 0);
        CHECK(NGE_LOCATION_X(actor_player) == px + 1 && NGE_LOCATION_Y(actor_player) == py - 1);

        keys_clear();
        fresh();
        nge_location_set(actor_player, ox, oy);
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
            if (NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_EFFECT) && EFFECT_KIND_OF(id) == EF_REGEN) {
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

    /* ---- the help screen (docs/help_screen.md) ---- */
    /* Turning: right and l on, left and h back, wrapping round; the digits go to a page; other keys stay */
    CHECK(screen_guide_turn(0, 9) == 1 && screen_guide_turn(0, 'l') == 1);
    CHECK(screen_guide_turn(3, 9) == 0 && screen_guide_turn(3, 'l') == 0);
    CHECK(screen_guide_turn(0, 8) == 3 && screen_guide_turn(0, 'h') == 3);
    CHECK(screen_guide_turn(2, 8) == 1 && screen_guide_turn(2, 'h') == 1);
    CHECK(screen_guide_turn(2, '1') == 0 && screen_guide_turn(0, '2') == 1 && screen_guide_turn(0, '3') == 2 &&
          screen_guide_turn(0, '4') == 3);
    CHECK(screen_guide_turn(1, '5') == 1 && screen_guide_turn(1, '0') == 1 && screen_guide_turn(1, '?') == 1 &&
          screen_guide_turn(1, 13) == 1 && screen_guide_turn(1, 'j') == 1);
    /* The text fits its panel (R11): 29 lines of 40 characters, only INK codes and \n, no trailing spaces */
    {
        const unsigned char *t;
        uint8_t p;
        int lines;
        int col;
        int ok = 1;

        for (p = 0; p < SCREEN_GUIDE_PAGES; p++) {
            lines = 0;
            col = 0;
            for (t = (const unsigned char *)screen_guide_pages[p].text; *t != '\0'; t++) {
                if (*t == NGE_CTRL_INK) {
                    t++;
                    ok = ok && (*t == NGE_INK_WHITE || *t == NGE_INK_YELLOW || *t == NGE_INK_CYAN);
                } else if (*t == '\n') {
                    ok = ok && (col == 0 || t[-1] != ' ');
                    lines++;
                    col = 0;
                } else {
                    ok = ok && *t >= ' ' && *t < 127;
                    col++;
                    ok = ok && col <= 40;
                }
            }
            ok = ok && (col == 0 || t[-1] != ' ');
            lines += col != 0;   /* the last line, with no \n */
            ok = ok && lines <= 29 && lines > 1;
        }
        CHECK(ok);
    }
    /* ? (SYMBOL SHIFT + C) in play: page 1, no turn used, the main screen untouched */
    memcpy(main_before, test_layer2_buf[0], sizeof(main_before));
    fresh();
    press(EV(NGE_KEY_SPACE));
    player_turn = 1;
    player_command(NGE_KEY_C | (NGE_MOD_SYM << 8));
    CHECK(RAN_AND_CLOSED());
    CHECK(player_turn && nge_event_count() == 0);
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);
    CHECK(strstr(OVER_ROW(0), " Commands ") != NULL && INK(0, 16) == L2_YELLOW);
    CHECK(strcmp(seen_cols(1, 0, 37, 39), "1/4") == 0);
    CHECK(starts(LIST_ROW(0), "Moving") && INK(2, 0) == L2_YELLOW);
    CHECK(starts(LIST_ROW(1), " arrows/hjkl  move;") && INK(3, 1) == L2_CYAN && INK(3, 14) == L2_WHITE);
    CHECK(strcmp(HELP(), "Left/right:page 1-4:go to SPACE:back    ") == 0);
    CHECK(seen[1][0][0].paper == SCREEN_PAPER && seen[1][30][39].paper == SCREEN_PAPER);   /* rows 0-30 covered */
    /* Right: page 2, the last page's text cleared */
    fresh();
    press(EV(NGE_KEY_RIGHT));
    press(EV(NGE_KEY_SPACE));
    screen_guide_run();
    CHECK(RAN_AND_CLOSED());
    CHECK(strstr(OVER_ROW(0), " Screens and prompts ") != NULL && strcmp(seen_cols(1, 0, 37, 39), "2/4") == 0);
    CHECK(strcmp(LIST_ROW(0), "Inventory (i)                           ") == 0);
    /* Right, 4: page 4. A key that isn't the screen's (a) changes nothing */
    fresh();
    press(EV(NGE_KEY_RIGHT));
    press(EV(NGE_KEY_4));
    press(EV(NGE_KEY_A));
    press(EV(NGE_KEY_SPACE));
    screen_guide_run();
    CHECK(RAN_AND_CLOSED());
    CHECK(strstr(OVER_ROW(0), " Abilities and stats ") != NULL && strcmp(seen_cols(1, 0, 37, 39), "4/4") == 0);
    CHECK(starts(LIST_ROW(0), "Abilities"));
    /* Left from page 1: page 4; then 3 */
    fresh();
    press(EV(NGE_KEY_LEFT));
    press(EV(NGE_KEY_SPACE));
    screen_guide_run();
    CHECK(RAN_AND_CLOSED() && strcmp(seen_cols(1, 0, 37, 39), "4/4") == 0);
    fresh();
    press(EV(NGE_KEY_3));
    press(EV(NGE_KEY_SPACE));
    screen_guide_run();
    CHECK(RAN_AND_CLOSED() && strstr(OVER_ROW(0), " Goal ") != NULL && strcmp(seen_cols(1, 0, 37, 39), "3/4") == 0);
    CHECK(memcmp(main_before, test_layer2_buf[0], sizeof(main_before)) == 0);

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(screen_inv_run(ACT_COUNT + 1, &chosen, &used));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(screen_inv_run(ACT_MOVE, &chosen, &used));   /* not an item action */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(screen_close());                      /* none open */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(player_ask_direction(PLAYER_PROMPT_COUNT, NULL, NULL));
    CHECK(test_halt_count == 1);

    /* ---- character creation (last: each class shown empties the world) ---- */
    {
        char cname[PC_NAME_MAX + 1] = "";   /* empty: the class's default */
        uint8_t cls;

        /* ENTER at once: the class it opened on, its default name, the welcome. The panels clear the whole screen */
        for (y = 0; y <= 30; y++) {
            for (x = 0; x < 40; x++) {
                test_layer2_buf[SCREEN_BUFFER_OVERLAY][y][x].c = '#';   /* an earlier screen's text */
            }
        }
        fresh();
        press(EV(NGE_KEY_ENTER));
        cls = chargen_run(PC_CLASS_FIGHTER, cname);
        CHECK(RAN_AND_CLOSED() && cls == PC_CLASS_FIGHTER && pc_class == PC_CLASS_FIGHTER && actor_player == 0);
        CHECK(strcmp(OVER_ROW(0), "         CREATE YOUR CHARACTER          ") == 0 && INK(0, 9) == L2_YELLOW);
        CHECK(strstr(OVER_ROW(2), " Bran the Fighter ") != NULL);
        CHECK(strcmp(pc_name, "Bran") == 0 && cname[0] == '\0');
        for (y = 0; y <= 30; y++) {
            CHECK(strchr(OVER_ROW(y), '#') == NULL);
        }
        CHECK(strcmp(seen_cols(1, 4, 0, 11), "Class       ") == 0);
        CHECK(strcmp(seen_cols(1, 5, 0, 11), "> Fighter   ") == 0 && INK(5, 2) == L2_YELLOW);
        CHECK(strcmp(seen_cols(1, 6, 0, 11), "  Mage      ") == 0 && INK(6, 2) == L2_WHITE);
        CHECK(strcmp(seen_cols(1, 4, 13, 39), "HP     12    AC     14     ") == 0);
        CHECK(strcmp(seen_cols(1, 5, 13, 39), "MP   none    Speed normal  ") == 0);
        CHECK(strcmp(seen_cols(1, 7, 13, 39), "Str 16  +3   Int  9  -1    ") == 0);
        CHECK(strcmp(seen_cols(1, 9, 13, 39), "Con 14  +2   Cha 10  +0    ") == 0);
        CHECK(strcmp(seen_cols(1, 11, 13, 39), "Spells none                ") == 0);
        CHECK(starts(seen_cols(1, 13, 13, 39), "Kit (cyan: in use)"));
        /* In use, in slot order: weapon, shield, body */
        CHECK(starts(seen_cols(1, 14, 13, 39), " short sword") && INK(14, 14) == L2_CYAN);
        CHECK(starts(seen_cols(1, 15, 13, 39), " iron shield"));
        CHECK(starts(seen_cols(1, 16, 13, 39), " leather armour"));
        CHECK(strcmp(seen_cols(1, 17, 13, 39), "                           ") == 0);
        CHECK(starts(HELP(), "Up/down:class N:change name ENTER:start"));
        CHECK(said("i inv, d drop, f fire, x look, Q quit."));

        /* Down: the Mage, frail, with mana, a spell, stones counted and a spell book */
        fresh();
        press(EV(NGE_KEY_DOWN));
        press(EV(NGE_KEY_ENTER));
        cls = chargen_run(PC_CLASS_FIGHTER, cname);
        CHECK(RAN_AND_CLOSED() && cls == PC_CLASS_MAGE && pc_class == PC_CLASS_MAGE);
        CHECK(strstr(OVER_ROW(2), " Elwyn the Mage ") != NULL);
        CHECK(strcmp(seen_cols(1, 5, 0, 11), "  Fighter   ") == 0 && strcmp(seen_cols(1, 6, 0, 11), "> Mage      ") == 0);
        CHECK(starts(seen_cols(1, 4, 13, 39), "HP      8"));
        CHECK(starts(seen_cols(1, 5, 13, 39), "MP      8"));
        CHECK(strcmp(seen_cols(1, 11, 13, 39), "Spells Magic Bolt          ") == 0);
        CHECK(starts(seen_cols(1, 14, 13, 39), " dagger"));
        CHECK(starts(seen_cols(1, 15, 13, 39), " sling"));
        CHECK(starts(seen_cols(1, 16, 13, 39), " pouch of stones ("));
        CHECK(starts(seen_cols(1, 17, 13, 39), " basic spellbook ") && INK(17, 14) != L2_CYAN);   /* carried */

        /* The ends: up from the first and down from the last stay put (j and k too) */
        fresh();
        press(EV(NGE_KEY_UP));
        press(EV(NGE_KEY_K));
        press(EV(NGE_KEY_ENTER));
        CHECK(chargen_run(PC_CLASS_FIGHTER, cname) == PC_CLASS_FIGHTER && RAN_AND_CLOSED());
        fresh();
        press(EV(NGE_KEY_DOWN));
        press(EV(NGE_KEY_J));
        press(EV(NGE_KEY_ENTER));
        CHECK(chargen_run((uint8_t)(CHARGEN_CLASSES - 2), cname) == CHARGEN_CLASSES - 1 && RAN_AND_CLOSED());

        /* N: a name typed, kept when the class changes (R2a), and in the caller's buffer */
        fresh();
        press(EV(NGE_KEY_N));
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_N));
        press(EV(NGE_KEY_A));
        press(EV(NGE_KEY_ENTER));
        press(EV(NGE_KEY_UP));
        press(EV(NGE_KEY_ENTER));
        cls = chargen_run(PC_CLASS_MAGE, cname);
        CHECK(RAN_AND_CLOSED() && cls == PC_CLASS_FIGHTER);
        CHECK(strcmp(cname, "ana") == 0 && strcmp(pc_name, "ana") == 0);
        CHECK(strstr(OVER_ROW(2), " ana the Fighter ") != NULL);
        CHECK(strcmp(seen_cols(1, 29, 0, 39), BLANK) == 0);   /* the prompt cleared */
        CHECK(starts(HELP(), "Up/down:class N:change name ENTER:start"));

        /* N, then ENTER alone: the name stays */
        fresh();
        press(EV(NGE_KEY_N));
        press(EV(NGE_KEY_ENTER));
        press(EV(NGE_KEY_ENTER));
        CHECK(chargen_run(PC_CLASS_MAGE, cname) == PC_CLASS_MAGE && RAN_AND_CLOSED());
        CHECK(strcmp(cname, "ana") == 0 && strcmp(pc_name, "ana") == 0);

#if GAME_DEV_KIT
        /* The Dev: more kit than rows, the last row says how many more */
        fresh();
        press(EV(NGE_KEY_ENTER));
        CHECK(chargen_run(PC_CLASS_DEV, cname) == PC_CLASS_DEV && RAN_AND_CLOSED());
        CHECK(strstr(OVER_ROW(2), " ana the Dev ") != NULL);
        CHECK(strcmp(seen_cols(1, 7, 13, 39), "Str 12  +1   Int 12  +1    ") == 0);
        CHECK(starts(seen_cols(1, 14, 13, 39), " sword"));
        CHECK(starts(seen_cols(1, 27, 13, 39), " and 5 more in the pack"));
#endif
        TEST_EXPECT_HALT(chargen_run(CHARGEN_CLASSES, cname));
    }

    return test_summary("ui");
}
