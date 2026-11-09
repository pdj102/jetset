/*
 * The inventory screen: banked in page 116 with the other screens and
 * their literals, its list in page 117 (screen_pdata.c). It passes its
 * literals only to common code (the screen, the output path, the panels).
 */
#ifdef __SDCC
#pragma codeseg PAGE_116
#pragma constseg PAGE_116
#endif

#include "game/ui/screen_inv.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "ui/nge_list.h"
#include "ecs/components/nge_location.h"
#include "ui/nge_panel.h"

#include "game/actors.h"
#include "game/components/attack.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/game.h"
#include "game/gen/gen.h"
#include "game/gen/gen_kinds.h"
#include "game/messages/messages.h"
#include "game/ui/screen_internal.h"

/* The status line scrolls: an action's words, one a line, the newest last */
static const nge_panel_t panels[SCREEN_INV_COUNT] = {
    [SCREEN_INV_TITLE]   = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 1 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [SCREEN_INV_LIST]    = { { .x1 = 0, .x2 = 39, .y1 = 2, .y2 = 28 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [SCREEN_INV_STATUS]  = { { .x1 = 0, .x2 = 39, .y1 = 29, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_SCROLL },
};

#define LIST_ROWS     27   /* the list panel's rows */
#define NAME_MAX      20   /* a kind's name, and its NUL */
#define HELP_MAX      48   /* a help line built for an item: longer than the line, which clips it */

/*
 * A row's columns (docs/ui_inventory.md, 5.1): the choose mode's letter,
 * the count, the name, the details. The longest name is 19 (test_gen
 * checks NAME_MAX), the longest details 14 ("1d10 [+10,+10]")
 */
#define COUNT_WIDTH   3    /* a count is a byte: 255 at most */
#define NAME_WIDTH    (NAME_MAX - 1)
#define PACK_COL      30   /* "Pack nn/mm" on the title row, right-aligned */

/* The inks of a row: what is in use stands out */
#define INK_IN_USE  NGE_INK_CYAN
#define INK_STOWED  NGE_INK_WHITE

/* Characters of the keys (nge_key_ascii()) */
#define CHAR_DOWN   10
#define CHAR_UP     11
#define CHAR_ENTER  13

/* Which items a list shows */
#define LIST_STOWED      0   /* in the pack, in ID order */
#define LIST_EQUIPPABLE  1   /* in the pack, fitting a slot the player has */
#define LIST_EQUIPPED    2   /* in use, in slot order */
#define LIST_ALL         3   /* what is in use, then the pack */
#define LIST_USABLE      4   /* in the pack, doing something when used */
#define LIST_AIMABLE     5   /* in the pack, with something to aim */
#define LIST_TOOLS       6   /* in the pack, a key or a lock pick */
#define LIST_COUNT       7   /* the number of lists */

/* A key's answer, besides an action (ACT_*) and SCREEN_INV_BACK: stay open */
#define KEY_STAY  (ACT_COUNT + 1)

/* A mode of the screen's own, besides browse and the actions': which of two worn items goes (R21). Not
   KEY_STAY: choosing in it answers the mode */
#define MODE_WHICH  (ACT_COUNT + 2)

/* which_slot()'s answer when the question is given up: no slot */
#define WHICH_BACK  SLOT_COUNT

/* A mode: browse (SCREEN_INV_BROWSE), or choosing an item for an action */
typedef struct inv_mode {
    const char *title;
    const char *help;    /* the help line; NULL to build it for the selected item (browse) */
    const char *empty;   /* when the list is empty */
    uint8_t     list;    /* LIST_* */
} inv_mode_t;

static const inv_mode_t modes[MODE_WHICH + 1] = {
    [SCREEN_INV_BROWSE] = { "Worldly Possessions", NULL, "You are carrying nothing.", LIST_ALL },
    [ACT_DROP]    = { "Shed a Burden", "Letter/ENTER: drop it  SPACE: back", "You have nothing to drop.",
                      LIST_STOWED },
    [ACT_EQUIP]   = { "Arm Thyself", "Letter/ENTER: wear it  SPACE: back", "You have nothing to equip.",
                      LIST_EQUIPPABLE },
    [ACT_UNEQUIP] = { "Divest", "Letter/ENTER: take it off  SPACE: back", "You aren't using anything.",
                      LIST_EQUIPPED },
    [ACT_USE]     = { "Partake", "Letter/ENTER: use it  SPACE: back", "You have nothing to use.",
                      LIST_USABLE },
    [ACT_AIM]     = { "Take Aim", "Letter/ENTER: aim it  SPACE: back", "You have nothing to aim.",
                      LIST_AIMABLE },
    [ACT_UNLOCK]  = { "Try a Lock", "Letter/ENTER: try it  SPACE: back", "You have no key or pick.", LIST_TOOLS },
    [ACT_EXCHANGE] = { "Exchange Thy Gear", "Letter/ENTER: wear it  SPACE: back", "You have nothing to equip.",
                       LIST_EQUIPPABLE },
    [MODE_WHICH]  = { "Which Shall Go?", "Letter/ENTER: take it off  SPACE: back", "", LIST_EQUIPPED },
};

/* The actions done in the screen in browse mode, which stays open (R1) */
#define BUNDLED(action)  ((action) == ACT_EQUIP || (action) == ACT_UNEQUIP || (action) == ACT_DROP || \
                          (action) == ACT_USE)

/* The browse keys of the actions, as the help line lists them, in this order */
typedef struct help_key {
    uint8_t     action;
    const char *words;
} help_key_t;

static const help_key_t help_keys[] = {
    { ACT_EQUIP, " w:wear" }, { ACT_EXCHANGE, " x:swap" }, { ACT_UNEQUIP, " r:off" },
    { ACT_DROP, " d:drop" },  { ACT_USE, " u:use" },       { ACT_AIM, " a:aim" },
};

/* ---- The items ---- */

/* An item in the pack belongs in list (LIST_STOWED, LIST_EQUIPPABLE or LIST_USABLE); slots: the player's */
static uint8_t listed(uint8_t list, nge_entity_id_t id, uint16_t slots)
{
    uint8_t subclass;

    switch (list) {
    case LIST_EQUIPPABLE:
        return (gen_item_fits(kind_index[id]) & slots) != 0;
    case LIST_USABLE:
        return GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_USE)) != EF_NONE;
    case LIST_AIMABLE:
        return GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_AIM)) != EF_NONE;
    case LIST_TOOLS:
        subclass = gen_item_subclass(kind_index[id]);
        return subclass == ITEM_KEY || subclass == ITEM_TOOL;
    default:
        return 1;
    }
}

/* The pack's items in list (LIST_STOWED, LIST_EQUIPPABLE or LIST_USABLE), appended to the list from n */
static uint8_t collect_stowed(uint8_t list, uint8_t n)
{
    nge_entity_id_t id = NGE_ENTITY_INVALID;
    uint16_t slots = inv_slots_of(actor_player);   /* once, not per item: a banked call */

    while (n < SCREEN_INV_MAX && (id = inv_next(actor_player, id)) != NGE_ENTITY_INVALID) {
        if (listed(list, id, slots)) {
            screen_inv_ids[n++] = id;
        }
    }
    return n;
}

/* The items in use in slots (a mask of SLOT_BIT()), in slot order, appended to the list from n */
static uint8_t collect_equipped(uint8_t n, uint16_t slots)
{
    nge_entity_id_t id;
    uint8_t slot;

    for (slot = 1; slot < SLOT_COUNT && n < SCREEN_INV_MAX; slot++) {
        if ((slots & SLOT_BIT(slot)) == 0) {
            continue;
        }
        id = inv_equipped(actor_player, slot);
        if (id != NGE_ENTITY_INVALID) {
            screen_inv_ids[n++] = id;
        }
    }
    return n;
}

/* The items in a list (SCREEN_INV_MAX at most), into screen_inv_ids. Returns how many */
static uint8_t collect(uint8_t list)
{
    switch (list) {
    case LIST_EQUIPPED:
        return collect_equipped(0, SLOTS_HUMANOID);
    case LIST_ALL:
        return collect_stowed(LIST_STOWED, collect_equipped(0, SLOTS_HUMANOID));
    default:
        return collect_stowed(list, 0);
    }
}

/* The selected item: the list isn't empty */
static nge_entity_id_t selected(void)
{
    return screen_inv_ids[screen_inv_list.cursor];
}

/* The slots an item fits that the player has */
static uint16_t fitting(nge_entity_id_t id)
{
    return gen_item_fits(kind_index[id]) & inv_slots_of(actor_player);
}

/*
 * The player must say which item an exchange replaces (R21): it is stowed,
 * and more than one item in use is in a slot it fits (two rings)
 */
static uint8_t must_ask(nge_entity_id_t id)
{
    uint16_t slots;
    uint8_t slot;
    uint8_t n = 0;

    if (item_slot[id] != SLOT_NONE) {
        return 0;   /* the exchange refuses it (R22) */
    }
    slots = fitting(id);
    for (slot = 1; slot < SLOT_COUNT; slot++) {
        if ((slots & SLOT_BIT(slot)) != 0 && inv_equipped(actor_player, slot) != NGE_ENTITY_INVALID) {
            n++;
        }
    }
    return n > 1;
}

/*
 * The game allows action with the selected item: asked in check mode. If
 * not and say, the reason shows on the status line, in the words the
 * message window would use
 */
static uint8_t allowed(uint8_t action, uint8_t say)
{
    act_t a = { 0 };

    if (action == ACT_UNLOCK) {
        return 1;   /* it needs a lock, which is chosen after the item */
    }
    a.actor = actor_player;
    a.item = selected();
    a.slot = SLOT_NONE;   /* equip: the first free slot it fits; exchange: the first taken */
    a.x = NGE_LOCATION_X(actor_player);   /* aim: its own tile asks about the item alone (act_aim()) */
    a.y = NGE_LOCATION_Y(actor_player);
    if (act_do(action, &a, ACT_CHECK) == ACT_SUCCESS) {
        return 1;
    }
    if (say) {
        screen_message_to(screen_inv_streams[SCREEN_INV_STATUS]);
        messages_act_failed(&a);
        screen_message_to(NULL);
    }
    return 0;
}

/* ---- Drawing ---- */

/* A signed bonus, with its sign: "+1", "-1", "+0" */
static void put_signed(int8_t v, NGE_FILE *s)
{
    if (v >= 0) {
        nge_putc('+', s);
    }
    nge_puti16(v, s);
}

/* A count and its unit: "8 charges", "1 charge" */
static void put_count(uint8_t n, const char *unit, NGE_FILE *s)
{
    nge_putu16(n, s);
    nge_puts(unit, s);
    if (n != 1) {
        nge_putc('s', s);
    }
}

/*
 * An item's details, after its name (docs/ui_inventory.md, 5.2): an
 * attack's dice and bonuses, ammo's bonuses, armour's AC, a count's
 * meaning; nothing for the rest. The kind's own numbers (R11)
 */
static void describe(nge_entity_id_t id, NGE_FILE *s)
{
    attack_kind_t k;
    uint8_t kind = kind_index[id];
    uint8_t subclass = gen_item_subclass(kind);
    uint16_t hook;

    switch (subclass) {
    case ITEM_WEAPON:
    case ITEM_RANGED:
    case ITEM_AMMO:
        gen_item_attack(kind, &k);   /* zeros for plain ammo, with no attack part */
        if (k.dice != 0) {
            nge_putu16(k.dice, s);
            nge_putc('d', s);
            nge_putu16(k.sides, s);
            nge_putc(' ', s);
        } else if (subclass != ITEM_AMMO) {
            return;
        }
        nge_putc('[', s);
        put_signed(k.to_hit, s);
        nge_putc(',', s);
        put_signed(k.bonus, s);
        nge_putc(']', s);
        return;
    case ITEM_WAND:
        put_count(item_count[id], " charge", s);
        return;
    case ITEM_LIGHT:
        put_count(item_count[id], " turn", s);
        return;
    case ITEM_TOOL:
        put_count(item_count[id], " use", s);
        return;
    default:
        break;
    }
    hook = gen_hook_of(id, GEN_HOOK_WORN);
    if (GEN_HOOK_EF(hook) == EF_WORN_AC) {
        nge_puts("AC ", s);
        nge_puti16(GEN_HOOK_POWER(hook), s);
    }
}

/* How many of an item there are: a stack's count, 1 for anything else (Q2) */
static uint8_t how_many(nge_entity_id_t id)
{
    uint8_t subclass = gen_item_subclass(kind_index[id]);

    return (subclass == ITEM_AMMO || subclass == ITEM_TREASURE) ? item_count[id] : 1;
}

/* Draw item i's row, if it is shown: lettered in choose mode, highlighted under the cursor */
static void draw_row(uint8_t mode, uint8_t i)
{
    NGE_FILE *s = screen_inv_streams[SCREEN_INV_LIST];
    char name[NAME_MAX];
    uint8_t top = screen_inv_list.top;
    nge_entity_id_t id = screen_inv_ids[i];
    uint8_t n;

    if (i < top || i >= top + LIST_ROWS) {
        return;
    }
    nge_text_at((uint8_t)(i - top), 0, s);
    nge_text_paper((i == screen_inv_list.cursor) ? SCREEN_HIGHLIGHT : SCREEN_PAPER, s);
    nge_text_ink((item_slot[id] != SLOT_NONE) ? INK_IN_USE : INK_STOWED, s);
    if (mode != SCREEN_INV_BROWSE) {
        nge_putc((uint8_t)('a' + i), s);
        nge_putc(')', s);
    } else {
        nge_puts("  ", s);
    }
    nge_putu16w(how_many(id), COUNT_WIDTH, ' ', s);
    nge_putc(' ', s);
    gen_name(id, name, sizeof(name));
    nge_puts(name, s);
    for (n = (uint8_t)strlen(name); n <= NAME_WIDTH; n++) {
        nge_putc(' ', s);   /* to the details' column, and the space before it */
    }
    describe(id, s);
    nge_text_clreol(s);   /* the rest of the row: a highlight bar, or what a longer row left */
    nge_text_paper(SCREEN_PAPER, s);
}

/* Draw every row shown, or say the list is empty */
static void draw_rows(uint8_t mode)
{
    NGE_FILE *s = screen_inv_streams[SCREEN_INV_LIST];
    uint8_t i;

    nge_text_cls(s);
    if (screen_inv_list.count == 0) {
        nge_text_at(0, 1, s);
        nge_text_ink(INK_STOWED, s);
        nge_puts(modes[mode].empty, s);
        return;
    }
    for (i = screen_inv_list.top; i < screen_inv_list.count; i++) {
        draw_row(mode, i);
    }
}

/* The pack's count, on the title row */
static void draw_pack(void)
{
    NGE_FILE *s = screen_inv_streams[SCREEN_INV_TITLE];

    nge_text_at(0, PACK_COL, s);
    nge_text_ink(NGE_INK_CYAN, s);
    nge_puts("Pack ", s);
    nge_putu16w(inv_count(actor_player), 2, ' ', s);
    nge_putc('/', s);
    nge_putu16w(inv_capacity_of(actor_player), 2, ' ', s);
}

/* The help line: a choosing mode's own, or the actions allowed for the selected item */
static void show_help(uint8_t mode)
{
    char help[HELP_MAX];
    const char *own = modes[mode].help;
    const help_key_t *k;
    uint8_t action;
    uint8_t wear = 0;

    if (own != NULL) {
        screen_help(own);
        return;
    }
    strcpy(help, "jk:select");
    if (screen_inv_list.count != 0) {
        /* By pointer: an index into a banked table makes z80asm warn "integer range" (pc_screen.md) */
        for (k = help_keys; k != help_keys + sizeof(help_keys) / sizeof(help_keys[0]); k += 1) {
            action = k->action;
            if (action == ACT_EXCHANGE && wear) {
                continue;   /* on a free slot x does what w does: the line has no room for both (R24) */
            }
            if (allowed(action, 0)) {
                strncat(help, k->words, sizeof(help) - 1 - strlen(help));
                if (action == ACT_EQUIP) {
                    wear = 1;
                }
            }
        }
    }
    strncat(help, screen_inv_used ? " SPACE:done" : " SPACE:back", sizeof(help) - 1 - strlen(help));
    screen_help(help);   /* on the stack: common memory */
}

/* The cursor moved (NGE_LIST_*): draw what changed */
static void moved(uint8_t mode, uint8_t old, uint8_t change)
{
    if (change == NGE_LIST_NONE) {
        return;
    }
    if (change == NGE_LIST_SCROLLED) {
        draw_rows(mode);
    } else {
        draw_row(mode, old);
        draw_row(mode, screen_inv_list.cursor);
    }
    nge_text_cls(screen_inv_streams[SCREEN_INV_STATUS]);   /* a refusal was about the old item */
    show_help(mode);
}

/* The title row: the mode's title and the pack's count */
static void draw_title(uint8_t mode)
{
    nge_text_cls(screen_inv_streams[SCREEN_INV_TITLE]);   /* the title before, maybe longer: the ring question's */
    nge_text_ink(NGE_INK_YELLOW, screen_inv_streams[SCREEN_INV_TITLE]);
    nge_panel_centre(&panels[SCREEN_INV_TITLE], 0, modes[mode].title, screen_inv_streams[SCREEN_INV_TITLE]);
    draw_pack();
}

/* Draw the whole screen, the status line apart: it keeps an action's words */
static void draw(uint8_t mode)
{
    draw_title(mode);
    nge_text_ink(NGE_INK_YELLOW, screen_inv_streams[SCREEN_INV_STATUS]);
    draw_rows(mode);
    show_help(mode);
}

/* ---- Actions done here (browse mode) ---- */

/*
 * Do action with item, in slot (equip, exchange; SLOT_NONE for the
 * action's choice): its words go to the message window and the status
 * line (R5), its events are handled at once (D2)
 */
static void act_here(uint8_t action, nge_entity_id_t item, uint8_t slot)
{
    NGE_FILE *status = screen_inv_streams[SCREEN_INV_STATUS];
    act_t a = { 0 };

    a.actor = actor_player;
    a.item = item;
    a.slot = slot;
    nge_text_cls(status);
    screen_message_copy(status);
    if (act_do(action, &a, ACT_DO) == ACT_FAIL) {
        messages_act_failed(&a);
    }
    game_pause_events();
    screen_message_copy(NULL);
    if (a.used) {
        screen_inv_used = 1;
    }
}

/*
 * List mode's items again, the cursor on item if it is still listed, or
 * at place (D4), and redraw the rows, the pack's count and the help
 */
static void relist(uint8_t mode, nge_entity_id_t item, uint8_t place)
{
    uint8_t i;

    nge_list_init(&screen_inv_list, collect(modes[mode].list), LIST_ROWS);
    for (i = 0; i < screen_inv_list.count; i++) {
        if (screen_inv_ids[i] == item) {
            place = i;
            break;
        }
    }
    if (screen_inv_list.count != 0) {
        nge_list_set(&screen_inv_list, place);   /* clamped to the last */
    }
    draw_rows(mode);
    draw_pack();
    show_help(mode);
}

/* ---- Keys ---- */

/*
 * Try action with the selected item: the action if the game allows it, or
 * stay (told why). The ring question's choice, and an exchange that asks
 * it, are checked once the slot is known: a cursed left ring mustn't stop
 * the right one going
 */
static uint8_t try(uint8_t action)
{
    if (screen_inv_list.count == 0) {
        return KEY_STAY;
    }
    if (action == MODE_WHICH || (action == ACT_EXCHANGE && must_ask(selected()))) {
        return action;
    }
    return allowed(action, 1) ? action : KEY_STAY;
}

/* What ENTER does with the selected item in browse mode */
static uint8_t obvious_action(void)
{
    nge_entity_id_t id = selected();

    if (item_slot[id] != SLOT_NONE) {
        return ACT_UNEQUIP;
    }
    if (GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_USE)) != EF_NONE) {
        return ACT_USE;
    }
    if (GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_AIM)) != EF_NONE) {
        return ACT_AIM;
    }
    return ACT_EQUIP;
}

/* Move the cursor delta rows, and draw what changed */
static uint8_t move(uint8_t mode, int8_t delta)
{
    uint8_t old = screen_inv_list.cursor;

    moved(mode, old, nge_list_move(&screen_inv_list, delta));
    return KEY_STAY;
}

/*
 * The screen's answer to one key event: an allowed action (ACT_*),
 * SCREEN_INV_BACK, or KEY_STAY. The keys are described in the help screen (game/ui/screen_guide_pdata.c)
 */
static uint8_t key(uint8_t mode, nge_key_event_t ev)
{
    uint8_t c = nge_key_ascii(ev);
    uint8_t old;

    switch (c) {
    case CHAR_UP:
        return move(mode, -1);
    case CHAR_DOWN:
        return move(mode, 1);
    case ' ':
        return SCREEN_INV_BACK;
    case CHAR_ENTER:
        if (screen_inv_list.count == 0) {
            return KEY_STAY;
        }
        return try((mode == SCREEN_INV_BROWSE) ? obvious_action() : mode);
    default:
        break;
    }

    if (mode != SCREEN_INV_BROWSE) {
        /* Choose mode: a letter of the list chooses its item */
        if (c >= 'a' && c < 'a' + screen_inv_list.count) {
            old = screen_inv_list.cursor;
            moved(mode, old, nge_list_set(&screen_inv_list, (uint8_t)(c - 'a')));
            return try(mode);
        }
        return KEY_STAY;
    }

    switch (c) {
    case 'k':
        return move(mode, -1);
    case 'j':
        return move(mode, 1);
    case 'w':
        return try(ACT_EQUIP);
    case 'r':
        return try(ACT_UNEQUIP);
    case 'x':
        return try(ACT_EXCHANGE);
    case 'd':
        return try(ACT_DROP);
    case 'u':
        return try(ACT_USE);
    case 'a':
        return try(ACT_AIM);
    default:
        return KEY_STAY;
    }
}

/*
 * The slot an exchange of item replaces: SLOT_NONE for the action's choice,
 * or the slot of the item in use the player chooses, when more than one
 * could go (R21); WHICH_BACK if the player gives the question up
 */
static uint8_t which_slot(nge_entity_id_t item)
{
    uint8_t answer;

    if (!must_ask(item)) {
        return SLOT_NONE;
    }
    nge_list_init(&screen_inv_list, collect_equipped(0, fitting(item)), LIST_ROWS);
    nge_text_cls(screen_inv_streams[SCREEN_INV_STATUS]);
    draw(MODE_WHICH);
    do {
        answer = key(MODE_WHICH, screen_wait_key());
    } while (answer == KEY_STAY);
    return (answer == SCREEN_INV_BACK) ? WHICH_BACK : item_slot[selected()];
}

/*
 * Exchange the selected item, asking which goes if it must, in mode
 * (browse, or choose for ACT_EXCHANGE). Done in the screen in both modes,
 * as choose mode can't return the slot chosen: in choose mode it then
 * closes, as a letter finishes the command (R23). Returns KEY_STAY or
 * SCREEN_INV_BACK
 */
static uint8_t exchange(uint8_t mode)
{
    nge_entity_id_t item = selected();
    uint8_t place = screen_inv_list.cursor;
    uint8_t slot = which_slot(item);
    uint8_t asked = slot != SLOT_NONE;

    if (slot != WHICH_BACK) {
        act_here(ACT_EXCHANGE, item, slot);
        if (mode != SCREEN_INV_BROWSE || game_is_over()) {
            return SCREEN_INV_BACK;
        }
    }
    if (asked) {
        draw_title(mode);   /* the question's title; relist() draws the rest */
    }
    relist(mode, item, place);
    return KEY_STAY;
}

uint8_t screen_inv_run(uint8_t mode, nge_entity_id_t *item, uint8_t *used) NGE_BANKED
{
    uint8_t answer;
    uint8_t ok;

    nge_util_assert(mode <= ACT_COUNT && modes[mode].title != NULL);

    *used = 0;
    screen_inv_used = 0;
    nge_list_init(&screen_inv_list, collect(modes[mode].list), LIST_ROWS);
    if (screen_inv_list.count == 0 && mode != SCREEN_INV_BROWSE) {
        screen_message(modes[mode].empty);   /* nothing to choose: the screen isn't opened */
        return SCREEN_INV_BACK;
    }

    screen_open();
    ok = nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, SCREEN_INV_COUNT, screen_inv_streams);
    nge_util_assert(ok);
    draw(mode);
    do {
        answer = key(mode, screen_wait_key());
        if (answer == ACT_EXCHANGE) {
            answer = exchange(mode);
        } else if (mode == SCREEN_INV_BROWSE && BUNDLED(answer)) {
            nge_entity_id_t item = selected();

            act_here(answer, item, SLOT_NONE);
            if (game_is_over()) {
                answer = SCREEN_INV_BACK;   /* the player died: close (R3a) */
            } else {
                relist(SCREEN_INV_BROWSE, item, screen_inv_list.cursor);
                answer = KEY_STAY;
            }
        }
    } while (answer == KEY_STAY);
    if (answer != SCREEN_INV_BACK) {
        *item = selected();
    }
    *used = screen_inv_used;
    nge_panel_close_all(screen_inv_streams, SCREEN_INV_COUNT);
    screen_close();
    return answer;
}
