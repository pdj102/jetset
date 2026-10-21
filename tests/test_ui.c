/* game/ui: the item screens, drawn into the PC tests' tilemap memory. */
#include "test.h"

#include <string.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "input/nge_input.h"
#include "layer2/nge_layer2.h"
#include "map/nge_map.h"

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
#include "game/screen.h"
#include "game/ui/ui.h"

#define AREA     NGE_OWNER_AREA(0)
#define EV(k)    ((nge_key_event_t)(k))

/* What game.c and actors.c provide in the game (not linked here) */
nge_entity_id_t actor_player;
static int waits;     /* game_wait_key() calls: a chooser waited for a key */

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

/* Layer 2 as it was while the last chooser waited: a closed screen clears its panel */
static test_l2_cell_t seen[32][40];

/* The key the player presses while a chooser waits: the next one set with press() */
static nge_key_event_t pressed = NGE_KEY_NONE;

static void press(nge_key_event_t ev)
{
    pressed = ev;
}

nge_key_event_t game_wait_key(uint16_t frames)
{
    nge_key_event_t ev = pressed;

    (void)frames;
    waits++;
    memcpy(seen, test_layer2, sizeof(seen));
    pressed = NGE_KEY_NONE;
    CHECK(ev != NGE_KEY_NONE);   /* a chooser waited with no key set up for it */
    return ev;
}

static char text[NGE_TILEMAP_WIDTH + 1];

/* Row y of the map window (the screen panel, columns 0-27) as it was while the last chooser waited */
static const char *panel(uint8_t y)
{
    uint8_t x;

    for (x = 0; x < 28; x++) {
        text[x] = (char)seen[y][x].c;
    }
    text[28] = '\0';
    return text;
}

/* Row y of the whole screen as it was while the last chooser waited */
static const char *seen_row(uint8_t y)
{
    uint8_t x;

    for (x = 0; x < 40; x++) {
        text[x] = (char)seen[y][x].c;
    }
    text[40] = '\0';
    return text;
}

/* Row y of the whole screen, now */
static const char *row(uint8_t y)
{
    test_layer2_row(y, 0, 39, text);
    return text;
}

/* The map window is covered (an opaque panel at its top-left), now or as it was while the last chooser waited */
#define COVERED_NOW()  (test_layer2[0][0].paper != NGE_LAYER2_TRANSPARENT)
#define COVERED_SEEN() (seen[0][0].paper != NGE_LAYER2_TRANSPARENT)

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
    nge_map_set_block(LEVEL_TILE_DOOR_SHUT, NGE_BLOCK_NONE);
    nge_map_set_block(LEVEL_TILE_DOOR_OPEN, NGE_BLOCK_NONE);
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

    /* ---- the inventory, empty: titled, says so, any key goes back ---- */
    waits = 0;
    CHECK(!COVERED_NOW());
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_I));
    CHECK(waits == 1 && COVERED_SEEN() && !COVERED_NOW());          /* covered while it waited, uncovered when closed */
    CHECK(nge_render_area == AREA);                                /* the sprites aren't hidden: the panel covers them */
    CHECK(strcmp(panel(0), "    Worldly Possessions     ") == 0);  /* centred */
    CHECK(strcmp(panel(1), "                            ") == 0);
    CHECK(starts(panel(2), " You are carrying nothing."));
    CHECK(starts(seen_row(31), "Any key: back"));
    CHECK(stowed(actor_player) == 0);

    /* ---- items: the pack in ID order, then what is in use, with its slot ---- */
    sword = gen_item(IT_SWORD, actor_player);
    ring1 = gen_item(IT_RING, actor_player);
    ring2 = gen_item(IT_RING, actor_player);
    item_slot[ring1] = SLOT_RING_L;
    press(EV(NGE_KEY_A));
    ui_show_inventory();
    CHECK(starts(panel(2), " a) sword "));
    CHECK(starts(panel(3), " b) ring of health "));
    CHECK(starts(panel(4), " c) ring of health (left)"));
    CHECK(strcmp(panel(5), "                            ") == 0);   /* the rest cleared */
    /* A key only leaves: nothing is done to the item its letter shows */
    CHECK(stowed(sword) && nge_event_count() == 0);

    /* ---- choosing: a letter gives its item, any other key nothing ---- */
    {
        nge_entity_id_t ids[2];

        ids[0] = sword;
        ids[1] = ring2;
        CHECK(ui_item_key(ids, 2, EV(NGE_KEY_A)) == sword);
        CHECK(ui_item_key(ids, 2, EV(NGE_KEY_B)) == ring2);
        CHECK(ui_item_key(ids, 2, EV(NGE_KEY_C)) == NGE_ENTITY_INVALID && said("Never mind."));
        CHECK(ui_item_key(ids, 2, EV(NGE_KEY_ENTER)) == NGE_ENTITY_INVALID);
        CHECK(ui_item_key(ids, 2, EV(NGE_KEY_A | (NGE_MOD_CAPS << 8))) == NGE_ENTITY_INVALID);   /* 'A' */
    }
    press(EV(NGE_KEY_B));
    CHECK(ui_choose_item(UI_DROP) == ring2 && nge_event_count() == 0);   /* chosen, not dropped */

    /* ---- drop: the pack only; d and a letter drops that item, on the player's tile ---- */
    player_turn = 1;
    press(EV(NGE_KEY_B));
    player_command(EV(NGE_KEY_D));
    CHECK(!COVERED_NOW() && starts(panel(0), "       Shed a Burden "));   /* (28 - 13) / 2 */
    CHECK(starts(panel(2), " a) sword") && starts(panel(3), " b) ring of health ") && starts(panel(4), "         "));
    CHECK(starts(seen_row(31), "Letter: drop it   Other keys: back"));
    CHECK(NGE_OWNER_OF(ring2) == AREA && NGE_LOCATION_X(ring2) == 5 && NGE_LOCATION_Y(ring2) == 5);
    CHECK(!player_turn);                                   /* the turn is used */
    CHECK(nge_event_count() == 1);
    nge_event_flush();
    /* Another key: never mind, nothing done, the turn kept */
    player_turn = 1;
    press(EV(NGE_KEY_Z));                                          /* 'z': past the list */
    player_command(EV(NGE_KEY_D));
    CHECK(stowed(sword) && player_turn && said("Never mind."));
    press(EV(NGE_KEY_Q | (NGE_MOD_CAPS << 8)));                     /* 'Q': not a letter of the list */
    player_command(EV(NGE_KEY_D));
    CHECK(stowed(sword) && player_turn);
    press(EV(NGE_KEY_B));                                          /* 'b': just past the list (a: sword) */
    player_command(EV(NGE_KEY_D));
    CHECK(stowed(sword) && player_turn && said("Never mind."));
    /* A potion in the pack: dropped and listed, but nothing to equip */
    ring2 = gen_item(IT_POTION, actor_player);

    /* ---- equip: what fits a slot; take off: what is in use, in slot order ---- */
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_W));
    CHECK(starts(panel(0), "        Arm Thyself "));
    CHECK(starts(panel(2), " a) sword") && starts(panel(3), "     "));   /* ring1 in use, the potion fits nothing */
    CHECK(item_slot[sword] == SLOT_WEAPON && !player_turn);
    player_turn = 1;
    press(EV(NGE_KEY_B));
    player_command(EV(NGE_KEY_R));
    CHECK(starts(panel(0), "           Divest"));
    CHECK(starts(panel(2), " a) sword (in hand)") && starts(panel(3), " b) ring of health (left)"));
    CHECK(item_slot[ring1] == SLOT_NONE && item_slot[sword] == SLOT_WEAPON);
    /* A refusal: told why, the turn kept */
    player_turn = 1;
    inv_capacity[actor_player] = (uint8_t)inv_count(actor_player);
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_R));
    CHECK(item_slot[sword] == SLOT_WEAPON && player_turn);
    CHECK(said("You can't carry any more."));
    inv_capacity[actor_player] = 20;
    nge_event_flush();

    /* ---- use: only what does something when used; a letter uses it up ---- */
    player_turn = 1;
    press(EV(NGE_KEY_A));
    player_command(EV(NGE_KEY_U));
    CHECK(starts(panel(0), "          Partake "));   /* (28 - 7) / 2 */
    CHECK(starts(panel(2), " a) healing potion") && starts(panel(3), "     "));   /* not the ring */
    CHECK(starts(seen_row(31), "Letter: use it   Other keys: back"));
    CHECK(NGE_ENTITY_STATE(ring2) == NGE_ENTITY_DYING && !player_turn);
    nge_event_process(messages_on_event);
    CHECK(starts(row(29), "You drink the healing potion.") && said("You feel much better."));
    waits = 0;
    player_command(EV(NGE_KEY_U));
    CHECK(waits == 0 && said("You have nothing to use."));   /* not opened: no key waited for */
    /* Food: eaten; the poison's start text */
    ring2 = gen_item(IT_FOOD, actor_player);
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
    nge_entity_destroy(ring2);                                     /* the potion */
    item_slot[sword] = SLOT_NONE;
    nge_entity_destroy(sword);
    nge_render_set_view(AREA);
    waits = 0;
    CHECK(ui_choose_item(UI_DROP) == NGE_ENTITY_INVALID);
    CHECK(!COVERED_NOW() && waits == 0);                            /* not opened */
    CHECK(said("You have nothing to drop."));
    CHECK(ui_choose_item(UI_UNEQUIP) == NGE_ENTITY_INVALID && said("You aren't using anything."));

    /* ---- doors: bump to open, c and a direction to close ---- */
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
    CHECK(said("Close in which direction?") && starts(seen_row(31), "Direction: close   Other keys: back"));
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
    TEST_EXPECT_HALT(ui_choose_item(UI_SCREEN_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(ui_choose_item(UI_INVENTORY));   /* it only shows */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(player_ask_direction(PLAYER_PROMPT_COUNT, NULL, NULL));
    CHECK(test_halt_count == 1);

    return test_summary("ui");
}
