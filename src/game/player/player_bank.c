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
#include "ecs/systems/nge_timer.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/components/attrs.h"
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/dev.h"
#include "game/components/inventory.h"
#include "game/game.h"
#include "game/messages/messages.h"
#include "game/screen.h"

/* Characters of the movement keys (nge_key_ascii): cursor keys */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9
#define CHAR_DOWN   10
#define CHAR_UP     11

/* The direction of a movement key (the cursor keys, also CAPS+5-8, or h j k l): 1 and (dx, dy), or 0 */
static uint8_t key_direction(nge_key_event_t ev, int8_t *dx, int8_t *dy)
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
        nge_timer_start();
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

void player_command(nge_key_event_t ev) NGE_BANKED
{
    int8_t dx;
    int8_t dy;

    if (key_direction(ev, &dx, &dy)) {
        move(dx, dy);
        return;
    }
    switch (nge_key_ascii(ev)) {
    case 'g':
    case ',':
        pick_up();
        break;
    case 'i':
        game_set_state(GAME_INVENTORY);
        break;
    case 'd':
        game_set_state(GAME_DROP);
        break;
    case 'w':
        game_set_state(GAME_EQUIP);
        break;
    case 'r':
        game_set_state(GAME_UNEQUIP);
        break;
    case 'u':
        game_set_state(GAME_USE);
        break;
    case 'f':
        game_set_state(GAME_FIRE);
        break;
    case 'c':
        game_set_state(GAME_CLOSE);
        break;
    case 'Q':
        screen_message("You quit.");
        game_set_state(GAME_DEAD);
        break;
#if GAME_DEV_KIT
    case 'X': {
        /* Development: hurt the player, 3 damage, a different type each time (no turn used) */
        static uint8_t type;

        health_damage(actor_player, NGE_ENTITY_INVALID, 3, type);
        type = (uint8_t)((type + 1) % DMG_COUNT);
        break;
    }
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

/* ---- Direction prompts ---- */

#define NO_ACTION  0xFF   /* nothing to do yet: say so */

/* A prompt for a direction, and what a direction does */
typedef struct player_prompt {
    const char *question;
    const char *help;
    uint8_t     action;   /* ACT_* in that direction, or NO_ACTION */
} player_prompt_t;

static const player_prompt_t prompts[PLAYER_PROMPTS] = {
    [PLAYER_PROMPT_FIRE]  = { "Fire in which direction?", "Direction: fire   Other keys: back", NO_ACTION },
    [PLAYER_PROMPT_CLOSE] = { "Close in which direction?", "Direction: close   Other keys: back", ACT_CLOSE },
};

void player_direction_enter(uint8_t prompt) NGE_BANKED
{
    nge_util_assert(prompt < PLAYER_PROMPTS);

    screen_message(prompts[prompt].question);
    screen_help(prompts[prompt].help);
}

void player_direction_update(uint8_t prompt) NGE_BANKED
{
    nge_key_event_t ev = nge_key_get();

    if (ev != NGE_KEY_NONE) {
        player_direction_key(prompt, ev);
    }
}

void player_direction_key(uint8_t prompt, nge_key_event_t ev) NGE_BANKED
{
    act_t a = { 0 };
    int8_t dx;
    int8_t dy;

    nge_util_assert(prompt < PLAYER_PROMPTS);
    if (!key_direction(ev, &dx, &dy)) {
        screen_message("Never mind.");
    } else if (prompts[prompt].action == NO_ACTION) {
        screen_message("You have nothing to fire.");
    } else {
        a.target = NGE_ENTITY_INVALID;   /* what is in that direction */
        a.dx = dx;
        a.dy = dy;
        player_act(prompts[prompt].action, &a);
    }
    game_set_state(GAME_PLAY);
}
