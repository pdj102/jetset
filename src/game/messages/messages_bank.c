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

#include "game/actors.h"
#include "game/components/health.h"
#include "game/components/kind.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/screen.h"

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
    case EV_HEALED:
        if (target == actor_player) {
            screen_message("You feel better.");
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
        if (source == actor_player) {
            say_named("You see a ", target, " here.");
        }
        break;
    case EV_PICKED_UP:
        if (source == actor_player) {
            say_named("You pick up a ", target, ".");
        }
        break;
    case EV_DROPPED:
        if (source == actor_player) {
            say_named("You drop a ", target, ".");
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
    case EV_GLOWED:
        if (target == actor_player) {
            screen_message("A warm glow surrounds you.");
        }
        break;
    case EV_GLOW_FADED:
        if (target == actor_player) {
            screen_message("The warm glow fades.");
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
    case ACT_R_BLOCKED_CREATURE:
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
    default:
        break;
    }
}
