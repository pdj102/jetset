/*
 * The player's commands, and the direction prompts: banked in page 114 with their
 * literals (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). They run when a key is pressed, not every frame,
 * and pass their literals only to common code (the screen and messages).
 */
#ifdef __SDCC
#pragma codeseg PAGE_114
#pragma constseg PAGE_114
#endif

#include "game/player/player.h"

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_timer.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/components/attrs.h"
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/dev.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/game.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/screen.h"
#include "game/target/target.h"
#include "game/ui/ui.h"

/* Characters of the movement keys (nge_key_ascii): cursor keys */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9
#define CHAR_DOWN   10
#define CHAR_UP     11

uint8_t player_key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy) NGE_BANKED
{
    *dx = 0;
    *dy = 0;
    switch (nge_key_ascii(ev)) {
    case CHAR_LEFT:
    case 'h':
        *dx = -1;
        return 1;
    case CHAR_RIGHT:
    case 'l':
        *dx = 1;
        return 1;
    case CHAR_UP:
    case 'k':
        *dy = -1;
        return 1;
    case CHAR_DOWN:
    case 'j':
        *dy = 1;
        return 1;
    default:
        return 0;
    }
}


void player_act(uint8_t action, act_t *a) NGE_BANKED
{
    a->actor = actor_player;
    if (act_do(action, a, ACT_DO) == ACT_FAIL) {
        messages_act_failed(a);
    }
    if (a->used) {
        player_turn = 0;   /* time passes again */
    }
}

/*
 * The move command, bump to attack and bump to open: a move blocked by
 * something that can be damaged attacks it instead, and one blocked by a
 * closed door opens it (act_move()'s check mode finds the blocker)
 */
static void move(int8_t dx, int8_t dy)
{
    act_t a = { 0 };
    nge_entity_id_t blocker;

    a.actor = actor_player;
    a.dx = dx;
    a.dy = dy;
    if (act_move(&a, ACT_CHECK) == ACT_FAIL && a.reason == ACT_R_BLOCKED_ENTITY) {
        blocker = a.target;
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(blocker, COMPONENT_HEALTH)) {
            player_act(ACT_MELEE, &a);   /* a.target: the blocker */
            return;
        }
        if (FEATURE_IS(blocker) && !FEATURE_IS_OPEN(blocker)) {
            player_act(ACT_OPEN, &a);    /* a.target: the door */
            return;
        }
    }
    player_act(ACT_MOVE, &a);
}

static void pick_up(void)
{
    act_t a = { 0 };

    a.target = NGE_ENTITY_INVALID;   /* the first item here */
    player_act(ACT_PICKUP, &a);
}

/* Look: choose a tile anywhere on the map, and hear what is on it. No turn is used */
static void look(void)
{
    target_t t;

    t.purpose = TARGET_LOOK;
    t.origin_x = NGE_LOCATION_X(actor_player);
    t.origin_y = NGE_LOCATION_Y(actor_player);
    t.range = TARGET_RANGE_ANY;
    t.x = t.origin_x;
    t.y = t.origin_y;
    if (target_choose(&t)) {
        messages_look(t.x, t.y);
    }
}

/* Choose an item on a screen (UI_*), then do the action (ACT_*) with it */
static void item_command(uint8_t screen, uint8_t action)
{
    act_t a = { 0 };

    a.item = ui_choose_item(screen);
    if (a.item != NGE_ENTITY_INVALID) {
        a.kind = SLOT_NONE;   /* equip: the first free slot it fits */
        player_act(action, &a);
    }
}

/* Close: choose a direction, then close the door that way */
static void close_command(void)
{
    act_t a = { 0 };

    if (player_ask_direction(PLAYER_PROMPT_CLOSE, &a.dx, &a.dy)) {
        a.target = NGE_ENTITY_INVALID;   /* what is in that direction */
        player_act(ACT_CLOSE, &a);
    }
}

/* Fire: choose a direction; there is nothing to fire yet */
static void fire_command(void)
{
    int8_t dx;
    int8_t dy;

    if (player_ask_direction(PLAYER_PROMPT_FIRE, &dx, &dy)) {
        screen_message("You have nothing to fire.");
    }
}

static void stairs(uint8_t action)
{
    act_t a = { 0 };

    player_act(action, &a);
}

void player_command(nge_key_event_t ev) NGE_BANKED
{
    int8_t dx;
    int8_t dy;

    if (player_key_direction(ev, &dx, &dy)) {
        move(dx, dy);
        return;
    }
    switch (nge_key_ascii(ev)) {
    case 'g':
    case ',':
        pick_up();
        break;
    case 'i':
        ui_show_inventory();
        break;
    case 'd':
        item_command(UI_DROP, ACT_DROP);
        break;
    case 'w':
        item_command(UI_EQUIP, ACT_EQUIP);
        break;
    case 'r':
        item_command(UI_UNEQUIP, ACT_UNEQUIP);
        break;
    case 'u':
        item_command(UI_USE, ACT_USE);
        break;
    case 'f':
        fire_command();
        break;
    case 'c':
        close_command();
        break;
    case 'x':
        look();
        break;
    case '<':
        stairs(ACT_ASCEND);
        break;
    case '>':
        stairs(ACT_DESCEND);
        break;
    case 'Q':
        screen_message("You quit.");
        game_over(GAME_OVER_QUIT);
        break;
#if GAME_DEV_KIT
    case 'X': {
        /* Development: hurt the player, 3 damage, a different type each time (no turn used) */
        static uint8_t type;

        health_damage(actor_player, NGE_ENTITY_INVALID, 3, type);
        type = (uint8_t)((type + 1) % DMG_COUNT);
        break;
    }
    case 'F':
        /* Development: fall to the next level, anywhere on it (no turn used): falls before there are holes */
        level_request((uint8_t)(level_depth + 1), LEVEL_ARRIVE_ANY);
        break;
#endif
    default:
        break;   /* other keys: no command, no turn */
    }
}

void player_show_stats(void) NGE_BANKED
{
    uint8_t scores[ABIL_COUNT];   /* a copy: the table is by ability, then entity */
    uint8_t a;

    for (a = 0; a < ABIL_COUNT; a++) {
        scores[a] = stats_ability[a][actor_player];
    }
    screen_show_hp(health_hp[actor_player], health_hp_max[actor_player]);
    screen_show_ac(health_ac[actor_player]);
    screen_show_abilities(scores);
    screen_show_status(status_mask[actor_player]);
}

/* ---- The direction prompt ---- */

/* A prompt for a direction */
typedef struct player_prompt {
    const char *question;
    const char *help;
} player_prompt_t;

static const player_prompt_t prompts[PLAYER_PROMPT_COUNT] = {
    [PLAYER_PROMPT_FIRE]  = { "Fire in which direction?", "Direction: fire   Other keys: back" },
    [PLAYER_PROMPT_CLOSE] = { "Close in which direction?", "Direction: close   Other keys: back" },
};

uint8_t player_direction_key(nge_key_event_t ev, int8_t *dx, int8_t *dy) NGE_BANKED
{
    if (!player_key_direction(ev, dx, dy)) {
        screen_message("Never mind.");
        return 0;
    }
    return 1;
}

uint8_t player_ask_direction(uint8_t prompt, int8_t *dx, int8_t *dy) NGE_BANKED
{
    uint8_t answered;

    nge_util_assert(prompt < PLAYER_PROMPT_COUNT);

    nge_key_flush();   /* keys typed for play don't answer */
    screen_message(prompts[prompt].question);
    screen_help(prompts[prompt].help);
    answered = player_direction_key(game_wait_key(0), dx, dy);
    game_play_help();
    nge_key_flush();   /* a key held down doesn't leak into play */
    return answered;
}
