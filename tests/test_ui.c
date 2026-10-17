/* game/ui: the item screens, drawn into the PC tests' tilemap memory. */
#include "test.h"

#include <string.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/game.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/messages/messages.h"
#include "game/player/player.h"
#include "game/screen.h"
#include "game/ui/ui.h"

#define AREA     NGE_OWNER_AREA(0)
#define NO_STATE 0xEE

/* What game.c and actors.c provide in the game (not linked here) */
game_state_t game_state;
nge_entity_id_t actor_player;
static uint8_t last_state;

void game_set_state(game_state_t state)
{
    last_state = (uint8_t)state;
}

static char text[NGE_TILEMAP_WIDTH + 1];

/* Row y of the map window (the screen panel, columns 0-27) */
static const char *panel(uint8_t y)
{
    test_tilemap_row(y, 0, 27, text);
    return text;
}

/* Row y of the whole screen */
static const char *row(uint8_t y)
{
    test_tilemap_row(y, 0, 39, text);
    return text;
}

/* Text starts with s */
static int starts(const char *t, const char *s)
{
    return strncmp(t, s, strlen(s)) == 0;
}

/* The last message: the message window (rows 27-30) fills from the top, then scrolls */
static int said(const char *s)
{
    int y;

    for (y = 30; y >= 27; y--) {
        if (row((uint8_t)y)[0] != ' ') {
            return starts(text, s);
        }
    }
    return 0;
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

    test_init();

    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    screen_init();
    actor_player = gen_creature(CR_PLAYER, AREA);
    nge_location_set(actor_player, 5, 5);
    nge_render_set_view(AREA);
    nge_event_flush();   /* the player's innate effect started */

    /* ---- the stats window: the player's stats, from the base record ---- */
    screen_draw_stats();
    player_show_stats();
    test_tilemap_row(2, 28, 39, text);
    CHECK(strcmp(text, "HP   12/ 12 ") == 0);
    test_tilemap_row(3, 28, 39, text);
    CHECK(strcmp(text, "AC       10 ") == 0);
    test_tilemap_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      16 ") == 0);
    test_tilemap_row(10, 28, 39, text);
    CHECK(strcmp(text, "Cha      10 ") == 0);
    test_tilemap_row(12, 28, 39, text);
    CHECK(strncmp(text, "Gold", 4) == 0);
    effect_apply(EF_STRENGTH, actor_player, NGE_ENTITY_INVALID);
    player_show_stats();
    test_tilemap_row(5, 28, 39, text);
    CHECK(strcmp(text, "Str      17 ") == 0);
    nge_event_flush();

    /* ---- the inventory, empty: titled, says so, any key goes back ---- */
    last_state = NO_STATE;
    ui_items_enter(UI_INVENTORY);
    CHECK(last_state == NO_STATE);                                 /* it stays open */
    CHECK(nge_render_area == NGE_RENDER_NO_VIEW);                  /* no sprites over it */
    CHECK(strcmp(panel(0), "    Worldly Possessions     ") == 0);  /* centred */
    CHECK(strcmp(panel(1), "                            ") == 0);
    CHECK(starts(panel(2), " You are carrying nothing."));
    CHECK(starts(row(31), "Any key: back"));
    ui_items_key(UI_INVENTORY, NGE_KEY_A);
    CHECK(last_state == GAME_PLAY && stowed(actor_player) == 0);

    /* ---- items: the pack in ID order, then what is in use, with its slot ---- */
    sword = gen_item(IT_SWORD, actor_player);
    ring1 = gen_item(IT_RING, actor_player);
    ring2 = gen_item(IT_RING, actor_player);
    item_slot[ring1] = SLOT_RING_L;
    ui_items_enter(UI_INVENTORY);
    CHECK(starts(panel(2), " a) sword "));
    CHECK(starts(panel(3), " b) ring of health "));
    CHECK(starts(panel(4), " c) ring of health (left)"));
    CHECK(strcmp(panel(5), "                            ") == 0);   /* the rest cleared */
    /* A key only leaves: nothing is done to the item its letter shows */
    last_state = NO_STATE;
    ui_items_key(UI_INVENTORY, NGE_KEY_A);
    CHECK(last_state == GAME_PLAY && stowed(sword) && nge_event_count() == 0);

    /* ---- drop: the pack only; a letter drops that item, on the player's tile ---- */
    last_state = NO_STATE;
    ui_items_enter(UI_DROP);
    CHECK(last_state == NO_STATE && starts(panel(0), "       Shed a Burden "));   /* (28 - 13) / 2 */
    CHECK(starts(panel(2), " a) sword") && starts(panel(3), " b) ring of health ") && starts(panel(4), "         "));
    CHECK(starts(row(31), "Letter: drop it   Other keys: back"));
    nge_timer_stop();
    ui_items_key(UI_DROP, NGE_KEY_B);
    CHECK(last_state == GAME_PLAY);
    CHECK(NGE_OWNER_OF(ring2) == AREA && NGE_LOCATION_X(ring2) == 5 && NGE_LOCATION_Y(ring2) == 5);
    CHECK(!nge_timer_stopped());                                   /* the turn is used */
    CHECK(nge_event_count() == 1);
    nge_event_flush();
    /* Another key: never mind, nothing done, the turn kept */
    nge_timer_stop();
    ui_items_enter(UI_DROP);
    ui_items_key(UI_DROP, NGE_KEY_Z);                              /* 'z': past the list */
    CHECK(last_state == GAME_PLAY && stowed(sword) && nge_timer_stopped());
    CHECK(said("Never mind."));
    ui_items_enter(UI_DROP);
    ui_items_key(UI_DROP, NGE_KEY_Q | (NGE_MOD_CAPS << 8));        /* 'Q': not a letter of the list */
    CHECK(stowed(sword) && nge_timer_stopped());
    ui_items_enter(UI_DROP);
    ui_items_key(UI_DROP, NGE_KEY_B);                              /* 'b': just past the list (a: sword) */
    CHECK(stowed(sword) && nge_timer_stopped() && said("Never mind."));
    /* A potion in the pack: dropped and listed, but nothing to equip */
    ring2 = gen_item(IT_POTION, actor_player);

    /* ---- equip: what fits a slot; take off: what is in use, in slot order ---- */
    ui_items_enter(UI_EQUIP);
    CHECK(starts(panel(0), "        Arm Thyself "));
    CHECK(starts(panel(2), " a) sword") && starts(panel(3), "     "));   /* ring1 in use, the potion fits nothing */
    ui_items_key(UI_EQUIP, NGE_KEY_A);
    CHECK(item_slot[sword] == SLOT_WEAPON && !nge_timer_stopped());
    nge_timer_stop();
    ui_items_enter(UI_UNEQUIP);
    CHECK(starts(panel(0), "           Divest"));
    CHECK(starts(panel(2), " a) sword (in hand)") && starts(panel(3), " b) ring of health (left)"));
    ui_items_key(UI_UNEQUIP, NGE_KEY_B);
    CHECK(item_slot[ring1] == SLOT_NONE && item_slot[sword] == SLOT_WEAPON);
    /* A refusal: told why, the turn kept */
    nge_timer_stop();
    inv_capacity[actor_player] = (uint8_t)inv_count(actor_player);
    ui_items_enter(UI_UNEQUIP);
    ui_items_key(UI_UNEQUIP, NGE_KEY_A);
    CHECK(item_slot[sword] == SLOT_WEAPON && nge_timer_stopped());
    CHECK(said("You can't carry any more."));
    inv_capacity[actor_player] = 20;
    nge_event_flush();

    /* ---- use: only what does something when used; a letter uses it up ---- */
    nge_timer_stop();
    last_state = NO_STATE;
    ui_items_enter(UI_USE);
    CHECK(last_state == NO_STATE && starts(panel(0), "          Partake "));   /* (28 - 7) / 2 */
    CHECK(starts(panel(2), " a) healing potion") && starts(panel(3), "     "));   /* not the ring */
    CHECK(starts(row(31), "Letter: use it   Other keys: back"));
    ui_items_key(UI_USE, NGE_KEY_A);
    CHECK(last_state == GAME_PLAY && NGE_ENTITY_STATE(ring2) == NGE_ENTITY_DYING && !nge_timer_stopped());
    nge_event_process(messages_on_event);
    CHECK(starts(row(29), "You drink the healing potion.") && said("You feel much better."));
    last_state = NO_STATE;
    ui_items_enter(UI_USE);
    CHECK(last_state == GAME_PLAY && said("You have nothing to use."));
    /* Food: eaten; the poison's start text */
    ring2 = gen_item(IT_FOOD, actor_player);
    ui_items_enter(UI_USE);
    ui_items_key(UI_USE, NGE_KEY_A);
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

    /* ---- a choosing screen with nothing to choose: says so, back to play at once ---- */
    nge_entity_destroy(ring1);
    nge_entity_destroy(ring2);                                     /* the potion */
    item_slot[sword] = SLOT_NONE;
    nge_entity_destroy(sword);
    nge_render_set_view(AREA);
    last_state = NO_STATE;
    ui_items_enter(UI_DROP);
    CHECK(last_state == GAME_PLAY && nge_render_area == AREA);    /* not opened */
    CHECK(said("You have nothing to drop."));
    last_state = NO_STATE;
    ui_items_enter(UI_UNEQUIP);
    CHECK(last_state == GAME_PLAY && said("You aren't using anything."));

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(ui_items_enter(UI_SCREENS));
    CHECK(test_halt_count == 1);

    return test_summary("ui");
}
