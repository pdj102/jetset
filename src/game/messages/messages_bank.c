/*
 * The message window's reactions: banked in page 118 with their literals
 * (a banked game module, see "Banked game modules" in docs/memory_map.md),
 * as they run once per event or refused command, not every frame. They pass
 * their literals only to common code (the screen).
 */
#ifdef __SDCC
#pragma codeseg PAGE_118
#pragma constseg PAGE_118
#endif

#include "game/messages/messages.h"

#include <string.h>

#include "ecs/components/nge_location.h"

#include "game/actors.h"
#include "game/ai/ai.h"
#include "game/components/components.h"
#include "game/components/health.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/game.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/quest/quest.h"
#include "game/ui/screen_main.h"

#define MESSAGE_MAX  41   /* a message window line, and its NUL */
#define NAME_MAX     20   /* a kind's name, and its NUL */

/* Show before + the name of id's kind + after, cut to one message line */
static void say_named(const char *before, nge_entity_id_t id, const char *after)
{
    char line[MESSAGE_MAX];
    char name[NAME_MAX];

    gen_name(id, name, sizeof(name));
    line[0] = '\0';
    strncat(line, before, sizeof(line) - 1);
    strncat(line, name, sizeof(line) - 1 - strlen(line));
    strncat(line, after, sizeof(line) - 1 - strlen(line));
    screen_message(line);
}

/* Effect ef's start or end text (EFFECT_TEXT_*), if it has one */
static void say_effect(uint8_t ef, uint8_t which)
{
    char line[MESSAGE_MAX];

    effect_text(ef, which, line, sizeof(line));
    if (line[0] != '\0') {
        screen_message(line);
    }
}

/* Each damage type's name, in messages */
static const char *const damage_names[DMG_COUNT] = {
    [DMG_BLUNT] = "blunt", [DMG_SLASH] = "slashing", [DMG_PIERCE] = "piercing", [DMG_FIRE] = "fire",
    [DMG_COLD] = "cold", [DMG_POISON] = "poison", [DMG_ACID] = "acid", [DMG_MAGIC] = "magic",
};

/* Append s to line (MESSAGE_MAX), cut to fit */
static void add(char *line, const char *s)
{
    strncat(line, s, MESSAGE_MAX - 1 - strlen(line));
}

/* Append n (0-255) in decimal to line */
static void add_number(char *line, uint8_t n)
{
    char digits[4];
    char *d = digits + sizeof(digits) - 1;

    *d = '\0';
    do {
        *--d = (char)('0' + n % 10);
        n /= 10;
    } while (n != 0);
    add(line, d);
}

/*
 * EV_DAMAGED: "You take 3 fire damage (resisted).", "The rat is immune to
 * poison." The player is "You", others "The <kind>"
 */
static void say_damage(nge_entity_id_t target, uint8_t taken, uint8_t p2)
{
    char line[MESSAGE_MAX];
    char name[NAME_MAX];
    uint8_t outcome = EV_DMG_OUTCOME(p2);
    uint8_t you = target == actor_player;

    if (you) {
        strcpy(line, "You ");
    } else {
        strcpy(line, "The ");
        gen_name(target, name, sizeof(name));
        add(line, name);
        add(line, " ");
    }
    if (outcome == DMG_IMMUNE) {
        add(line, you ? "are immune to " : "is immune to ");
        add(line, damage_names[EV_DMG_TYPE(p2)]);
        add(line, ".");
    } else {
        add(line, you ? "take " : "takes ");
        add_number(line, taken);
        add(line, " ");
        add(line, damage_names[EV_DMG_TYPE(p2)]);
        add(line, outcome == DMG_RESISTED ? " damage (resisted)."
                  : outcome == DMG_VULNERABLE ? " damage (vulnerable)!" : " damage.");
    }
    screen_message(line);
}

/* EV_ARRIVED: "You go down to level 2.", by how the player came (LEVEL_ARRIVE_*) */
static void say_arrived(uint8_t depth, uint8_t arrival)
{
    char line[MESSAGE_MAX];

    strcpy(line, arrival == LEVEL_ARRIVE_UP_STAIRS ? "You go down to level "
                 : arrival == LEVEL_ARRIVE_DOWN_STAIRS ? "You go up to level " : "You find yourself on level ");
    add_number(line, depth);
    add(line, ".");
    screen_message(line);
}

/* EV_LEVEL_GAINED: "Welcome to experience level 2." (not "level": that is the depth) */
static void say_level(uint8_t level)
{
    char line[MESSAGE_MAX];

    strcpy(line, "Welcome to experience level ");
    add_number(line, level);
    add(line, ".");
    screen_message(line);
}

void messages_on_event(const nge_event_t *e) NGE_BANKED
{
    /*
     * The fields in locals: SDCC (z88dk, Oct 2026) reused HL for
     * actor_player while it still expected it to point at e, so
     * say_named(..., e->target, ...) passed the player instead
     */
    nge_entity_id_t target = e->target;
    nge_entity_id_t source = e->source;
    uint8_t p1 = e->p1;
    uint8_t p2 = e->p2;

    switch (e->id) {
    case EV_DAMAGED:
        say_damage(target, p1, p2);
        break;
    case EV_HIT:
        if (source == actor_player) {
            say_named("You hit the ", target, ".");
        } else if (target == actor_player) {
            say_named("The ", source, " hits you.");
        }
        break;
    case EV_MISSED:
        if (source == actor_player) {
            say_named("You miss the ", target, ".");
        } else if (target == actor_player) {
            say_named("The ", source, " misses you.");
        }
        break;
    case EV_DIED:
        if (target == actor_player) {
            screen_message("You die...");
        } else if (KIND_IS(target, CLASS_CREATURE)) {
            say_named("The ", target, " dies.");
        } else {
            say_named("The ", target, " is destroyed.");
        }
        break;
    case EV_STOOD_ON:
        if (source == actor_player && KIND_IS(target, CLASS_ITEM)) {   /* features show on the map */
            say_named("You see a ", target, " here.");
        }
        break;
    case EV_PICKED_UP:
        if (source == actor_player) {
            say_named("You pick up a ", target, ".");
        }
        break;
    case EV_QUEST_FOUND:
        screen_message("Now climb back out of the dungeon.");
        break;
    case EV_DROPPED:
        if (source == actor_player) {
            say_named("You drop a ", target, ".");
        }
        break;
    case EV_OPENED:
        if (source == actor_player) {
            say_named("You open the ", target, ".");
        }
        break;
    case EV_CLOSED:
        if (source == actor_player) {
            say_named("You close the ", target, ".");
        }
        break;
    case EV_EQUIPPED:
        if (source == actor_player) {
            say_named("You equip a ", target, ".");
        }
        break;
    case EV_UNEQUIPPED:
        if (source == actor_player) {
            say_named("You take off a ", target, ".");
        }
        break;
    case EV_USED:
        if (source == actor_player) {
            say_named(KIND_IS(target, CLASS_ITEM) && gen_item_subclass(kind_index[target]) == ITEM_FOOD
                          ? "You eat the " : "You drink the ",
                      target, ".");
        }
        break;
    case EV_EFFECT_STARTED:
        if (target == actor_player) {
            say_effect(p1, EFFECT_TEXT_START);
        }
        break;
    case EV_EFFECT_ENDED:
        if (target == actor_player) {
            say_effect(p1, EFFECT_TEXT_END);
        }
        break;
    case EV_ARRIVED:
        say_arrived(p1, p2);
        break;
    case EV_LEVEL_GAINED:
        say_level(p1);
        break;
    case EV_LEFT_LEVEL:
        say_named("The ", target, p1 < level_depth ? " goes up and away." : " goes down and away.");
        break;
    case EV_AI_CHANGED:
        if (p1 == AI_STATE_ASLEEP) {
            say_named("The ", target, " wakes up.");
        }
        break;
    default:
        break;
    }
}

/* Tell the player why their action failed (a->reason), if they need telling */
void messages_act_failed(const act_t *a) NGE_BANKED
{
    /* The fields in locals: see messages_on_event() */
    uint8_t reason = a->reason;
    nge_entity_id_t target = a->target;

    switch (reason) {
    case ACT_R_BLOCKED:
        screen_message("You bump into a wall.");
        break;
    case ACT_R_BLOCKED_ENTITY:
        say_named("There is a ", target, " in the way.");
        break;
    case ACT_R_NOTHING_HERE:
        screen_message("There is nothing here to pick up.");
        break;
    case ACT_R_FULL:
        screen_message("You can't carry any more.");
        break;
    case ACT_R_EQUIPPED:
        screen_message("You must take that off first.");
        break;
    case ACT_R_ALREADY_EQUIPPED:
        screen_message("You are already using that.");
        break;
    case ACT_R_CANT_EQUIP:
        screen_message("You can't equip that.");
        break;
    case ACT_R_NO_SLOT:
        screen_message("You have nowhere to put that.");
        break;
    case ACT_R_SLOT_TAKEN:
        say_named("You are already using a ", target, ".");
        break;
    case ACT_R_NOT_EQUIPPED:
        screen_message("You aren't using that.");
        break;
    case ACT_R_NO_ATTACK:
        screen_message("You have no way to attack.");
        break;
    case ACT_R_NO_TARGET:
        screen_message("There is nothing there to attack.");
        break;
    case ACT_R_CANT_DAMAGE:
        screen_message("You can't attack that.");
        break;
    case ACT_R_OUT_OF_RANGE:
        screen_message("That is out of reach.");
        break;
    case ACT_R_CANT_USE:
        screen_message("You can't use that.");
        break;
    case ACT_R_NOTHING_THERE:
        screen_message("There is nothing there.");
        break;
    case ACT_R_CANT_OPEN:
        screen_message("That doesn't open or close.");
        break;
    case ACT_R_ALREADY_OPEN:
        say_named("The ", target, " is already open.");
        break;
    case ACT_R_ALREADY_CLOSED:
        say_named("The ", target, " is already closed.");
        break;
    case ACT_R_OBSTRUCTED:
        screen_message("Something is in the way.");
        break;
    case ACT_R_NO_STAIRS:
        screen_message("There are no stairs that way here.");
        break;
    case ACT_R_NO_QUEST:
        say_named("You can't leave without the ", quest_item, ".");
        break;
    default:
        break;
    }
}

void messages_game_over(uint8_t reason) NGE_BANKED
{
    if (reason == GAME_OVER_WON) {
        say_named("You escape with the ", quest_item, ". You win!");
        screen_message("Press any key.");
    } else {
        screen_message("Game over. Press any key.");
    }
}

void messages_look(uint8_t x, uint8_t y) NGE_BANKED
{
    nge_entity_id_t id;
    uint8_t seen = 0;

    /* Only entities with a kind: they have names */
    NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, x, y, COMPONENT_KIND) {
        if (id == actor_player) {
            screen_message("You see yourself.");
        } else {
            say_named("You see a ", id, ".");
        }
        seen = 1;
    }
    if (!seen) {
        screen_message("You see nothing there.");
    }
}
