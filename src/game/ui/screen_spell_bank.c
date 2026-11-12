/*
 * The spell list, and the book screen as its second mode (docs/spells.md,
 * 10.5.5): banked in page 104 beside the PC screen, as page 116 is
 * full. It passes its literals only to common code (the help line, the
 * output path, the panels); what other pages fill in (a spell's name) goes
 * to buffers on the stack.
 */
#ifdef __SDCC
#pragma codeseg PAGE_104
#pragma constseg PAGE_104
#endif

#include "game/ui/screen_spell.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "ui/nge_list.h"
#include "ui/nge_panel.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/components/kind.h"
#include "game/components/mana.h"
#include "game/game.h"
#include "game/gen/gen.h"
#include "game/messages/messages.h"
#include "game/spells/spells.h"
#include "game/ui/screen_internal.h"

/* The screen's panels, by PANEL_* */
#define PANEL_TITLE   0
#define PANEL_LIST    1   /* the header, the spells, the MP left */
#define PANEL_STATUS  2   /* why a spell can't be cast or learned; what learning one said */
#define PANEL_COUNT   3

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_TITLE]  = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 1 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_LIST]   = { { .x1 = 0, .x2 = 39, .y1 = 2, .y2 = 28 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_STATUS] = { { .x1 = 0, .x2 = 39, .y1 = 29, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

/* The list panel: the header on row 0, a spell a row from row 1, the MP left (or a book's key and free slots) at the foot */
#define ROW_FIRST   1
#define ROW_KEY     23   /* a book: what the colours mean */
#define ROW_MP      24   /* the MP left; a book: the free slots */
#define NAME_MAX    20   /* a kind's name, and its NUL: a book's title */
#define COL_NAME    4
#define COL_LEVEL   20   /* width 5 */
#define COL_COST    26   /* width 3 */

/* Characters of the keys (nge_key_ascii()) */
#define CHAR_DOWN   10
#define CHAR_UP     11
#define CHAR_ENTER  13

/* A key's answer: chosen, back, or stay open */
#define KEY_CHOSEN  0
#define KEY_BACK    1
#define KEY_STAY    2

/* A row's state, from the action asked in check mode: white, cyan (a book's known spell) or red */
#define ROW_CAN    0
#define ROW_KNOWN  1
#define ROW_CANT   2

/* The screen's state, on the run's stack */
typedef struct spell_screen {
    NGE_FILE       *streams[PANEL_COUNT];
    uint8_t         spells[SPELL_COUNT];   /* the spells known, in SP_* order; or the book's, in its order */
    nge_list_t      list;
    nge_entity_id_t book;                  /* the book read; NGE_ENTITY_INVALID to cast */
    uint8_t         learned;               /* 1 once a spell has been learned from it */
} spell_screen_t;

typedef char spell_book_fits_check[(SPELL_BOOK_MAX <= SPELL_COUNT) ? 1 : -1];

/*
 * Spell sp's state: casting it asked in check mode on the player's own
 * tile, or learning it from the book (D21). If not ROW_CAN and say, why,
 * on the status line
 */
static uint8_t row_state(spell_screen_t *w, uint8_t sp, uint8_t say)
{
    act_t a = { 0 };

    a.actor = actor_player;
    a.spell = sp;
    if (w->book == NGE_ENTITY_INVALID) {
        a.x = NGE_LOCATION_X(actor_player);
        a.y = NGE_LOCATION_Y(actor_player);
        if (act_do(ACT_CAST, &a, ACT_CHECK) == ACT_SUCCESS || a.reason == ACT_R_NO_TARGET) {
            return ROW_CAN;   /* a bolt on its own tile: it can be cast, at a tile chosen next */
        }
    } else {
        a.item = w->book;
        if (act_do(ACT_LEARN, &a, ACT_CHECK) == ACT_SUCCESS) {
            return ROW_CAN;
        }
    }
    if (say) {
        screen_message_to(w->streams[PANEL_STATUS]);
        messages_act_failed(&a);
        screen_message_to(NULL);
    }
    return (a.reason == ACT_R_KNOWN) ? ROW_KNOWN : ROW_CANT;
}

/* Draw spell i's row: lettered, coloured by its state, highlighted under the cursor */
static void draw_row(spell_screen_t *w, uint8_t i)
{
    NGE_FILE *s = w->streams[PANEL_LIST];
    char name[SPELL_NAME_MAX + 1];
    spell_def_t def;
    uint8_t sp = w->spells[i];

    spell_get(sp, &def);
    spell_name(sp, name, sizeof(name));
    nge_text_at((uint8_t)(ROW_FIRST + i), 0, s);
    nge_text_paper((i == w->list.cursor) ? SCREEN_HIGHLIGHT : SCREEN_PAPER, s);
    switch (row_state(w, sp, 0)) {
    case ROW_CAN:
        nge_text_ink(NGE_INK_WHITE, s);
        break;
    case ROW_KNOWN:
        nge_text_ink(NGE_INK_CYAN, s);
        break;
    default:
        nge_text_ink(NGE_INK_RED, s);
        break;
    }
    nge_putc(' ', s);
    nge_putc((uint8_t)('a' + i), s);
    nge_puts(") ", s);
    nge_puts(name, s);
    nge_text_at((uint8_t)(ROW_FIRST + i), COL_LEVEL, s);
    nge_putu16w(def.level, 5, ' ', s);
    nge_text_at((uint8_t)(ROW_FIRST + i), COL_COST, s);
    nge_putu16w(spell_cost(def.level), 3, ' ', s);
    nge_text_clreol(s);   /* the rest of the row: a highlight bar */
    nge_text_paper(SCREEN_PAPER, s);
}

/* The foot: the MP left; or, for a book, what the colours mean and the free slots at each spell level */
static void draw_foot(spell_screen_t *w)
{
    NGE_FILE *s = w->streams[PANEL_LIST];
    uint8_t level;

    nge_text_at(ROW_MP, 1, s);
    nge_text_ink(NGE_INK_CYAN, s);
    if (w->book != NGE_ENTITY_INVALID) {
        nge_puts("Free slots ", s);
        for (level = 1; level <= SPELL_LEVEL_MAX; level++) {
            nge_text_ink(NGE_INK_CYAN, s);
            nge_puts(" L", s);
            nge_putc((uint8_t)('0' + level), s);
            nge_putc(':', s);
            nge_text_ink(NGE_INK_WHITE, s);
            nge_putu16w(spell_free(actor_player, level), 1, ' ', s);
        }
        nge_text_clreol(s);
        return;
    }
    nge_puts("MP ", s);
    nge_text_ink(NGE_INK_WHITE, s);
    if (MANA_IS(actor_player)) {
        nge_putu16w(mana_mp[actor_player], 1, ' ', s);
        nge_putc('/', s);
        nge_putu16w(mana_mp_max[actor_player], 1, ' ', s);
    } else {
        nge_puts("none", s);
    }
}

static void draw_rows(spell_screen_t *w)
{
    uint8_t n = w->list.count;   /* a local: zsdcc added the field's offset to HL again on each pass of the loop */
    uint8_t i;

    for (i = 0; i < n; i++) {
        draw_row(w, i);
    }
}

static void draw(spell_screen_t *w)
{
    NGE_FILE *s = w->streams[PANEL_LIST];
    char title[NAME_MAX];

    nge_text_ink(NGE_INK_YELLOW, w->streams[PANEL_TITLE]);
    if (w->book != NGE_ENTITY_INVALID) {
        gen_name(w->book, title, sizeof(title));
        nge_panel_centre(&panels[PANEL_TITLE], 0, title, w->streams[PANEL_TITLE]);
    } else {
        nge_panel_centre(&panels[PANEL_TITLE], 0, "Cast a Spell", w->streams[PANEL_TITLE]);
    }
    nge_text_ink(NGE_INK_YELLOW, w->streams[PANEL_STATUS]);
    nge_text_ink(NGE_INK_CYAN, s);
    nge_text_at(0, COL_NAME, s);
    nge_puts("Spell", s);
    nge_text_at(0, COL_LEVEL, s);
    nge_puts("Level", s);
    nge_text_at(0, COL_COST + 1, s);
    nge_puts("MP", s);
    draw_rows(w);
    if (w->book != NGE_ENTITY_INVALID) {
        nge_text_at(ROW_KEY, 1, s);
        nge_text_ink(NGE_INK_CYAN, s);
        nge_puts("cyan: known  ", s);
        nge_text_ink(NGE_INK_RED, s);
        nge_puts("red: can't learn yet", s);
        draw_foot(w);
        screen_help("Letter/ENTER: learn  SPACE: close");
        return;
    }
    draw_foot(w);
    screen_help("Letter/ENTER: cast  SPACE: back");
}

/* Put the cursor on spell i, drawing the rows that changed; the status line was about the old one */
static void put_cursor(spell_screen_t *w, uint8_t i)
{
    uint8_t old = w->list.cursor;

    if (nge_list_set(&w->list, i) != NGE_LIST_NONE) {
        draw_row(w, old);
        draw_row(w, w->list.cursor);
        nge_text_cls(w->streams[PANEL_STATUS]);
    }
}

/*
 * Learn spell sp from the book, in the screen, as the inventory does its
 * bundled actions: the words on the status line, the events handled, then
 * every row and the foot again, as one learn can fill a level's last slot
 */
static void learn(spell_screen_t *w, uint8_t sp)
{
    NGE_FILE *status = w->streams[PANEL_STATUS];
    act_t a = { 0 };

    a.actor = actor_player;
    a.item = w->book;
    a.spell = sp;
    nge_text_cls(status);
    screen_message_copy(status);
    if (act_do(ACT_LEARN, &a, ACT_DO) == ACT_FAIL) {
        messages_act_failed(&a);
    }
    game_pause_events();
    screen_message_copy(NULL);
    if (a.used) {
        w->learned = 1;   /* closing the book uses the turn (D22) */
    }
    draw_rows(w);
    draw_foot(w);
}

/* Try the selected spell: chosen if it can be cast; learned if it can be; or stay (told why) */
static uint8_t try(spell_screen_t *w)
{
    uint8_t sp = w->spells[w->list.cursor];

    if (row_state(w, sp, 1) != ROW_CAN) {
        return KEY_STAY;
    }
    if (w->book == NGE_ENTITY_INVALID) {
        return KEY_CHOSEN;
    }
    learn(w, sp);
    return KEY_STAY;   /* the book stays open: several spells a reading (R48) */
}

/* The list's keys: described in the help screen (game/ui/screen_guide_pdata.c) */
static uint8_t key(spell_screen_t *w, nge_key_event_t ev)
{
    uint8_t c = nge_key_ascii(ev);
    uint8_t cursor = w->list.cursor;

    switch (c) {
    case CHAR_UP:
    case 'k':
        if (cursor != 0) {
            put_cursor(w, (uint8_t)(cursor - 1));
        }
        return KEY_STAY;
    case CHAR_DOWN:
    case 'j':
        put_cursor(w, (uint8_t)(cursor + 1));   /* clamped to the last */
        return KEY_STAY;
    case ' ':
        return KEY_BACK;
    case CHAR_ENTER:
        return try(w);
    default:
        if (c >= 'a' && c < 'a' + w->list.count) {
            put_cursor(w, (uint8_t)(c - 'a'));
            return try(w);
        }
        return KEY_STAY;
    }
}

/* Show the screen for w's n spells until a choice or back: KEY_CHOSEN or KEY_BACK */
static uint8_t run(spell_screen_t *w, uint8_t n)
{
    uint8_t answer;
    uint8_t ok;

    nge_list_init(&w->list, n, n);
    screen_open();
    ok = nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, PANEL_COUNT, w->streams);
    nge_util_assert(ok);
    draw(w);
    do {
        answer = key(w, screen_wait_key());
    } while (answer == KEY_STAY);
    nge_panel_close_all(w->streams, PANEL_COUNT);
    screen_close();
    return answer;
}

uint8_t screen_spell_run(uint8_t *sp) NGE_BANKED
{
    spell_screen_t w;
    uint8_t n = 0;
    uint8_t s;
    uint8_t answer;

    for (s = 1; s < SPELL_COUNT; s++) {
        if (spell_knows(actor_player, s)) {
            w.spells[n++] = s;
        }
    }
    if (n == 0) {
        screen_message("You know no spells.");   /* nothing to choose: the screen isn't opened */
        return 0;
    }
    w.book = NGE_ENTITY_INVALID;
    w.learned = 0;
    answer = run(&w, n);
    *sp = w.spells[w.list.cursor];
    return answer == KEY_CHOSEN;
}

uint8_t screen_book_run(nge_entity_id_t book) NGE_BANKED
{
    spell_screen_t w;
    uint8_t n;

    nge_util_assert(book < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(book));
    n = spell_book(gen_item_book_spells(kind_index[book]), w.spells);   /* the book's order; SP_NONE after the last */
    nge_util_assert(n != 0);
    w.book = book;
    w.learned = 0;
    run(&w, n);
    return w.learned;
}
