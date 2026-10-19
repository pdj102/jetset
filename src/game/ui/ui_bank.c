/*
 * The item screens: banked in page 116 with their literals and the table
 * of descriptions (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). They pass their literals only to common code (the
 * screen, the output path, messages).
 */
#ifdef __SDCC
#pragma codeseg PAGE_116
#pragma constseg PAGE_116
#endif

#include "game/ui/ui.h"

#include <string.h>

#include "core/nge_util.h"
#include "ecs/systems/nge_render.h"
#include "io/nge_print.h"
#include "io/nge_tm_stream.h"

#include "game/actors.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/game.h"
#include "game/gen/gen.h"
#include "game/screen.h"

#define PANEL_W     28   /* the map window, in tiles */
#define LIST_ROW    2    /* the first item's row: under the title and a blank row */
#define LIST_MAX    25   /* items in a list: the rows under the title (a to y) */
#define NAME_MAX    20   /* a kind's name, and its NUL */

/* Which items a list shows */
#define LIST_STOWED      0   /* in the pack, in ID order */
#define LIST_EQUIPPABLE  1   /* in the pack, fitting a slot the player has */
#define LIST_EQUIPPED    2   /* in use, in slot order */
#define LIST_ALL         3   /* the pack, then what is in use */
#define LIST_USABLE      4   /* in the pack, doing something when used */

/* An item screen */
typedef struct ui_item_screen {
    const char *title;
    const char *help;    /* the help bar while it is open */
    const char *empty;   /* when the list is empty */
    uint8_t     list;    /* LIST_* */
} ui_item_screen_t;

static const ui_item_screen_t screens[UI_SCREEN_COUNT] = {
    [UI_INVENTORY] = { "Worldly Possessions", "Any key: back", "You are carrying nothing.", LIST_ALL },
    [UI_DROP]      = { "Shed a Burden", "Letter: drop it   Other keys: back", "You have nothing to drop.",
                       LIST_STOWED },
    [UI_EQUIP]     = { "Arm Thyself", "Letter: equip it   Other keys: back", "You have nothing to equip.",
                       LIST_EQUIPPABLE },
    [UI_UNEQUIP]   = { "Divest", "Letter: take it off   Other keys: back", "You aren't using anything.",
                       LIST_EQUIPPED },
    [UI_USE]       = { "Partake", "Letter: use it   Other keys: back", "You have nothing to use.",
                       LIST_USABLE },
};

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
    [SLOT_RING_L] = " (left)",    /* short: "ring of health (right)" fits the window */
    [SLOT_RING_R] = " (right)",
    [SLOT_LIGHT]  = " (lit)",
};

/* An item in the pack belongs in list (LIST_STOWED, LIST_EQUIPPABLE or LIST_USABLE) */
static uint8_t listed(uint8_t list, nge_entity_id_t id)
{
    switch (list) {
    case LIST_EQUIPPABLE:
        return (gen_item_fits(kind_index[id]) & inv_slots[actor_player]) != 0;
    case LIST_USABLE:
        return gen_hook(id, GEN_HOOK_USE) != EF_NONE;
    default:
        return 1;
    }
}

/* The pack's items in list (LIST_STOWED, LIST_EQUIPPABLE or LIST_USABLE), appended to ids */
static uint8_t collect_stowed(uint8_t list, nge_entity_id_t *ids, uint8_t n)
{
    nge_entity_id_t id = NGE_ENTITY_INVALID;

    while (n < LIST_MAX && (id = inv_next(actor_player, id)) != NGE_ENTITY_INVALID) {
        if (listed(list, id)) {
            ids[n++] = id;
        }
    }
    return n;
}

/* The items in use, in slot order, appended to ids */
static uint8_t collect_equipped(nge_entity_id_t *ids, uint8_t n)
{
    nge_entity_id_t id;
    uint8_t slot;

    for (slot = 1; slot < SLOT_COUNT && n < LIST_MAX; slot++) {
        id = inv_equipped(actor_player, slot);
        if (id != NGE_ENTITY_INVALID) {
            ids[n++] = id;
        }
    }
    return n;
}

/* The items in a list (LIST_MAX at most), into ids. Returns how many */
static uint8_t collect(uint8_t list, nge_entity_id_t *ids)
{
    switch (list) {
    case LIST_EQUIPPED:
        return collect_equipped(ids, 0);
    case LIST_ALL:
        return collect_equipped(ids, collect_stowed(LIST_STOWED, ids, 0));
    default:
        return collect_stowed(list, ids, 0);
    }
}

/* Draw the screen: the title centred on the top row, then the list, lettered, one item a row */
static void draw(const ui_item_screen_t *s, const nge_entity_id_t *ids, uint8_t n)
{
    char name[NAME_MAX];
    uint8_t i;

    nge_tm_cls(screen_panel);
    nge_tm_at(0, (uint8_t)((PANEL_W - strlen(s->title)) / 2), screen_panel);
    nge_tm_ink(NGE_INK_YELLOW, screen_panel);
    nge_puts(s->title, screen_panel);

    nge_tm_ink(NGE_INK_WHITE, screen_panel);
    if (n == 0) {
        nge_tm_at(LIST_ROW, 1, screen_panel);
        nge_puts(s->empty, screen_panel);
    }
    for (i = 0; i < n; i++) {
        nge_tm_at((uint8_t)(LIST_ROW + i), 1, screen_panel);
        nge_putc((uint8_t)('a' + i), screen_panel);
        nge_puts(") ", screen_panel);
        gen_name(ids[i], name, sizeof(name));
        nge_puts(name, screen_panel);
        nge_puts(slot_words[item_slot[ids[i]]], screen_panel);
    }
}

/* Open a screen over the map window: no sprites over it, its list, its keys */
static void open_screen(const ui_item_screen_t *s, const nge_entity_id_t *ids, uint8_t n)
{
    nge_key_flush();   /* keys typed for play don't choose */
    nge_render_set_view(NGE_RENDER_NO_VIEW);
    draw(s, ids, n);
    screen_help(s->help);
}

/* Close it: the map, the sprites, the stats and play's help drawn again */
static void close_screen(void)
{
    game_play_redraw();
    nge_key_flush();   /* a key held down doesn't leak into play */
}

nge_entity_id_t ui_item_key(const nge_entity_id_t *ids, uint8_t n, nge_key_event_t ev) NGE_BANKED
{
    uint8_t c = (uint8_t)(nge_key_ascii(ev) - 'a');   /* any key but a to y wraps past the list */

    if (c >= n) {
        screen_message("Never mind.");
        return NGE_ENTITY_INVALID;
    }
    return ids[c];
}

nge_entity_id_t ui_choose_item(uint8_t screen) NGE_BANKED
{
    const ui_item_screen_t *s;
    nge_entity_id_t ids[LIST_MAX];
    nge_entity_id_t chosen;
    uint8_t n;

    nge_util_assert(screen < UI_SCREEN_COUNT && screen != UI_INVENTORY);
    s = &screens[screen];
    n = collect(s->list, ids);
    if (n == 0) {
        screen_message(s->empty);   /* nothing to choose: the screen isn't opened */
        return NGE_ENTITY_INVALID;
    }
    open_screen(s, ids, n);
    chosen = ui_item_key(ids, n, game_wait_key(0));
    close_screen();
    return chosen;
}

void ui_show_inventory(void) NGE_BANKED
{
    nge_entity_id_t ids[LIST_MAX];

    open_screen(&screens[UI_INVENTORY], ids, collect(screens[UI_INVENTORY].list, ids));
    game_wait_key(0);   /* any key: back */
    close_screen();
}
