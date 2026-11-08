/*
 * The creation screen (docs/character_creation.md, 5.2): banked in page 140
 * with the class table (chargen_internal.h), which it reads directly. Its
 * literals go only to common code (the output path, the panels, the help
 * line and the messages: N-BANK-5).
 */
#ifdef __SDCC
#pragma codeseg PAGE_140
#pragma constseg PAGE_140
#endif

#include "game/chargen/chargen.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"
#include "io/nge_kbd_stream.h"
#include "io/nge_line.h"
#include "io/nge_print.h"
#include "io/nge_stream.h"
#include "io/nge_text.h"
#include "ui/nge_panel.h"

#include "game/actors.h"
#include "game/chargen/chargen_internal.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/gen/gen.h"
#include "game/spells/spells.h"
#include "game/ui/screen.h"
#include "game/ui/screen_main.h"

/* The screen's panels, by PANEL_*: together they cover rows 0-30, so nothing of an earlier screen shows */
#define PANEL_HEAD    0   /* the heading and "<name> the <class>" */
#define PANEL_LIST    1   /* the classes, with the cursor */
#define PANEL_BODY    2   /* the chosen class's character */
#define PANEL_PROMPT  3   /* the name prompt, while it is typed */
#define PANEL_COUNT   4

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_HEAD]   = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 3 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_LIST]   = { { .x1 = 0, .x2 = 12, .y1 = 4, .y2 = 27 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_BODY]   = { { .x1 = 13, .x2 = 39, .y1 = 4, .y2 = 27 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_PROMPT] = { { .x1 = 0, .x2 = 39, .y1 = 28, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

/* The head panel's rows */
#define ROW_HEADING  0
#define ROW_TITLE    2

/* The body panel's rows and columns */
#define ROW_HP        0   /* HP, then AC */
#define ROW_MP        1   /* MP, then speed */
#define ROW_ABILITY   3   /* Str Dex Con, with Int Wis Cha beside them */
#define ROW_SPELLS    7
#define ROW_KIT       9   /* the heading; the items below it */
#define KIT_ROWS      14  /* the items' rows: to the panel's last */
#define COL_RIGHT     13  /* the second column: AC, speed, Int Wis Cha */

#define TITLE_MAX     (PC_NAME_MAX + 5 + CHARGEN_NAME_MAX + 1)   /* "<name> the Fighter", and its NUL */
#define ITEM_NAME_MAX 20                                         /* a kind's name, and its NUL (gen_name()) */
#define SPEED_NORMAL  10                                         /* ticks between turns (game/components/stats.h) */

/* Characters of the keys (nge_key_ascii()) */
#define CHAR_DOWN   10
#define CHAR_UP     11
#define CHAR_ENTER  13

static const char ability_names[ABIL_COUNT][4] = { "Str", "Dex", "Con", "Int", "Wis", "Cha" };

static const char help_keys[] = "Up/down:class N:change name ENTER:start";

/* ---- Drawing ---- */

/* At column x of row y: text, in ink */
static void puts_at(uint8_t y, uint8_t x, const char *text, uint8_t ink, NGE_FILE *s)
{
    nge_text_at(y, x, s);
    nge_text_ink(ink, s);
    nge_puts(text, s);
}

/* At column x of row y: a label, then n right-aligned in 3 columns */
static void label_at(uint8_t y, uint8_t x, const char *label, uint16_t n, NGE_FILE *s)
{
    puts_at(y, x, label, NGE_INK_WHITE, s);
    nge_putu16w(n, 3, ' ', s);
}

/* The heading, and "<name> the <class>" under it, centred */
static void draw_head(uint8_t cls, NGE_FILE *s)
{
    char title[TITLE_MAX];   /* on the stack: common memory, for the panel's code */

    strcpy(title, pc_name);   /* slot 0: PC_NAME_MAX characters at most */
    strcat(title, " the ");
    strcat(title, chargen_classes[cls].name);
    nge_text_cls(s);
    nge_text_ink(NGE_INK_YELLOW, s);
    nge_panel_centre(&panels[PANEL_HEAD], ROW_HEADING, "CREATE YOUR CHARACTER", s);
    nge_text_ink(NGE_INK_WHITE, s);
    nge_panel_centre(&panels[PANEL_HEAD], ROW_TITLE, title, s);
}

/* The classes offered, the chosen one in yellow with the cursor */
static void draw_list(uint8_t cls, NGE_FILE *s)
{
    uint8_t i;

    nge_text_cls(s);
    puts_at(0, 0, "Class", NGE_INK_YELLOW, s);
    for (i = 0; i < CHARGEN_CLASSES; i++) {
        puts_at((uint8_t)(1 + i), 0, (i == cls) ? "> " : "  ", NGE_INK_YELLOW, s);
        nge_text_ink((i == cls) ? NGE_INK_YELLOW : NGE_INK_WHITE, s);
        nge_puts(chargen_classes[i].name, s);
    }
}

/* HP, MP (or none), AC and speed; the abilities with their modifiers */
static void draw_attributes(NGE_FILE *s)
{
    nge_entity_id_t p = actor_player;
    uint8_t speed = stats_speed[p];
    uint8_t a;
    uint8_t score;
    int8_t m;

    label_at(ROW_HP, 0, "HP    ", health_hp_max[p], s);
    label_at(ROW_HP, COL_RIGHT, "AC    ", health_ac[p], s);
    if (MANA_IS(p)) {
        label_at(ROW_MP, 0, "MP    ", mana_mp_max[p], s);
    } else {
        puts_at(ROW_MP, 0, "MP   none", NGE_INK_WHITE, s);
    }
    puts_at(ROW_MP, COL_RIGHT, (speed < SPEED_NORMAL) ? "Speed fast" : (speed == SPEED_NORMAL) ? "Speed normal" : "Speed slow",
            NGE_INK_WHITE, s);

    for (a = 0; a < ABIL_COUNT; a++) {
        score = stats_ability[a][p];
        m = STATS_MOD(score);
        label_at((uint8_t)(ROW_ABILITY + a % 3), (a < 3) ? 0 : COL_RIGHT, ability_names[a], score, s);
        nge_puts((m < 0) ? "  -" : "  +", s);
        nge_putu16((uint16_t)((m < 0) ? -m : m), s);
    }
}

/* The spells known, one after another (clipped at the panel's edge), or none */
static void draw_spells(NGE_FILE *s)
{
    char name[SPELL_NAME_MAX + 1];
    uint8_t sp;
    uint8_t any = 0;

    puts_at(ROW_SPELLS, 0, "Spells ", NGE_INK_YELLOW, s);
    nge_text_ink(NGE_INK_WHITE, s);
    if (SPELLS_IS(actor_player)) {
        for (sp = SP_NONE + 1; sp < SPELL_COUNT; sp++) {
            if (spell_knows(actor_player, sp)) {
                spell_name(sp, name, sizeof(name));
                if (any) {
                    nge_puts(", ", s);
                }
                nge_puts(name, s);
                any = 1;
            }
        }
    }
    if (!any) {
        nge_puts("none", s);
    }
}

/* One kit row: the item's name and its count if more than one, cyan if it is in use */
static void draw_item(uint8_t row, nge_entity_id_t id, NGE_FILE *s)
{
    char name[ITEM_NAME_MAX];

    gen_name(id, name, sizeof(name));
    puts_at(row, 1, name, (item_slot[id] != SLOT_NONE) ? NGE_INK_CYAN : NGE_INK_WHITE, s);
    if (item_count[id] > 1) {
        nge_puts(" (", s);
        nge_putu16(item_count[id], s);
        nge_putc(')', s);
    }
}

/*
 * The kit: what is in use, in slot order, then the pack. When it is longer
 * than the rows, the last row says how many more are in the pack
 */
static void draw_kit(NGE_FILE *s)
{
    nge_entity_id_t p = actor_player;
    nge_entity_id_t id;
    uint8_t row = 0;
    uint8_t slot;
    uint8_t left;

    puts_at(ROW_KIT, 0, "Kit", NGE_INK_YELLOW, s);
    nge_puts(" (cyan: in use)", s);
    for (slot = SLOT_NONE + 1; slot < SLOT_COUNT; slot++) {
        id = inv_equipped(p, slot);
        if (id != NGE_ENTITY_INVALID && item_slot[id] == slot) {   /* a two-handed item once, in its own slot */
            draw_item((uint8_t)(ROW_KIT + 1 + row++), id, s);
        }
    }
    left = inv_count(p);
    id = NGE_ENTITY_INVALID;
    while (left != 0) {
        if (row == KIT_ROWS - 1 && left > 1) {
            puts_at((uint8_t)(ROW_KIT + 1 + row), 1, "and ", NGE_INK_WHITE, s);
            nge_putu16(left, s);
            nge_puts(" more in the pack", s);
            return;
        }
        id = inv_next(p, id);
        nge_util_assert(id != NGE_ENTITY_INVALID);
        draw_item((uint8_t)(ROW_KIT + 1 + row++), id, s);
        left--;
    }
}

static void draw_body(NGE_FILE *s)
{
    nge_text_cls(s);
    draw_attributes(s);
    draw_spells(s);
    draw_kit(s);
}

/* ---- The name ---- */

/*
 * Ask for the player's name on the prompt panel and read it from the
 * keyboard, running paused frames until ENTER: into name (the caller's,
 * common memory) and the player's, unless it was empty
 */
static void ask_name(NGE_FILE *prompt, char *name)
{
    char typed[PC_NAME_MAX + 1];   /* on the stack: common memory, as the line editor needs */
    NGE_FILE *keyboard = nge_open_keyboard();
    nge_line_t line;
    uint8_t done;

    nge_util_assert(keyboard != NULL);
    screen_help("Type a name, then ENTER");
    nge_text_at(1, 1, prompt);
    nge_text_ink(NGE_INK_YELLOW, prompt);
    nge_puts("What is your name? ", prompt);
    nge_text_ink(NGE_INK_WHITE, prompt);
    nge_line_begin(&line, typed, sizeof(typed), keyboard, prompt);
    do {
        screen_frame();
        done = nge_line_update(&line);
    } while (!done);
    nge_close(keyboard);
    nge_text_cls(prompt);
    screen_help(help_keys);

    if (typed[0] != '\0') {
        strcpy(name, typed);
        pc_set_name(actor_player, name);
    }
}

/* ---- The screen ---- */

uint8_t chargen_run(uint8_t cls, char *name) NGE_BANKED
{
    NGE_FILE *streams[PANEL_COUNT];
    uint8_t ok;
    uint8_t c;
    uint8_t want;

    nge_util_assert(cls < CHARGEN_CLASSES);
    nge_util_assert(name != NULL);

    screen_open();
    ok = nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, PANEL_COUNT, streams);
    nge_util_assert(ok);
    (void)ok;
    screen_help(help_keys);

    want = cls;
    cls = CHARGEN_CLASSES;   /* none made yet: the first pass makes one */
    for (;;) {
        if (want != cls) {
            cls = want;
            chargen_make(cls, name);
            draw_head(cls, streams[PANEL_HEAD]);
            draw_list(cls, streams[PANEL_LIST]);
            draw_body(streams[PANEL_BODY]);
        }
        c = nge_key_ascii(screen_wait_key());
        if (c == CHAR_ENTER) {
            break;
        }
        if ((c == CHAR_UP || c == 'k') && cls > 0) {
            want = (uint8_t)(cls - 1);
        } else if ((c == CHAR_DOWN || c == 'j') && cls + 1 < CHARGEN_CLASSES) {
            want = (uint8_t)(cls + 1);
        } else if (c == 'n' || c == 'N') {
            ask_name(streams[PANEL_PROMPT], name);
            draw_head(cls, streams[PANEL_HEAD]);
        }
    }

    nge_panel_close_all(streams, PANEL_COUNT);
    screen_close();

    screen_message("Welcome to the dungeon.");
    screen_message("Move: cursor keys or h j k l.");
    screen_message("i inv, d drop, f fire, x look, Q quit.");
    return cls;
}
