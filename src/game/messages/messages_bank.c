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
#include "game/components/feature.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/dev.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/game.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/quest/quest.h"
#include "game/spells/spells.h"
#include "game/ui/screen_main.h"

#define MESSAGE_MAX  41   /* a message window line, and its NUL */
#define NAME_MAX     20   /* a kind's name, and its NUL */
#define SEEN_MAX     26   /* a space, an effect's longest seen text ("is drained of presence."), and its NUL */

/* line[MESSAGE_MAX] ends in the word "a", and name is next: make it "an" ahead of a vowel */
static void set_article(char *line, const char *name)
{
    uint8_t n = (uint8_t)strlen(line);
    char c = name[0];

    /* By hand: strcmp() would link into common memory, which has no room */
    if (n >= 3 && n + 1 < MESSAGE_MAX && line[n - 3] == ' ' && line[n - 2] == 'a' && line[n - 1] == ' ' &&
        (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')) {
        line[n - 1] = 'n';
        line[n] = ' ';
        line[n + 1] = '\0';
    }
}

/* Show before + the name of id's kind + after, cut to one message line */
static void say_named(const char *before, nge_entity_id_t id, const char *after)
{
    char line[MESSAGE_MAX];
    char name[NAME_MAX];

    gen_name(id, name, sizeof(name));
    line[0] = '\0';
    strncat(line, before, sizeof(line) - 1);
    set_article(line, name);
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

/* Effect ef starting on a monster the player sees: "The goblin is terrified!", if ef has a text for it */
static void say_seen_effect(nge_entity_id_t target, uint8_t ef)
{
    char after[SEEN_MAX];

    after[0] = ' ';
    effect_text(ef, EFFECT_TEXT_SEEN, after + 1, sizeof(after) - 1);
    if (after[1] != '\0') {
        say_named("The ", target, after);
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

/* A monster with AI that the player sees (an AI entity's doings are said only then) */
static uint8_t seen_ai(nge_entity_id_t id)
{
    return id != NGE_EVENT_NONE && AI_IS(id) && ai_sees(actor_player, id);
}

/* "The goblin picks up a sword.": verb is " picks up a ", thing is the sword */
static void say_did(nge_entity_id_t actor, const char *verb, nge_entity_id_t thing)
{
    char line[MESSAGE_MAX];
    char name[NAME_MAX];

    strcpy(line, "The ");
    gen_name(actor, name, sizeof(name));
    add(line, name);
    add(line, verb);
    gen_name(thing, name, sizeof(name));
    set_article(line, name);
    add(line, name);
    add(line, ".");
    screen_message(line);
}

/* How an item is used, by what use_verb() says */
#define USE_DRINK  0
#define USE_EAT    1
#define USE_READ   2
#define USE_COUNT  3

static const char *const use_you[USE_COUNT] = { "You drink the ", "You eat the ", "You read the " };
static const char *const use_it[USE_COUNT] = { " drinks the ", " eats the ", " reads the " };

/* What the player is told on operating a feature, by its kind's verb (FVERB_*, docs/features.md 4.11) */
static const char *const operate_you[FVERB_COUNT] = {
    "You use the ", "You pray at the ", "You drink from the ", "You touch the ", "You step on the ", "You fall into the ",
    "You fall through the ",
};

/* What the player is told when a trap goes off at them, or at a monster in sight, by its kind's verb */
static const char *const trap_you[FVERB_COUNT] = {
    "You set off a ", "You set off a ", "You set off a ", "You set off a ", "You step on a ", "You fall into a ",
    "You fall through a ",
};
static const char *const trap_it[FVERB_COUNT] = {
    " sets off a ", " sets off a ", " sets off a ", " sets off a ", " steps on a ", " falls into a ",
    " falls through a ",
};

/* How item is used (USE_*), from its subclass */
static uint8_t use_verb(nge_entity_id_t item)
{
    uint8_t subclass;

    if (!KIND_IS(item, CLASS_ITEM)) {
        return USE_DRINK;
    }
    subclass = gen_item_subclass(kind_index[item]);
    if (subclass == ITEM_FOOD) {
        return USE_EAT;
    }
    return (subclass == ITEM_PARCHMENT) ? USE_READ : USE_DRINK;
}

#if GAME_DEV_KIT
/* Development: what look says after a monster's name, by its AI state (game/dev.h) */
static const char *const ai_state_names[AI_STATE_COUNT] = {
    [AI_STATE_NONE] = ".", [AI_STATE_ASLEEP] = " (asleep).", [AI_STATE_IDLE] = " (idle).",
    [AI_STATE_HUNT] = " (hunting).",
};
#endif

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

/* "You learn <spell>." */
static void say_learned(uint8_t sp)
{
    char line[MESSAGE_MAX];
    char name[SPELL_NAME_MAX + 1];

    spell_name(sp, name, sizeof(name));   /* a copy: the name is in the spells' page */
    strcpy(line, "You learn ");
    add(line, name);
    add(line, ".");
    screen_message(line);
}

/* "You cast <spell>.", or "The <caster> casts <spell>." if the player sees it */
static void say_cast(nge_entity_id_t caster, uint8_t sp)
{
    char line[MESSAGE_MAX];
    char name[SPELL_NAME_MAX + 1];

    spell_name(sp, name, sizeof(name));
    if (caster == actor_player) {
        strcpy(line, "You cast ");
        add(line, name);
        add(line, ".");
        screen_message(line);
    } else if (seen_ai(caster)) {
        strcpy(line, " casts ");
        add(line, name);
        add(line, ".");
        say_named("The ", caster, line);
    }
}

/*
 * What the player sees on tile (x, y) of the current level: a line for each
 * entity with a kind (they have names), in ID order, or nothing there. A
 * hidden feature isn't seen: unknown to the player is not there (D8)
 */
static void look_at(uint8_t x, uint8_t y)
{
    nge_entity_id_t id;
    uint8_t seen = 0;

    NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, x, y, COMPONENT_KIND) {
        if (FEATURE_IS(id) && FEATURE_IS_HIDDEN(id)) {
            continue;
        }
        if (id == actor_player) {
            screen_message("You see yourself.");
        } else {
#if GAME_DEV_KIT
            say_named("You see a ", id, AI_IS(id) ? ai_state_names[ai_get_state(id)] : ".");
#else
            say_named("You see a ", id, ".");
#endif
        }
        seen = 1;
    }
    if (!seen) {
        screen_message("You see nothing there.");
    }
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
    case EV_FIRED:
        if (source == actor_player) {
            screen_message("You fire.");
        } else if (seen_ai(source)) {
            say_named("The ", source, " fires.");
        }
        break;
    case EV_AIMED:
        if (source == actor_player) {
            say_named("You aim the ", p1, ".");
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
        } else if (seen_ai(source)) {
            say_did(source, " picks up a ", target);
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
        } else if (seen_ai(source)) {
            say_did(source, " opens the ", target);
        }
        break;
    case EV_OPERATED:
        if (source == actor_player) {   /* its effect's own message follows */
            say_named(operate_you[gen_feature_verb(kind_index[target])], target, ".");
        }
        break;
    case EV_TRIGGERED: {
        /* Off the trap's tile, a failed disarm set it off: no stepping or falling */
        uint8_t verb = (NGE_LOCATION_X(source) == NGE_LOCATION_X(target) && NGE_LOCATION_Y(source) == NGE_LOCATION_Y(target))
                           ? gen_feature_verb(kind_index[target]) : FVERB_NONE;

        if (source == actor_player) {
            say_named(trap_you[verb], target, "!");
        } else if (seen_ai(source)) {
            say_did(source, trap_it[verb], target);
        }
        break;
    }
    case EV_FOUND:
        if (source == actor_player) {   /* NGE_EVENT_NONE: found by going off, which says so */
            say_named("You find a ", target, "!");
        }
        break;
    case EV_DISARMED:
        if (source == actor_player) {
            say_named("You disarm the ", target, ".");
        }
        break;
    case EV_SEARCHED:
        if (target == actor_player && p1 == 0) {
            screen_message("You find nothing.");
        }
        break;
    case EV_LOOKED:
        if (target == actor_player) {
            look_at(p1, p2);
        }
        break;
    case EV_UNLOCKED:
        if (source == actor_player) {
            say_named("You unlock the ", target, ".");
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
        } else if (seen_ai(source)) {
            say_did(source, (p1 == SLOT_WEAPON) ? " wields a " :
                            (p1 == SLOT_RANGED || p1 == SLOT_AMMO) ? " readies a " : " puts on a ", target);
        }
        break;
    case EV_UNEQUIPPED:
        if (source == actor_player) {
            say_named("You take off a ", target, ".");
        }
        break;
    case EV_USED:
        if (source == actor_player) {
            say_named(use_you[use_verb(target)], target, ".");
        } else if (seen_ai(source)) {
            say_did(source, use_it[use_verb(target)], target);
        }
        break;
    case EV_EFFECT_STARTED:
        if (target == actor_player) {
            say_effect(p1, EFFECT_TEXT_START);
        } else if (seen_ai(target)) {
            say_seen_effect(target, p1);   /* docs/items_extensions.md R12 */
        }
        break;
    case EV_EFFECT_ENDED:
        if (target == actor_player) {
            say_effect(p1, EFFECT_TEXT_END);
        } else if (p2 == EFFECT_END_SAVED && seen_ai(target)) {
            say_named("The ", target, " shakes it off.");   /* R13: one that runs out goes unsaid */
        }
        break;
    case EV_RESISTED:
        if (target == actor_player) {
            screen_message("You resist.");   /* docs/items_extensions.md R14 */
        } else if (seen_ai(target)) {
            say_named("The ", target, " resists.");
        }
        break;
    case EV_FIZZLED:
        if (target == actor_player) {
            screen_message("Nothing happens.");
        }
        break;
    case EV_ARRIVED:
        say_arrived(p1, p2);
        break;
    case EV_LEVEL_GAINED:
        say_level(p1);
        if (MANA_IS(target) && spell_slots_new(pc_class, p1)) {   /* the table, not the player: more levels may have come (D26) */
            screen_message("You can learn more spells from a book.");
        }
        break;
    case EV_CAST:
        say_cast(source, p1);
        break;
    case EV_SPELL_LEARNED:
        if (target == actor_player) {
            say_learned(p1);
        }
        break;
    case EV_LEFT_LEVEL:
        say_named("The ", target, p1 < level_depth ? " goes up and away." : " goes down and away.");
        break;
    case EV_AI_CHANGED:
        if (p1 == AI_STATE_ASLEEP && seen_ai(target)) {
            say_named("The ", target, " wakes up.");
        } else if (p1 == AI_STATE_IDLE && p2 == AI_STATE_HUNT && source == actor_player && seen_ai(target)) {
            say_named("The ", target, " notices you.");
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
    case ACT_R_ROOTED:
        screen_message("You can't move!");
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
        screen_message("You have nothing to attack with.");
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
    case ACT_R_NO_LINE:
        screen_message("Something is in the way.");
        break;
    case ACT_R_NO_AMMO:
        screen_message("You have no ammunition ready.");
        break;
    case ACT_R_WRONG_AMMO:
        screen_message("That ammunition doesn't fit.");
        break;
    case ACT_R_CANT_AIM:
        screen_message("You can't aim that.");
        break;
    case ACT_R_NO_CHARGES:
        screen_message("It has no charges left.");
        break;
    case ACT_R_CURSED:
        screen_message("It is cursed: it won't come off.");
        break;
    case ACT_R_LOCKED:
        say_named("The ", target, " is locked.");
        break;
    case ACT_R_NOT_LOCKED:
        say_named("The ", target, " isn't locked.");
        break;
    case ACT_R_WRONG_KEY:
        screen_message("That doesn't fit the lock.");
        break;
    case ACT_R_PICK_FAILED:
        screen_message("The lock holds.");
        break;
    case ACT_R_NO_MANA:
        screen_message("You haven't the mana.");
        break;
    case ACT_R_NO_SPELL:
        screen_message("You don't know that spell.");
        break;
    case ACT_R_NOT_BOOK:
        screen_message("That isn't a spell book.");
        break;
    case ACT_R_CANT_LEARN:
        screen_message("The spells mean nothing to you.");
        break;
    case ACT_R_NOT_IN_BOOK:
        screen_message("That spell isn't in the book.");
        break;
    case ACT_R_KNOWN:
        screen_message("You know that spell already.");
        break;
    case ACT_R_BEYOND:
        screen_message("That spell is beyond you for now.");
        break;
    case ACT_R_SLOTS_FULL:
        screen_message("No room for more spells of that level.");
        break;
    case ACT_R_AFRAID:
        screen_message("You are too afraid.");
        break;
    case ACT_R_STUMBLED:
        screen_message("You stumble.");
        break;
    case ACT_R_BLIND:
        screen_message("You can't see!");
        break;
    case ACT_R_CANT_OPERATE:
        say_named("You can't use the ", target, ".");
        break;
    case ACT_R_SPENT:
        say_named("The ", target, " is spent.");
        break;
    case ACT_R_CANT_DISARM:
        say_named("You can't disarm the ", target, ".");
        break;
    case ACT_R_DISARM_FAILED:
        say_named("You fail to disarm the ", target, ".");
        break;
    case ACT_R_SET_OFF:
        break;   /* the trap going off says so (EV_TRIGGERED) */
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
