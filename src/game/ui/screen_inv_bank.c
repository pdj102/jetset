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
#include "ui/nge_panel.h"

#include "game/actors.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/messages/messages.h"
#include "game/ui/screen_internal.h"

static const nge_panel_t panels[SCREEN_INV_COUNT] = {
    [SCREEN_INV_TITLE]   = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 1 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [SCREEN_INV_LIST]    = { { .x1 = 0, .x2 = 27, .y1 = 2, .y2 = 28 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [SCREEN_INV_DETAILS] = { { .x1 = 28, .x2 = 39, .y1 = 2, .y2 = 28 }, SCREEN_PAPER, NGE_PANEL_SCROLL },
    [SCREEN_INV_STATUS]  = { { .x1 = 0, .x2 = 39, .y1 = 29, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

#define LIST_ROWS     27   /* the list panel's rows */
#define DETAILS_LAST  26   /* the details panel's last row: the pack's count */
#define NAME_MAX      20   /* a kind's name, and its NUL */
#define HELP_MAX      48   /* a help line built for an item: longer than the line, which clips it */

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
#define LIST_COUNT       5   /* the number of lists */

/* A key's answer, besides an action (ACT_*) and SCREEN_INV_BACK: stay open */
#define KEY_STAY  (ACT_COUNT + 1)

/* A mode: browse (SCREEN_INV_BROWSE), or choosing an item for an action */
typedef struct inv_mode {
    const char *title;
    const char *help;    /* the help line; NULL to build it for the selected item (browse) */
    const char *empty;   /* when the list is empty */
    uint8_t     list;    /* LIST_* */
} inv_mode_t;

static const inv_mode_t modes[ACT_COUNT + 1] = {
    [SCREEN_INV_BROWSE] = { "Worldly Possessions", NULL, "You are carrying nothing.", LIST_ALL },
    [ACT_DROP]    = { "Shed a Burden", "Letter/ENTER: drop it  SPACE: back", "You have nothing to drop.",
                      LIST_STOWED },
    [ACT_EQUIP]   = { "Arm Thyself", "Letter/ENTER: wear it  SPACE: back", "You have nothing to equip.",
                      LIST_EQUIPPABLE },
    [ACT_UNEQUIP] = { "Divest", "Letter/ENTER: take it off  SPACE: back", "You aren't using anything.",
                      LIST_EQUIPPED },
    [ACT_USE]     = { "Partake", "Letter/ENTER: use it  SPACE: back", "You have nothing to use.",
                      LIST_USABLE },
};

/* The browse keys of the actions, as the help line lists them, in this order */
static const uint8_t help_actions[4] = { ACT_EQUIP, ACT_UNEQUIP, ACT_DROP, ACT_USE };
static const char *const help_words[4] = { " w:wear", " r:off", " d:drop", " u:use" };

/* Where an equipped item is, after its name */
static const char *const slot_words[SLOT_COUNT] = {
    [SLOT_NONE]   = "",
    [SLOT_WEAPON] = " (in hand)",
    [SLOT_SHIELD] = " (on arm)",
    [SLOT_HEAD]   = " (on head)",
    [SLOT_BODY]   = " (worn)",
    [SLOT_HANDS]  = " (on hands)",
    [SLOT_FEET]   = " (on feet)",
    [SLOT_NECK]   = " (neck)",
    [SLOT_RING_L] = " (left)",    /* short: "ring of health (right)" fits the list */
    [SLOT_RING_R] = " (right)",
    [SLOT_LIGHT]  = " (lit)",
    [SLOT_RANGED] = " (ready)",
    [SLOT_AMMO]   = " (quiver)",
    [SLOT_ARMS]   = " (on arms)",
    [SLOT_LEGS]   = " (on legs)",
    [SLOT_CLOAK]  = " (on back)",
};

/* ---- The items ---- */

/* An item in the pack belongs in list (LIST_STOWED, LIST_EQUIPPABLE or LIST_USABLE) */
static uint8_t listed(uint8_t list, nge_entity_id_t id)
{
    switch (list) {
    case LIST_EQUIPPABLE:
        return (gen_item_fits(kind_index[id]) & inv_slots[actor_player]) != 0;
    case LIST_USABLE:
        return GEN_HOOK_EF(gen_hook_of(id, GEN_HOOK_USE)) != EF_NONE;
    default:
        return 1;
    }
}

/* The pack's items in list (LIST_STOWED, LIST_EQUIPPABLE or LIST_USABLE), appended to the list from n */
static uint8_t collect_stowed(uint8_t list, uint8_t n)
{
    nge_entity_id_t id = NGE_ENTITY_INVALID;

    while (n < SCREEN_INV_MAX && (id = inv_next(actor_player, id)) != NGE_ENTITY_INVALID) {
        if (listed(list, id)) {
            screen_inv_ids[n++] = id;
        }
    }
    return n;
}

/* The items in use, in slot order, appended to the list from n */
static uint8_t collect_equipped(uint8_t n)
{
    nge_entity_id_t id;
    uint8_t slot;

    for (slot = 1; slot < SLOT_COUNT && n < SCREEN_INV_MAX; slot++) {
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
        return collect_equipped(0);
    case LIST_ALL:
        return collect_stowed(LIST_STOWED, collect_equipped(0));
    default:
        return collect_stowed(list, 0);
    }
}

/* The selected item: the list isn't empty */
static nge_entity_id_t selected(void)
{
    return screen_inv_ids[screen_inv_list.cursor];
}

/*
 * The game allows action with the selected item: asked in check mode. If
 * not and say, the reason shows on the status line, in the words the
 * message window would use
 */
static uint8_t allowed(uint8_t action, uint8_t say)
{
    act_t a = { 0 };

    a.actor = actor_player;
    a.item = selected();
    a.kind = SLOT_NONE;   /* equip: the first free slot it fits */
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

/* Draw item i's row, if it is shown: lettered in choose mode, highlighted under the cursor */
static void draw_row(uint8_t mode, uint8_t i)
{
    NGE_FILE *s = screen_inv_streams[SCREEN_INV_LIST];
    char name[NAME_MAX];
    uint8_t top = screen_inv_list.top;
    nge_entity_id_t id = screen_inv_ids[i];

    if (i < top || i >= top + LIST_ROWS) {
        return;
    }
    nge_text_at((uint8_t)(i - top), 0, s);
    nge_text_paper((i == screen_inv_list.cursor) ? SCREEN_HIGHLIGHT : SCREEN_PAPER, s);
    nge_putc(' ', s);
    if (mode != SCREEN_INV_BROWSE) {
        nge_putc((uint8_t)('a' + i), s);
        nge_puts(") ", s);
    }
    gen_name(id, name, sizeof(name));
    nge_puts(name, s);
    nge_puts(slot_words[item_slot[id]], s);
    nge_text_clreol(s);   /* the rest of the row: a highlight bar, or what a longer name left */
    nge_text_paper(SCREEN_PAPER, s);
}

/* Draw every row shown */
static void draw_rows(uint8_t mode)
{
    uint8_t i;

    for (i = screen_inv_list.top; i < screen_inv_list.count; i++) {
        draw_row(mode, i);
    }
}

/* The details panel: the selected item's name, and the pack's count */
static void draw_details(void)
{
    NGE_FILE *s = screen_inv_streams[SCREEN_INV_DETAILS];
    char name[NAME_MAX];

    nge_text_cls(s);
    if (screen_inv_list.count != 0) {
        nge_text_ink(NGE_INK_YELLOW, s);
        gen_name(selected(), name, sizeof(name));
        nge_puts(name, s);
    }
    nge_text_at(DETAILS_LAST, 0, s);
    nge_text_ink(NGE_INK_CYAN, s);
    nge_puts("Pack ", s);
    nge_putu16w(inv_count(actor_player), 2, ' ', s);
    nge_putc('/', s);
    nge_putu16w(inv_capacity[actor_player], 2, ' ', s);
}

/* The help line: a choosing mode's own, or the actions allowed for the selected item */
static void show_help(uint8_t mode)
{
    char help[HELP_MAX];
    const char *own = modes[mode].help;
    uint8_t i;

    if (own != NULL) {
        screen_help(own);
        return;
    }
    strcpy(help, "jk:select");
    if (screen_inv_list.count != 0) {
        for (i = 0; i < sizeof(help_actions); i++) {
            if (allowed(help_actions[i], 0)) {
                strncat(help, help_words[i], sizeof(help) - 1 - strlen(help));
            }
        }
    }
    strncat(help, " SPACE:back", sizeof(help) - 1 - strlen(help));
    screen_help(help);   /* on the stack: common memory */
}

/* The cursor moved (NGE_LIST_*): draw what changed */
static void moved(uint8_t mode, uint8_t old, uint8_t change)
{
    if (change == NGE_LIST_NONE) {
        return;
    }
    if (change == NGE_LIST_SCROLLED) {
        nge_text_cls(screen_inv_streams[SCREEN_INV_LIST]);
        draw_rows(mode);
    } else {
        draw_row(mode, old);
        draw_row(mode, screen_inv_list.cursor);
    }
    nge_text_cls(screen_inv_streams[SCREEN_INV_STATUS]);   /* a refusal was about the old item */
    draw_details();
    show_help(mode);
}

/* Draw the whole screen */
static void draw(uint8_t mode)
{
    NGE_FILE *s = screen_inv_streams[SCREEN_INV_LIST];

    nge_text_ink(NGE_INK_YELLOW, screen_inv_streams[SCREEN_INV_TITLE]);
    nge_panel_centre(&panels[SCREEN_INV_TITLE], 0, modes[mode].title, screen_inv_streams[SCREEN_INV_TITLE]);
    nge_text_ink(NGE_INK_YELLOW, screen_inv_streams[SCREEN_INV_STATUS]);
    if (screen_inv_list.count == 0) {
        nge_text_at(0, 1, s);
        nge_puts(modes[mode].empty, s);
    }
    draw_rows(mode);
    draw_details();
    show_help(mode);
}

/* ---- Keys ---- */

/* Try action with the selected item: the action if the game allows it, or stay (told why) */
static uint8_t try(uint8_t action)
{
    if (screen_inv_list.count == 0 || !allowed(action, 1)) {
        return KEY_STAY;
    }
    return action;
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
    return ACT_EQUIP;
}

/* Move the cursor delta rows, and draw what changed */
static uint8_t move(uint8_t mode, int8_t delta)
{
    uint8_t old = screen_inv_list.cursor;

    moved(mode, old, nge_list_move(&screen_inv_list, delta));
    return KEY_STAY;
}

/* The screen's answer to one key event: an allowed action (ACT_*), SCREEN_INV_BACK, or KEY_STAY */
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
    case 'd':
        return try(ACT_DROP);
    case 'u':
        return try(ACT_USE);
    default:
        return KEY_STAY;
    }
}

uint8_t screen_inv_run(uint8_t mode, nge_entity_id_t *item) NGE_BANKED
{
    uint8_t answer;
    uint8_t ok;

    nge_util_assert(mode <= ACT_COUNT && modes[mode].title != NULL);

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
    } while (answer == KEY_STAY);
    if (answer != SCREEN_INV_BACK) {
        *item = selected();
    }
    nge_panel_close_all(screen_inv_streams, SCREEN_INV_COUNT);
    screen_close();
    return answer;
}
