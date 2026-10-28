/* game/effects: applying, timing, ending and recomputing effects. */
#include "test.h"

#include <string.h>

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/stats.h"
#include "game/effects/effects_game.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/timers.h"

#define AREA  NGE_OWNER_AREA(0)

/* The events posted since the last call */
static nge_event_t seen[32];
static int seen_count;

static void record(const nge_event_t *e)
{
    if (seen_count < (int)(sizeof(seen) / sizeof(seen[0]))) {
        seen[seen_count] = *e;
    }
    seen_count++;
}

static int events(void)
{
    seen_count = 0;
    nge_event_process(record);
    return seen_count;
}

static int is(int i, uint8_t target, uint8_t id, uint8_t source, uint8_t p1)
{
    return i < seen_count && seen[i].target == target && seen[i].id == id && seen[i].source == source &&
           seen[i].p1 == p1;
}

/* Index of the first event id after from, or -1 */
static int find_event(int from, uint8_t id)
{
    int i;

    for (i = from; i < seen_count; i++) {
        if (seen[i].id == id) {
            return i;
        }
    }
    return -1;
}

/* The timers: effects fire through effects_on_timer(); each effect firing is counted */
static int fired;

static void dispatch(nge_entity_id_t id, uint8_t kind, uint8_t cycles)
{
    if (kind == TIMER_EFFECT) {
        fired++;
        effects_on_timer(id, cycles);
    }
}

/* Move time on until n more effect firings, or 1000 steps */
static void fire(int n)
{
    int want = fired + n;
    int steps = 0;

    while (fired < want && steps++ < 1000) {
        nge_timer_advance(dispatch);
    }
}

/* target's active effects of kind ef */
static int count_effects(nge_entity_id_t target, uint8_t ef)
{
    nge_entity_id_t id;
    uint8_t i;
    int n = 0;

    NGE_OWNER_FOR_EACH_OWNED(i, id, target) {
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_EFFECT) && effect_kind_of(id) == ef) {
            n++;
        }
    }
    return n;
}

/* target's (only) active effect of kind ef, or NGE_ENTITY_INVALID */
static nge_entity_id_t effect_on(nge_entity_id_t target, uint8_t ef)
{
    nge_entity_id_t id;
    uint8_t i;

    NGE_OWNER_FOR_EACH_OWNED(i, id, target) {
        if (NGE_ENTITY_HAS_ANY_COMPONENTS(id, COMPONENT_EFFECT) && effect_kind_of(id) == ef) {
            return id;
        }
    }
    return NGE_ENTITY_INVALID;
}

/* A fresh goblin (6 hp, AC 13, speed 10, resists blunt, vulnerable to fire; no innate effect) */
static nge_entity_id_t goblin(void)
{
    nge_entity_id_t id = gen_creature(CR_GOBLIN, AREA);

    events();
    return id;
}

/* Start a new world: every entity free, the clock at 0 */
static void reset(void)
{
    nge_entity_init();
    nge_timer_init();
    nge_event_init();
    fired = 0;
}

int main(void)
{
    char text[32];
    nge_entity_id_t g;
    nge_entity_id_t e;
    nge_entity_id_t e2;
    nge_entity_id_t ring;
    nge_entity_id_t player;
    uint8_t i;
    uint8_t free_before;
    int ok;
    int k;

    test_init();

    /* ---- the definitions: well formed, each duration with only what it uses ---- */
    ok = 1;
    for (i = 1; i < EF_COUNT; i++) {
        const effect_def_t *d = &effect_defs[i];
        int has_mods = 0;
        uint8_t m;

        ok = ok && d->duration < EFFECT_DURATIONS && (d->flags & ~EFFECT_STACK) == 0;
        ok = ok && d->on_start.op < OP_COUNT && d->on_tick.op < OP_COUNT && d->on_end.op < OP_COUNT;
        for (m = 0; m < EFFECT_MODS; m++) {
            ok = ok && d->mod[m].attr < ATTR_COUNT;
            has_mods = has_mods || d->mod[m].amount != 0;
        }
        switch (d->duration) {
        case EFFECT_INSTANT:   /* its on_start op only: nothing lasts */
            ok = ok && d->on_start.op != OP_NONE && d->on_tick.op == OP_NONE && d->on_end.op == OP_NONE &&
                 !has_mods && d->status == 0 && d->flags == 0 && d->period == 0 && d->cycles == 0 &&
                 d->end_text == NULL;
            break;
        case EFFECT_TIMED:
            ok = ok && d->period != 0;
            break;
        default:               /* worn: modifiers and statuses only */
            ok = ok && d->on_start.op == OP_NONE && d->on_tick.op == OP_NONE && d->on_end.op == OP_NONE &&
                 (has_mods || d->status != 0) && d->flags == 0 && d->period == 0 && d->cycles == 0 &&
                 d->start_text == NULL && d->end_text == NULL;
            break;
        }
    }
    CHECK(ok);
    CHECK(effect_defs[EF_NONE].duration == EFFECT_INSTANT && effect_defs[EF_NONE].flags == 0 &&
          effect_defs[EF_NONE].status == 0 && effect_defs[EF_NONE].on_start.op == OP_NONE &&
          effect_defs[EF_NONE].mod[0].amount == 0);
    CHECK(effect_defs[EF_RING_HEALTH].duration == EFFECT_WORN && effect_defs[EF_POISON].duration == EFFECT_TIMED);

    /* ---- instant: healing, no entity left behind ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 4, DMG_SLASH);
    events();
    free_before = nge_entity_free_count();
    CHECK(effect_apply(EF_HEALING, g, NGE_ENTITY_INVALID) == 1);
    CHECK(health_hp[g] == 6 && nge_entity_free_count() == free_before);   /* healed 4 (of 6), capped */
    CHECK(events() == 2 && is(0, g, EV_HEALED, NGE_EVENT_NONE, 4) &&
          is(1, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_HEALING));

    /* ---- refused: not alive, no health, dead ---- */
    e = nge_entity_create();                                       /* no health */
    CHECK(effect_apply(EF_HEALING, e, NGE_ENTITY_INVALID) == 0);
    CHECK(effect_apply(EF_POISON, e, NGE_ENTITY_INVALID) == 0 && count_effects(e, EF_POISON) == 0);
    health_damage(g, NGE_ENTITY_INVALID, 50, DMG_SLASH);           /* dying */
    events();
    CHECK(effect_apply(EF_POISON, g, NGE_ENTITY_INVALID) == 0 && events() == 0);
    player = gen_creature(CR_PLAYER, AREA);                       /* persistent: alive at 0 hp */
    health_damage(player, NGE_ENTITY_INVALID, 50, DMG_SLASH);
    events();
    CHECK(NGE_ENTITY_IS_ALIVE(player) && effect_apply(EF_HEALING, player, NGE_ENTITY_INVALID) == 0 &&
          health_hp[player] == 0);

    /* ---- timed: poison, an entity owned by the target with a timer ---- */
    reset();
    g = goblin();
    CHECK(effect_apply(EF_POISON, g, NGE_ENTITY_INVALID) == 1);
    e = effect_on(g, EF_POISON);
    CHECK(e != NGE_ENTITY_INVALID && NGE_OWNER_OF(e) == g && effect_kind_of(e) == EF_POISON);
    CHECK(NGE_ENTITY_HAS_COMPONENTS(e, NGE_COMPONENT_TIMER) && nge_timer_kind[e] == TIMER_EFFECT &&
          nge_timer_period[e] == 30 && nge_timer_cycles[e] == 4 && nge_timer_count[e] == 30);
    CHECK(STATUS_HAS(g, STATUS_POISONED) && effects_check(g));
    CHECK(events() == 2 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_STARTED, e, EF_POISON));
    /* Each firing: 1 poison damage, caused by the effect */
    fire(1);
    CHECK(nge_timer_turns == 3 && health_hp[g] == 5);
    CHECK(events() == 1 && is(0, g, EV_DAMAGED, e, 1));
    fire(2);
    CHECK(health_hp[g] == 3 && NGE_ENTITY_IS_ALIVE(e) && events() == 2);
    /* The 4th and last: the damage, then it ends: gone, the status cleared */
    fire(1);
    CHECK(nge_timer_turns == 12 && health_hp[g] == 2);
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && !STATUS_HAS(g, STATUS_POISONED) && effects_check(g));
    CHECK(events() == 3 && is(0, g, EV_DAMAGED, e, 1) && is(1, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(2, g, EV_EFFECT_ENDED, e, EF_POISON));
    fire(1);
    CHECK(health_hp[g] == 2);                                      /* no more firings */

    /* ---- refresh: poison again restarts the one there ---- */
    reset();
    g = goblin();
    effect_apply(EF_POISON, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    fire(2);
    events();
    free_before = nge_entity_free_count();
    CHECK(nge_timer_cycles[e] == 2);
    CHECK(effect_apply(EF_POISON, g, NGE_ENTITY_INVALID) == 1);
    CHECK(count_effects(g, EF_POISON) == 1 && nge_entity_free_count() == free_before);
    CHECK(nge_timer_cycles[e] == 4 && nge_timer_count[e] == 30);   /* its full time again */
    CHECK(events() == 1 && is(0, g, EV_EFFECT_STARTED, e, EF_POISON));

    /* ---- stack: regeneration twice is two effects, each healing ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 5, DMG_SLASH);
    CHECK(effect_apply(EF_REGEN, g, NGE_ENTITY_INVALID) == 1 && effect_apply(EF_REGEN, g, NGE_ENTITY_INVALID) == 1);
    CHECK(count_effects(g, EF_REGEN) == 2);
    events();
    fire(2);
    CHECK(nge_timer_turns == 10 && health_hp[g] == 3);             /* both fired on turn 10 */
    fire(4);
    CHECK(health_hp[g] == 6 && count_effects(g, EF_REGEN) == 2);   /* for ever: full, and still there */

    /* ---- cure: ends the effects granting the status, then the cure's own event ---- */
    reset();
    g = goblin();
    effect_apply(EF_POISON, g, NGE_ENTITY_INVALID);
    effect_apply(EF_REGEN, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    events();
    CHECK(effect_apply(EF_CURE, g, NGE_ENTITY_INVALID) == 1);
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && !STATUS_HAS(g, STATUS_POISONED) && count_effects(g, EF_REGEN) == 1);
    CHECK(events() == 3 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_ENDED, e, EF_POISON) && is(2, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_CURE));
    CHECK(effect_cure(g, STATUS_POISONED) == 0);                   /* nothing left to cure */
    effect_apply(EF_POISON, g, NGE_ENTITY_INVALID);
    CHECK(effect_cure(g, 0x80) == 0 && STATUS_HAS(g, STATUS_POISONED));   /* another status: kept */
    CHECK(effect_cure(g, STATUS_POISONED) == 1 && !STATUS_HAS(g, STATUS_POISONED));
    fire(3);
    CHECK(health_hp[g] == 6);                                      /* the ended poison never fires */

    /* ---- permanent: raising a base, maximum only (no healing) ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 2, DMG_SLASH);
    events();
    CHECK(effect_apply(EF_VITALITY, g, NGE_ENTITY_INVALID) == 1);
    CHECK(health_hp_max_base[g] == 7 && health_hp_max[g] == 7 && health_hp[g] == 4);
    CHECK(events() == 2 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_VITALITY));
    CHECK(count_effects(g, EF_VITALITY) == 0);                     /* nothing left behind */
    effects_set_base(g, ATTR_HP_MAX, 255);
    effect_apply(EF_VITALITY, g, NGE_ENTITY_INVALID);
    CHECK(health_hp_max_base[g] == 255 && health_hp_max[g] == 255);   /* at most 255 */

    /* ---- worn: an equipped ring counts, stowed or on the floor it doesn't ---- */
    reset();
    g = goblin();
    ring = gen_item(IT_RING, AREA);
    effects_recompute(g);
    CHECK(health_hp_max[g] == 6 && events() == 0);                 /* nothing changed: no event */
    inv_stow(ring, g);
    effects_recompute(g);
    CHECK(health_hp_max[g] == 6 && events() == 0);                 /* stowed */
    item_slot[ring] = SLOT_RING_L;
    CHECK(!effects_check(g));                                      /* not recomputed yet */
    effects_recompute(g);
    CHECK(health_hp_max[g] == 8 && health_hp[g] == 6 && effects_check(g));   /* a higher maximum doesn't heal */
    CHECK(events() == 1 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0));
    effects_recompute(g);
    CHECK(events() == 0);                                          /* the same again: nothing changed */
    health_heal(g, 2);
    CHECK(health_hp[g] == 8);
    item_slot[ring] = SLOT_NONE;
    effects_recompute(g);
    CHECK(health_hp_max[g] == 6 && health_hp[g] == 6);            /* hp kept within the lower maximum */
    item_slot[ring] = SLOT_RING_R;
    e = gen_item(IT_RING, AREA);
    inv_stow(e, g);
    item_slot[e] = SLOT_RING_L;
    effects_recompute(g);
    CHECK(health_hp_max[g] == 10);                                 /* two rings: both count */
    effects_set_base(g, ATTR_HP_MAX, 254);
    CHECK(health_hp_max[g] == 255);                                /* 254 + 4: at most 255 */
    effects_set_base(g, ATTR_HP_MAX, 6);
    CHECK(health_hp_max[g] == 10);
    nge_entity_destroy(e);
    effects_recompute(g);
    CHECK(health_hp_max[g] == 8);                                  /* a dying item doesn't */

    /* ---- the other attributes: clamped; speed sets the turn timer's period ---- */
    events();
    effects_set_base(g, ATTR_SPEED, 5);
    CHECK(stats_speed[g] == 5 && nge_timer_period[g] == 5 && effects_check(g));
    effects_set_base(g, ATTR_SPEED, 0);
    CHECK(stats_speed[g] == 1 && nge_timer_period[g] == 1);       /* at least 1 */
    effects_set_base(g, ATTR_AC, 17);
    CHECK(health_ac[g] == 17);
    effects_set_base(g, ATTR_HP_MAX, 0);
    CHECK(health_hp_max[g] == 0 + 2 && health_hp[g] == 2);        /* base 0, plus the ring */
    item_slot[ring] = SLOT_NONE;
    effects_recompute(g);
    CHECK(health_hp_max[g] == 1 && health_hp[g] == 1);            /* at least 1 */
    item_slot[ring] = SLOT_RING_R;
    effects_recompute(g);
    effects_set_base(g, ATTR_IMMUNE, DMG_BIT(DMG_ACID));
    effects_set_base(g, ATTR_RESIST, DMG_BIT(DMG_FIRE));
    effects_set_base(g, ATTR_VULN, DMG_BIT(DMG_COLD) | DMG_BIT(DMG_MAGIC));
    CHECK(health_immune[g] == DMG_BIT(DMG_ACID) && health_resist[g] == DMG_BIT(DMG_FIRE) &&
          health_vuln[g] == (DMG_BIT(DMG_COLD) | DMG_BIT(DMG_MAGIC)) && effects_check(g));
    CHECK(events() == 9);                                          /* one EV_ATTRS_CHANGED each */
    /* Abilities: clamped to 1-30 */
    CHECK(stats_ability[ABIL_STR][g] == 8 && stats_ability_base[ABIL_STR][g] == 8);   /* a goblin's */
    effects_set_base(g, ATTR_CHA, 200);
    CHECK(stats_ability[ABIL_CHA][g] == 30 && stats_ability_base[ABIL_CHA][g] == 200);
    effects_set_base(g, ATTR_CHA, 0);
    CHECK(stats_ability[ABIL_CHA][g] == 1 && effects_check(g));
    effects_set_base(g, ATTR_CHA, 8);
    /* A potion of strength: +1 base strength, for good */
    events();
    CHECK(effect_apply(EF_STRENGTH, g, NGE_ENTITY_INVALID) == 1);
    CHECK(stats_ability_base[ABIL_STR][g] == 9 && stats_ability[ABIL_STR][g] == 9 && stats_ability[ABIL_DEX][g] == 14);
    CHECK(events() == 2 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_STRENGTH));
    effect_text(EF_STRENGTH, EFFECT_TEXT_START, text, sizeof(text));
    CHECK_STR(text, "You feel strong!");
    stats_ability_base[ABIL_STR][g] = 30;
    effect_apply(EF_STRENGTH, g, NGE_ENTITY_INVALID);
    CHECK(stats_ability_base[ABIL_STR][g] == 31 && stats_ability[ABIL_STR][g] == 30);   /* the base may pass 30, the score can't */
    effects_set_base(g, ATTR_STR, 9);
    /* effects_check() notices every derived value out of step */
    health_ac[g]++;
    k = effects_check(g);
    health_ac[g]--;
    health_resist[g] ^= 1;
    k = k || effects_check(g);
    health_resist[g] ^= 1;
    stats_speed[g]++;
    k = k || effects_check(g);
    stats_speed[g]--;
    stats_status[g] ^= STATUS_POISONED;
    k = k || effects_check(g);
    stats_status[g] ^= STATUS_POISONED;
    for (i = 0; i < ABIL_COUNT; i++) {
        stats_ability[i][g]++;
        k = k || effects_check(g);
        stats_ability[i][g]--;
    }
    health_hp_max[g]--;
    k = k || effects_check(g);
    health_hp_max[g]++;
    CHECK(k == 0 && effects_check(g));
    /* Checking changes nothing, even hit points above the maximum */
    health_hp[g] = (uint8_t)(health_hp_max[g] + 1);
    CHECK(!effects_check(g) && health_hp[g] == health_hp_max[g] + 1);
    effects_recompute(g);
    CHECK(health_hp[g] == health_hp_max[g] && effects_check(g));
    /* The game's attributes: each a base table and a component; the damage masks are ORed, the rest added */
    ok = 1;
    for (i = 0; i < ATTR_COUNT; i++) {
        ok = ok && effects_game_attrs[i].base != NULL && effects_game_attrs[i].component != 0 &&
             effects_game_attrs[i].is_mask == (i == ATTR_IMMUNE || i == ATTR_RESIST || i == ATTR_VULN);
    }
    CHECK(ok);
    CHECK(effects_game_attrs[ATTR_HP_MAX].base == health_hp_max_base &&
          effects_game_attrs[ATTR_HP_MAX].component == COMPONENT_HEALTH &&
          effects_game_attrs[ATTR_SPEED].base == stats_speed_base &&
          effects_game_attrs[ATTR_SPEED].component == COMPONENT_STATS &&
          effects_game_attrs[ATTR_ABILITY(ABIL_CHA)].base == stats_ability_base[ABIL_CHA]);
    CHECK(effects_game_timer_kind == TIMER_EFFECT);

    /* ---- a target that dies: its effects stop acting, and no end event ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 5, DMG_SLASH);           /* 1 hp left */
    effect_apply(EF_POISON, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    events();
    fire(1);
    CHECK(NGE_ENTITY_STATE(g) == NGE_ENTITY_DYING && NGE_ENTITY_IS_ALIVE(e));   /* released with it (game.c) */
    CHECK(events() == 2 && seen[0].id == EV_DAMAGED && seen[1].id == EV_DIED);
    fire(3);
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && find_event(0, EV_EFFECT_ENDED) < 0 && events() == 0);
    effects_recompute(g);                                          /* a dead entity: nothing */
    CHECK(events() == 0);

    /* ---- health but no stats: abilities and speed left alone, no statuses, no stats bases ---- */
    reset();
    g = goblin();
    stats_remove(g);
    stats_ability[ABIL_STR][g] = 99;                               /* not its attribute now: left alone */
    stats_speed[g] = 99;
    stats_status[g] = 0;
    effects_recompute(g);
    CHECK(stats_ability[ABIL_STR][g] == 99 && stats_speed[g] == 99 && effects_check(g));
    events();
    CHECK(effect_apply(EF_POISON, g, NGE_ENTITY_INVALID) == 1);
    fire(1);
    CHECK(health_hp[g] == 5 && stats_status[g] == 0 && effects_check(g));   /* damaged; no status stored */
    TEST_EXPECT_HALT(effects_set_base(g, ATTR_STR, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effect_apply(EF_STRENGTH, g, NGE_ENTITY_INVALID));    /* raising a base it hasn't */
    CHECK(test_halt_count == 1);

    /* ---- the player: their innate regeneration ---- */
    reset();
    player = gen_creature(CR_PLAYER, AREA);
    CHECK(count_effects(player, EF_REGEN) == 1);
    CHECK(events() == 1 && is(0, player, EV_EFFECT_STARTED, effect_on(player, EF_REGEN), EF_REGEN));
    CHECK(gen_hook(player, GEN_HOOK_INNATE) == EF_REGEN);

    /* ---- no entity free: a timed effect is refused, an instant one isn't ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 3, DMG_SLASH);
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    events();
    CHECK(effect_apply(EF_POISON, g, NGE_ENTITY_INVALID) == 0 && !STATUS_HAS(g, STATUS_POISONED) && events() == 0);
    CHECK(effect_apply(EF_HEALING, g, NGE_ENTITY_INVALID) == 1 && health_hp[g] == 6);

    /* ---- texts ---- */
    effect_text(EF_POISON, EFFECT_TEXT_START, text, sizeof(text));
    CHECK_STR(text, "You feel sick.");
    effect_text(EF_POISON, EFFECT_TEXT_END, text, sizeof(text));
    CHECK_STR(text, "The poison wears off.");
    effect_text(EF_HEALING, EFFECT_TEXT_START, text, sizeof(text));
    CHECK_STR(text, "You feel much better.");
    effect_text(EF_REGEN, EFFECT_TEXT_START, text, sizeof(text));
    CHECK_STR(text, "");                                           /* none: regeneration is silent */
    effect_text(EF_POISON, EFFECT_TEXT_START, text, 5);
    CHECK_STR(text, "You ");                                       /* cut to fit */
    effect_text(EF_POISON, EFFECT_TEXT_START, text, 1);
    CHECK_STR(text, "");

    /* ---- asserts ---- */
    reset();
    g = goblin();
    e = nge_entity_create();
    TEST_EXPECT_HALT(effect_apply(EF_NONE, g, NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effect_apply(EF_COUNT, g, NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);
    free_before = nge_entity_free_count();
    TEST_EXPECT_HALT(effect_apply(EF_RING_HEALTH, g, NGE_ENTITY_INVALID));   /* worn: never applied */
    CHECK(test_halt_count == 1 && health_hp_max[g] == 6 && nge_entity_free_count() == free_before);
    TEST_EXPECT_HALT(effect_end(e));                               /* not an effect */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effects_set_base(g, ATTR_COUNT, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effects_set_base(e, ATTR_AC, 1));             /* no health */
    CHECK(test_halt_count == 1);
    effect_apply(EF_POISON, g, NGE_ENTITY_INVALID);
    e2 = effect_on(g, EF_POISON);
    effect_end(e2);
    TEST_EXPECT_HALT(effect_end(e2));                              /* ended already */
    CHECK(test_halt_count == 1);

    return test_summary("effects");
}
