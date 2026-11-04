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

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/systems/nge_timer.h"
#include "map/nge_map.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/stats.h"
#include "game/dev.h"
#include "game/events.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/game.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/spells/spells.h"
#include "game/target/target.h"
#include "game/ui/screen.h"
#include "game/ui/screen_main.h"
#include "game/ui/screen_inv.h"
#include "game/ui/screen_pc.h"
#include "game/ui/screen_spell.h"

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
 * closed door opens it, or unlocks it with a key in the pack (act_move()'s
 * check mode finds the blocker)
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
        if (NGE_ENTITY_HAS_COMPONENT(blocker, COMPONENT_HEALTH)) {
            player_act(ACT_MELEE, &a);   /* a.target: the blocker */
            return;
        }
        if (FEATURE_IS(blocker) && !FEATURE_IS_OPEN(blocker)) {
            /* a.target: the door */
            if (FEATURE_IS_LOCKED(blocker)) {
                a.item = feature_fitting_key(blocker, actor_player);
                if (a.item != NGE_ENTITY_INVALID) {
                    player_act(ACT_UNLOCK, &a);
                    return;
                }
            }
            player_act(ACT_OPEN, &a);
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

/* Choose a tile from the player's, within range, for an action: 1 with it in a->x, a->y */
static uint8_t choose_tile(uint8_t purpose, uint8_t range, act_t *a)
{
    target_t t;

    t.purpose = purpose;
    t.origin_x = NGE_LOCATION_X(actor_player);
    t.origin_y = NGE_LOCATION_Y(actor_player);
    t.range = range;
    t.x = t.origin_x;
    t.y = t.origin_y;
    if (!target_choose(&t)) {
        return 0;
    }
    a->x = t.x;
    a->y = t.y;
    return 1;
}

/*
 * Choose an item and an action on the inventory screen: browsing
 * (SCREEN_INV_BROWSE), or for an action (ACT_*). Then do the action with it,
 * aiming it first, or choosing a direction, if that is the action
 */
static void item_command(uint8_t mode)
{
    act_t a = { 0 };
    uint8_t used;
    uint8_t action = screen_inv_run(mode, &a.item, &used);

    if (used) {
        player_turn = 0;   /* actions done in the screen: one turn for them all, and for what follows */
    }
    if (action == SCREEN_INV_BACK || (action == ACT_AIM && !choose_tile(TARGET_AIM, ACT_AIM_RANGE, &a))) {
        return;
    }
    if (action == ACT_UNLOCK) {
        if (!player_ask_direction(PLAYER_PROMPT_UNLOCK, &a.dx, &a.dy)) {
            return;
        }
        a.target = NGE_ENTITY_INVALID;   /* what is in that direction */
    }
    a.slot = SLOT_NONE;   /* equip: the first free slot it fits */
    player_act(action, &a);
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

#if GAME_DEV_KIT
/*
 * Development (the T key, with level_dev_features()): a dart trap, a pit, a
 * fire rune and a web on the free tiles around the player, found, to be
 * stepped on, and a second dart trap on the next free tile, hidden
 * (docs/features.md, step 5)
 */
static void dev_traps(void)
{
    static const uint8_t traps[] = { FT_DART_TRAP, FT_PIT, FT_FIRE_RUNE, FT_WEB, FT_DART_TRAP };
    nge_entity_id_t id;
    uint8_t k = 0;
    uint8_t x;
    uint8_t y;

    for (y = (uint8_t)(NGE_LOCATION_Y(actor_player) - 1); y <= (uint8_t)(NGE_LOCATION_Y(actor_player) + 1); y++) {
        for (x = (uint8_t)(NGE_LOCATION_X(actor_player) - 1); x <= (uint8_t)(NGE_LOCATION_X(actor_player) + 1); x++) {
            if (k < sizeof(traps) && nge_location_find(LEVEL_AREA, x, y, 0, 0) == NGE_ENTITY_INVALID &&
                nge_block_can_enter(LEVEL_AREA, x, y, NGE_BLOCK_NONE, NGE_ENTITY_INVALID, NULL)) {
                id = gen_feature(traps[k++], LEVEL_AREA);
                if (id != NGE_ENTITY_INVALID) {
                    feature_place(id, x, y);
                    if (k < sizeof(traps)) {
                        feature_find(id);   /* all found but the last: a dart trap left hidden */
                    }
                }
            }
        }
    }
}
#endif

/* Operate a feature next to the player: pray at a shrine, drink from a fountain (docs/features.md 4.6) */
static void operate_command(void)
{
    act_t a = { 0 };

    if (player_ask_direction(PLAYER_PROMPT_OPERATE, &a.dx, &a.dy)) {
        a.target = NGE_ENTITY_INVALID;   /* what is in that direction */
        player_act(ACT_OPERATE, &a);
    }
}

/*
 * Fire: a check on the player's own tile says whether they can, and the
 * range (act_fire()), so having nothing to fire is said before the cursor
 * appears. Then choose a tile within range, and fire at it
 */
static void fire_command(void)
{
    act_t a = { 0 };

    a.actor = actor_player;
    a.x = NGE_LOCATION_X(actor_player);
    a.y = NGE_LOCATION_Y(actor_player);
    act_fire(&a, ACT_CHECK);
    if (a.reason != ACT_R_NO_TARGET) {
        messages_act_failed(&a);
        return;
    }
    if (choose_tile(TARGET_FIRE, a.amount, &a)) {
        player_act(ACT_FIRE, &a);
    }
}

/*
 * Cast: choose a spell from the list, which says why one can't be cast.
 * A bolt or a direct spell then asks for a tile within its range.
 * Backing out of either uses no turn
 */
static void cast_command(void)
{
    act_t a = { 0 };
    spell_def_t def;

    if (!screen_spell_run(&a.spell)) {
        return;
    }
    spell_get(a.spell, &def);
    if (def.shape != SPELL_SELF && !choose_tile(TARGET_CAST, def.range, &a)) {
        return;
    }
    player_act(ACT_CAST, &a);
}

#if GAME_DEV_KIT
#define DEV_GOLD  10   /* the gold the E key gives */

/* -1, 0 or 1: the movement keys held for one axis (the cursor keys, CAPS+5-8, or h j k l) */
static int8_t held_axis(uint8_t less, uint8_t less_caps, uint8_t less_key, uint8_t more, uint8_t more_caps,
                        uint8_t more_key)
{
    uint8_t caps = nge_key_down(NGE_KEY_CAPS);

    if (nge_key_down(less) || nge_key_down(less_key) || (caps && nge_key_down(less_caps))) {
        return -1;
    }
    if (nge_key_down(more) || nge_key_down(more_key) || (caps && nge_key_down(more_caps))) {
        return 1;
    }
    return 0;
}

/* Development: lock the nearest closed door on the level, for the keys and picks to try, as no level locks any */
static void lock_nearest(void)
{
    gen_feature_info_t info;
    nge_entity_id_t id;
    nge_entity_id_t door = NGE_ENTITY_INVALID;
    nge_owner_t area = NGE_OWNER_OF(actor_player);
    uint8_t px = NGE_LOCATION_X(actor_player);
    uint8_t py = NGE_LOCATION_Y(actor_player);
    uint8_t best = 255;
    uint8_t dx;
    uint8_t dy;

    for (id = 0; id < NGE_ENTITY_MAX; id++) {
        if (NGE_ENTITY_IS_ALIVE(id) && FEATURE_IS(id) && NGE_OWNER_OF(id) == area && !FEATURE_IS_OPEN(id) &&
            !FEATURE_IS_LOCKED(id)) {
            gen_feature_info(kind_index[id], &info);
            dx = (uint8_t)((NGE_LOCATION_X(id) > px) ? NGE_LOCATION_X(id) - px : px - NGE_LOCATION_X(id));
            dy = (uint8_t)((NGE_LOCATION_Y(id) > py) ? NGE_LOCATION_Y(id) - py : py - NGE_LOCATION_Y(id));
            if ((info.can & FCAN_OPEN) && (uint8_t)(dx + dy) < best) {
                best = (uint8_t)(dx + dy);
                door = id;
            }
        }
    }
    if (door != NGE_ENTITY_INVALID) {
        feature_lock(door, KEY_IRON);
        screen_message("Development: the nearest door is locked.");
    }
}

/*
 * Development: pan the view a pixel a frame while a movement key is held,
 * to check the pixel camera (docs/ui_layer2.md, R11): the map and the
 * sprites should move together. Any other key centres the camera on the
 * player again. No time passes
 */
static void pan_view(void)
{
    nge_key_event_t ev;
    int8_t dx;
    int8_t dy;
    int16_t x = (int16_t)NGE_MAP_CAMERA_PX();
    int16_t y = (int16_t)NGE_MAP_CAMERA_PY();

    nge_key_flush();
    screen_help("Development: arrows pan the view   Other keys: back");
    for (;;) {
        game_frame_start();
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE && !player_key_direction(ev, &dx, &dy)) {
            break;
        }
        x += held_axis(NGE_KEY_LEFT, NGE_KEY_5, NGE_KEY_H, NGE_KEY_RIGHT, NGE_KEY_8, NGE_KEY_L);
        y += held_axis(NGE_KEY_UP, NGE_KEY_7, NGE_KEY_K, NGE_KEY_DOWN, NGE_KEY_6, NGE_KEY_J);
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        nge_map_set_camera_px((uint16_t)x, (uint16_t)y);
        x = (int16_t)NGE_MAP_CAMERA_PX();   /* clamped at the map's far edges */
        y = (int16_t)NGE_MAP_CAMERA_PY();
        game_frame_end();
    }
    nge_map_centre_camera(NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));
    game_play_help();
    nge_key_flush();
}
#endif

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
        item_command(SCREEN_INV_BROWSE);
        break;
    case 'd':
        item_command(ACT_DROP);
        break;
    case 'w':
        item_command(ACT_EQUIP);
        break;
    case 'r':
        item_command(ACT_UNEQUIP);
        break;
    case 'e':
        item_command(ACT_EXCHANGE);   /* done in the screen, which returns SCREEN_INV_BACK */
        break;
    case 'u':
        item_command(ACT_USE);
        break;
    case 'a':
        item_command(ACT_AIM);
        break;
    case 'p':
        item_command(ACT_UNLOCK);
        break;
    case 'o':
        operate_command();
        break;
    case 'f':
        fire_command();
        break;
    case 'm':
        cast_command();
        break;
    case 'c':
        close_command();
        break;
    case 'x':
        look();
        break;
    case 'C':
        screen_pc_run();   /* only shows: no turn used */
        break;
    case '<':
        stairs(ACT_ASCEND);
        break;
    case '>':
        stairs(ACT_DESCEND);
        break;
    case '.': {
        act_t a = { 0 };

        player_act(ACT_WAIT, &a);   /* let the monsters come */
        break;
    }
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
    case 'V':
        pan_view();   /* development: pan the view a pixel at a time */
        break;
    case 'L':
        lock_nearest();   /* development: no turn used */
        break;
    case 'T':
        level_dev_features(actor_player);   /* development: features to check, near the player (no turn used) */
        dev_traps();
        break;
    case 'M':
        /* Development: spend 1 MP, or restore them all at 0, to see the stats follow (no turn used) */
        if (MANA_IS(actor_player)) {
            if (MANA_HAS(actor_player, 1)) {
                mana_spend(actor_player, 1);
            } else {
                mana_restore(actor_player, mana_mp_max[actor_player]);
            }
        }
        break;
    case 'E': {
        /*
         * Development: up a level and 10 gold (no turn used): the XP the
         * next level needs, less the 10 the gold gives (one per gold piece)
         */
        uint16_t need;

        if (pc_level < PC_LEVEL_MAX) {
            need = (uint16_t)(pc_xp_needed((uint8_t)(pc_level + 1)) - pc_xp);
            if (need > DEV_GOLD) {
                pc_gain_xp(actor_player, (uint16_t)(need - DEV_GOLD));
            }
        }
        pc_gain_gold(actor_player, DEV_GOLD);
        break;
    }
#endif
    default:
        break;   /* other keys: no command, no turn */
    }
}

void player_on_pc_event(const nge_event_t *e) NGE_BANKED
{
    nge_entity_id_t target = e->target;   /* locals: see messages_on_event() */
    nge_entity_id_t source = e->source;
    uint8_t id = e->id;
    uint8_t p1 = e->p1;

    if (id == EV_DIED) {
        /* A creature the player killed: its kind's XP (it is dying, so its kind is still there) */
        if (source == actor_player && target != actor_player && KIND_IS(target, CLASS_CREATURE)) {
            pc_gain_xp(actor_player, gen_creature_xp(kind_index[target]));
        }
        return;
    }
    if (target != actor_player) {
        return;
    }
    switch (id) {
    case EV_XP_GAINED:
        screen_show_xp(pc_xp);
        break;
    case EV_LEVEL_GAINED:
        screen_show_level(pc_level);
        break;
    case EV_GOLD_CHANGED:
        screen_show_gold(pc_gold);
        break;
    case EV_ARRIVED:
        pc_reach_depth(actor_player, p1);
        break;
    default:
        break;
    }
}

void player_show_stats(void) NGE_BANKED
{
    uint8_t scores[ABIL_COUNT];   /* a copy: the table is by ability, then entity */
    uint8_t a;

    for (a = 0; a < ABIL_COUNT; a++) {
        scores[a] = stats_ability[a][actor_player];
    }
    screen_show_name(pc_name);
    screen_show_level(pc_level);
    screen_show_hp(health_hp[actor_player], health_hp_max[actor_player]);
    if (MANA_IS(actor_player)) {   /* none: no row (a class without magic, later) */
        screen_show_mp(mana_mp[actor_player], mana_mp_max[actor_player]);
    }
    screen_show_ac(health_ac[actor_player]);
    screen_show_abilities(scores);
    screen_show_xp(pc_xp);
    screen_show_gold(pc_gold);
    screen_show_status(stats_status[actor_player]);
}

/* ---- The direction prompt ---- */

/* A prompt for a direction */
typedef struct player_prompt {
    const char *question;
    const char *help;
} player_prompt_t;

static const player_prompt_t prompts[PLAYER_PROMPT_COUNT] = {
    [PLAYER_PROMPT_CLOSE] = { "Close in which direction?", "Direction: close   Other keys: back" },
    [PLAYER_PROMPT_UNLOCK] = { "Unlock in which direction?", "Direction: unlock   Other keys: back" },
    [PLAYER_PROMPT_OPERATE] = { "Use what? Which direction?", "Direction: use   Other keys: back" },
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
