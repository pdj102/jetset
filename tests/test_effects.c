/* game/effects, game/attrs: applying, timing, ending and recomputing effects. */
#include "test.h"

#include <stdlib.h>
#include <string.h>

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/attrs/attrs_internal.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/mana.h"
#include "game/components/stats.h"
#include "game/effects/effects_internal.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
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

static nge_entity_id_t effect_on(nge_entity_id_t target, uint8_t ef);

/* Move time on until target has no effect of kind ef, or 1000 steps: other effects may fire meanwhile */
static void run_out(nge_entity_id_t target, uint8_t ef)
{
    int steps = 0;

    while (effect_on(target, ef) != NGE_ENTITY_INVALID && steps++ < 1000) {
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
        if (NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_EFFECT) && EFFECT_KIND_OF(id) == ef) {
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
        if (NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_EFFECT) && EFFECT_KIND_OF(id) == ef) {
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

/* A seed whose first n d20 rolls (1 + rand() % 20, a saving throw's) are all lo to hi */
static unsigned int seed_for_rolls(int n, int lo, int hi)
{
    unsigned int s;
    int i;
    int r;
    int ok;

    for (s = 1;; s++) {
        srand(s);
        ok = 1;
        for (i = 0; i < n && ok; i++) {
            r = 1 + rand() % 20;
            ok = r >= lo && r <= hi;
        }
        if (ok) {
            srand(s);
            return s;
        }
    }
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
    int r;

    test_init();

    /* ---- the definitions: well formed, each duration with only what it uses ---- */
    ok = 1;
    for (i = 1; i < EF_COUNT; i++) {
        const effect_def_t *d = &effect_defs[i];
        int has_mods = 0;
        uint8_t m;

        ok = ok && d->duration < EFFECT_DURATIONS && (d->flags & ~(EFFECT_STACK | EFFECT_INNATE)) == 0;
        ok = ok && (!(d->flags & EFFECT_INNATE) || (d->duration == EFFECT_TIMED && d->cycles == 0 && d->save.dc == 0));
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
                 (has_mods || d->status != 0 || i == EF_WORN_REGEN || i == EF_WORN_LIGHT) && d->flags == 0 && d->period == 0 && d->cycles == 0 &&
                 d->start_text == NULL && d->end_text == NULL;
            break;
        }
    }
    CHECK(ok);
    CHECK(effect_defs[EF_NONE].duration == EFFECT_INSTANT && effect_defs[EF_NONE].flags == 0 &&
          effect_defs[EF_NONE].status == 0 && effect_defs[EF_NONE].on_start.op == OP_NONE &&
          effect_defs[EF_NONE].mod[0].amount == 0);
    CHECK(effect_defs[EF_WORN_HP].duration == EFFECT_WORN && effect_defs[EF_POISON].duration == EFFECT_TIMED);

    /* Timed effects come first (draw_effects() sizes by them) */
    ok = 1;
    for (i = 1; i < EF_COUNT; i++) {
        ok = ok && (effect_defs[i].duration == EFFECT_TIMED) == (i < EF_TIMED_COUNT);
    }
    CHECK(ok);

    /* ---- instant: healing, no entity left behind ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 4, DMG_SLASH);
    events();
    free_before = nge_entity_free_count();
    CHECK(effect_apply(EF_HEALING, 6, g, NGE_ENTITY_INVALID) == 1);
    CHECK(health_hp[g] == 6 && nge_entity_free_count() == free_before);   /* healed 4 (of 6), capped */
    CHECK(events() == 2 && is(0, g, EV_HEALED, NGE_EVENT_NONE, 4) &&
          is(1, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_HEALING));

    /* ---- refused: not alive, no health, dead ---- */
    e = nge_entity_create();                                       /* no health */
    CHECK(effect_apply(EF_HEALING, 6, e, NGE_ENTITY_INVALID) == 0);
    CHECK(effect_apply(EF_POISON, 1, e, NGE_ENTITY_INVALID) == 0 && count_effects(e, EF_POISON) == 0);
    health_damage(g, NGE_ENTITY_INVALID, 50, DMG_SLASH);           /* dying */
    events();
    CHECK(effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID) == 0 && events() == 0);
    player = gen_creature(CR_PLAYER, AREA);                       /* persistent: alive at 0 hp */
    health_damage(player, NGE_ENTITY_INVALID, 50, DMG_SLASH);
    events();
    CHECK(NGE_ENTITY_IS_ALIVE(player) && effect_apply(EF_HEALING, 6, player, NGE_ENTITY_INVALID) == 0 &&
          health_hp[player] == 0);

    /* ---- timed: poison, an entity owned by the target with a timer ---- */
    reset();
    g = goblin();
    CHECK(effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID) == 1);
    e = effect_on(g, EF_POISON);
    CHECK(e != NGE_ENTITY_INVALID && NGE_OWNER_OF(e) == g && EFFECT_KIND_OF(e) == EF_POISON && EFFECT_POWER_OF(e) == 1);
    CHECK(NGE_ENTITY_HAS_COMPONENT(e, NGE_COMPONENT_TIMER) && nge_timer_kind[e] == TIMER_EFFECT &&
          nge_timer_period[e] == 30 && nge_timer_cycles[e] == 4 && nge_timer_count[e] == 30);
    CHECK(STATUS_HAS(g, STATUS_POISONED) && effects_check(g));
    CHECK(events() == 2 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_STARTED, e, EF_POISON));
    /* Each firing: 1 poison damage, caused by the effect; every saving throw fails (Con 10 against DC 10) */
    seed_for_rolls(4, 1, 9);
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
          is(2, g, EV_EFFECT_ENDED, e, EF_POISON) && seen[2].p2 == EFFECT_END_EXPIRED);
    fire(1);
    CHECK(health_hp[g] == 2);                                      /* no more firings */

    /* ---- a saving throw passed: the firing's damage, then it ends early ---- */
    reset();
    g = goblin();
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    events();
    seed_for_rolls(1, 10, 10);                                     /* just reaches DC 10 */
    fire(1);
    CHECK(health_hp[g] == 5 && !NGE_ENTITY_IS_ALIVE(e) && !STATUS_HAS(g, STATUS_POISONED) && effects_check(g));
    CHECK(events() == 3 && is(0, g, EV_DAMAGED, e, 1) && is(1, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(2, g, EV_EFFECT_ENDED, e, EF_POISON) && seen[2].p2 == EFFECT_END_SAVED);
    fire(3);
    CHECK(health_hp[g] == 5);                                      /* no more firings */

    /* ---- saving throws: 1d20 + the ability's modifier >= DC ---- */
    seed_for_rolls(1, 9, 9);
    CHECK(effects_save(g, ABIL_CON, 10) == 0);                /* Con 10: +0 */
    seed_for_rolls(1, 10, 10);
    CHECK(effects_save(g, ABIL_CON, 10) == 1);
    seed_for_rolls(1, 10, 10);
    CHECK(effects_save(g, ABIL_CON, 11) == 0);                /* a higher DC */
    stats_ability[ABIL_CON][g] = 30;                               /* +10: a 1 passes */
    seed_for_rolls(1, 1, 1);
    CHECK(effects_save(g, ABIL_CON, 10) == 1);
    stats_ability[ABIL_CON][g] = 1;                                /* -5 */
    seed_for_rolls(1, 14, 14);
    CHECK(effects_save(g, ABIL_CON, 10) == 0);
    seed_for_rolls(1, 15, 15);
    CHECK(effects_save(g, ABIL_CON, 10) == 1);
    seed_for_rolls(1, 9, 9);
    CHECK(effects_save(g, ABIL_STR, 7) == 1);                 /* the ability named: Str 8, -1 */
    stats_remove(g);                                               /* no stats: +0, whatever its scores */
    seed_for_rolls(1, 14, 14);
    CHECK(effects_save(g, ABIL_CON, 10) == 1);
    seed_for_rolls(1, 9, 9);
    CHECK(effects_save(g, ABIL_CON, 10) == 0);
    TEST_EXPECT_HALT(effects_save(g, ABIL_COUNT, 10));
    CHECK(test_halt_count == 1);

    /* ---- no saving throw (DC 0): no roll ---- */
    reset();
    g = goblin();
    effect_apply(EF_REGEN, 1, g, NGE_ENTITY_INVALID);
    srand(5);
    r = rand();
    srand(5);
    fire(1);
    CHECK(rand() == r && count_effects(g, EF_REGEN) == 1);

    /* ---- refresh: poison again restarts the one there ---- */
    reset();
    g = goblin();
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    seed_for_rolls(2, 1, 9);                                       /* its saving throws fail */
    fire(2);
    events();
    free_before = nge_entity_free_count();
    CHECK(nge_timer_cycles[e] == 2);
    CHECK(effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(count_effects(g, EF_POISON) == 1 && nge_entity_free_count() == free_before);
    CHECK(nge_timer_cycles[e] == 4 && nge_timer_count[e] == 30);   /* its full time again */
    CHECK(events() == 1 && is(0, g, EV_EFFECT_STARTED, e, EF_POISON));

    /* ---- stack: regeneration twice is two effects, each healing ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 5, DMG_SLASH);
    CHECK(effect_apply(EF_REGEN, 1, g, NGE_ENTITY_INVALID) == 1 && effect_apply(EF_REGEN, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(count_effects(g, EF_REGEN) == 2);
    events();
    fire(2);
    CHECK(nge_timer_turns == 10 && health_hp[g] == 3);             /* both fired on turn 10 */
    fire(4);
    CHECK(health_hp[g] == 6 && count_effects(g, EF_REGEN) == 2);   /* for ever: full, and still there */

    /* ---- cure: ends the effects granting the status, then the cure's own event ---- */
    reset();
    g = goblin();
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
    effect_apply(EF_REGEN, 1, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    events();
    CHECK(effect_apply(EF_CURE, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(!NGE_ENTITY_IS_ALIVE(e) && !STATUS_HAS(g, STATUS_POISONED) && count_effects(g, EF_REGEN) == 1);
    CHECK(events() == 3 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_ENDED, e, EF_POISON) && seen[1].p2 == EFFECT_END_CURED &&
          is(2, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_CURE));
    CHECK(effect_cure(g, STATUS_POISONED) == 0);                   /* nothing left to cure */
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
    CHECK(effect_cure(g, 0x80) == 0 && STATUS_HAS(g, STATUS_POISONED));   /* another status: kept */
    CHECK(effect_cure(g, STATUS_POISONED) == 1 && !STATUS_HAS(g, STATUS_POISONED));
    fire(3);
    CHECK(health_hp[g] == 6);                                      /* the ended poison never fires */

    /* ---- innate: no cure ends it, and a cure still ends what else it has ---- */
    reset();
    g = goblin();
    effect_apply(EF_ROOT, 1, g, NGE_ENTITY_INVALID);
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
    CHECK(STATUS_HAS(g, STATUS_ROOTED));
    CHECK(effect_cure(g, STATUS_ROOTED) == 0 && STATUS_HAS(g, STATUS_ROOTED));
    CHECK(effect_apply(EF_CURE_ALL, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(!STATUS_HAS(g, STATUS_POISONED) && STATUS_HAS(g, STATUS_ROOTED) && count_effects(g, EF_ROOT) == 1);
    /* A hold on the same status is cured; the roots stay */
    effect_apply(EF_HOLD, 1, g, NGE_ENTITY_INVALID);
    CHECK(effect_cure(g, STATUS_ROOTED) == 1 && count_effects(g, EF_HOLD) == 0 && STATUS_HAS(g, STATUS_ROOTED));
    /* A hold alone: over with its time or its saving throw */
    reset();
    g = goblin();
    effect_apply(EF_HOLD, 1, g, NGE_ENTITY_INVALID);
    CHECK(STATUS_HAS(g, STATUS_ROOTED));
    fire(6);
    CHECK(!STATUS_HAS(g, STATUS_ROOTED) && count_effects(g, EF_HOLD) == 0);

    /* ---- permanent: raising a base, maximum only (no healing) ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 2, DMG_SLASH);
    events();
    CHECK(effect_apply(EF_VITALITY, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(health_hp_max_base[g] == 7 && health_hp_max[g] == 7 && health_hp[g] == 4);
    CHECK(events() == 2 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_VITALITY));
    CHECK(count_effects(g, EF_VITALITY) == 0);                     /* nothing left behind */
    effects_set_base(g, ATTR_HP_MAX, 255);
    effect_apply(EF_VITALITY, 1, g, NGE_ENTITY_INVALID);
    CHECK(health_hp_max_base[g] == 255 && health_hp_max[g] == 255);   /* at most 255 */

    /* ---- worn: an equipped ring counts, stowed or on the floor it doesn't ---- */
    reset();
    g = goblin();
    ring = gen_item(IT_RING_HEALTH_2, AREA);
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
    e = gen_item(IT_RING_HEALTH_2, AREA);
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
    CHECK(effect_apply(EF_STRENGTH, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(stats_ability_base[ABIL_STR][g] == 9 && stats_ability[ABIL_STR][g] == 9 && stats_ability[ABIL_DEX][g] == 14);
    CHECK(events() == 2 && is(0, g, EV_ATTRS_CHANGED, NGE_EVENT_NONE, 0) &&
          is(1, g, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_STRENGTH));
    effect_text(EF_STRENGTH, EFFECT_TEXT_START, text, sizeof(text));
    CHECK_STR(text, "You feel strong!");
    stats_ability_base[ABIL_STR][g] = 30;
    effect_apply(EF_STRENGTH, 1, g, NGE_ENTITY_INVALID);
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
        ok = ok && attrs_table[i].base != NULL && attrs_table[i].component != 0 &&
             attrs_table[i].is_mask == (i == ATTR_IMMUNE || i == ATTR_RESIST || i == ATTR_VULN);
    }
    CHECK(ok);
    CHECK(attrs_table[ATTR_HP_MAX].base == health_hp_max_base &&
          attrs_table[ATTR_HP_MAX].component == COMPONENT_HEALTH &&
          attrs_table[ATTR_SPEED].base == stats_speed_base &&
          attrs_table[ATTR_SPEED].component == COMPONENT_STATS &&
          attrs_table[ATTR_ABILITY(ABIL_CHA)].base == stats_ability_base[ABIL_CHA]);

    /* ---- effects_base() and effects_put_base(): the bases outside page 138 ---- */
    reset();
    g = goblin();
    ok = 1;
    for (i = 0; i < ATTR_COUNT; i++) {
        if (NGE_ENTITY_HAS_COMPONENT(g, attrs_table[i].component)) {
            ok = ok && effects_base(g, i) == attrs_table[i].base[g];
        }
    }
    CHECK(ok);
    /* No effects: each base is its value */
    k = health_hp_max[g];
    CHECK(effects_base(g, ATTR_HP_MAX) == k && effects_base(g, ATTR_AC) == health_ac[g] &&
          effects_base(g, ATTR_SPEED) == stats_speed[g] && effects_base(g, ATTR_ABILITY(ABIL_STR)) == 8);
    /* Stored without recomputing: the value stays, so the check sees a stale value until a recompute */
    effects_put_base(g, ATTR_HP_MAX, (uint8_t)(k + 3));
    effects_put_base(g, ATTR_ABILITY(ABIL_STR), 12);
    CHECK(effects_base(g, ATTR_HP_MAX) == k + 3 && health_hp_max_base[g] == k + 3 && health_hp_max[g] == k);
    CHECK(effects_base(g, ATTR_ABILITY(ABIL_STR)) == 12 && stats_ability[ABIL_STR][g] == 8);
    CHECK(!effects_check(g) && events() == 0);
    effects_recompute(g);
    CHECK(health_hp_max[g] == k + 3 && stats_ability[ABIL_STR][g] == 12 && effects_check(g));

    /* ---- a target that dies: its effects stop acting, and no end event ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 5, DMG_SLASH);           /* 1 hp left */
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
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
    CHECK(effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID) == 1);
    fire(1);
    CHECK(health_hp[g] == 5 && stats_status[g] == 0 && effects_check(g));   /* damaged; no status stored */
    TEST_EXPECT_HALT(effects_set_base(g, ATTR_STR, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effect_apply(EF_STRENGTH, 1, g, NGE_ENTITY_INVALID));    /* raising a base it hasn't */
    CHECK(test_halt_count == 1);

    /* ---- the player: their innate regeneration, then mana regeneration (a rule: they have mana) ---- */
    reset();
    player = gen_creature(CR_PLAYER, AREA);
    CHECK(count_effects(player, EF_REGEN) == 1 && count_effects(player, EF_MANA_REGEN) == 1);
    CHECK(events() == 2 && is(0, player, EV_EFFECT_STARTED, effect_on(player, EF_REGEN), EF_REGEN) &&
          is(1, player, EV_EFFECT_STARTED, effect_on(player, EF_MANA_REGEN), EF_MANA_REGEN));
    CHECK(GEN_HOOK_EF(gen_hook_of(player, GEN_HOOK_INNATE)) == EF_REGEN);

    /* ---- mana regeneration: 1 MP every 20 turns, stopping at the maximum; innate, so no cure ends it ---- */
    e = effect_on(player, EF_MANA_REGEN);
    CHECK(effect_defs[EF_MANA_REGEN].flags == EFFECT_INNATE && effect_defs[EF_MANA_REGEN].cycles == 0);
    mana_spend(player, 2);
    events();
    while (mana_mp[player] < 4 && nge_timer_turns < 100) {
        nge_timer_advance(dispatch);
    }
    CHECK(NGE_ENTITY_IS_ALIVE(e) && mana_mp[player] == 4 && nge_timer_turns == 20);         /* the first firing, at turn 20 */
    events();
    CHECK(find_event(0, EV_MANA_RESTORED) >= 0);
    effect_apply(EF_CURE_ALL, 1, player, NGE_ENTITY_INVALID);
    CHECK(count_effects(player, EF_MANA_REGEN) == 1);
    while (mana_mp[player] < 5 && nge_timer_turns < 100) {
        nge_timer_advance(dispatch);
    }
    CHECK(mana_mp[player] == 5 && nge_timer_turns == 40);
    events();
    while (nge_timer_turns < 60) {
        nge_timer_advance(dispatch);
    }
    events();
    CHECK(mana_mp[player] == 5 && find_event(0, EV_MANA_RESTORED) < 0 && count_effects(player, EF_MANA_REGEN) == 1);

    /* ---- maximum MP: an attribute, at least 1; a lower maximum takes MP down with it, a higher restores none ---- */
    events();
    CHECK(attrs_table[ATTR_MP_MAX].base == mana_mp_max_base &&
          attrs_table[ATTR_MP_MAX].component == COMPONENT_MANA);
    effects_set_base(player, ATTR_MP_MAX, 9);
    CHECK(mana_mp_max_base[player] == 9 && mana_mp_max[player] == 9 && mana_mp[player] == 5 && effects_check(player));
    effects_set_base(player, ATTR_MP_MAX, 3);
    CHECK(mana_mp_max[player] == 3 && mana_mp[player] == 3);
    effects_set_base(player, ATTR_MP_MAX, 0);
    CHECK(mana_mp_max[player] == 1 && mana_mp[player] == 1);
    CHECK(events() == 3);                                          /* one EV_ATTRS_CHANGED each */
    mana_mp_max[player]++;
    CHECK(!effects_check(player));
    mana_mp_max[player]--;
    CHECK(effects_check(player));
    g = goblin();                                                  /* no mana: the attribute is left alone */
    mana_mp_max[g] = 77;
    effects_recompute(g);
    CHECK(mana_mp_max[g] == 77 && count_effects(g, EF_MANA_REGEN) == 0);
    TEST_EXPECT_HALT(effects_set_base(g, ATTR_MP_MAX, 1));
    CHECK(test_halt_count == 1);

    /* ---- no entity free: a timed effect is refused, an instant one isn't ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 3, DMG_SLASH);
    while (nge_entity_create() != NGE_ENTITY_INVALID) {
    }
    events();
    CHECK(effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID) == 0 && !STATUS_HAS(g, STATUS_POISONED) && events() == 0);
    CHECK(effect_apply(EF_HEALING, 6, g, NGE_ENTITY_INVALID) == 1 && health_hp[g] == 6);

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
    effect_text(EF_POISON, EFFECT_TEXT_NAME, text, sizeof(text));
    CHECK_STR(text, "Poison");
    effect_text(EF_REGEN, EFFECT_TEXT_NAME, text, sizeof(text));
    CHECK_STR(text, "Regeneration");
    effect_text(EF_HEALING, EFFECT_TEXT_NAME, text, sizeof(text));
    CHECK_STR(text, "");                                           /* instant: nothing lasts to name */
    ok = 1;
    for (i = 1; i < EF_COUNT; i++) {
        effect_text(i, EFFECT_TEXT_NAME, text, sizeof(text));
        ok = ok && strlen(text) <= EFFECT_NAME_MAX &&
             (effect_defs[i].duration != EFFECT_TIMED || text[0] != '\0');   /* every timed effect named */
    }
    CHECK(ok);

    /* ---- saving throws ---- */
    {
        uint8_t ability = 99;

        CHECK(effect_save_of(EF_POISON, &ability) == 10 && ability == ABIL_CON);
        CHECK(effect_save_of(EF_REGEN, &ability) == 0);
    }

    /* ---- power: worn, by the item's hook ---- */
    reset();
    g = goblin();
    ring = gen_item(IT_RING_HEALTH_2, g);
    item_slot[ring] = SLOT_RING_L;
    e = gen_item(IT_RING_FRAILTY, g);
    item_slot[e] = SLOT_RING_R;
    effects_recompute(g);
    CHECK(health_hp_max[g] == 6 + 2 - 3 && health_hp[g] == 5 && effects_check(g));    /* two add up; a negative power */
    e = gen_item(IT_PLATE_ARMOUR_2, g);
    item_slot[e] = SLOT_BODY;
    k = health_ac[g];
    effects_recompute(g);
    CHECK(health_ac[g] == k + 9);
    e = gen_item(IT_RING_FIRE, g);
    item_slot[ring] = SLOT_NONE;
    item_slot[e] = SLOT_RING_L;
    effects_recompute(g);
    CHECK(health_resist[g] == (DMG_BIT(DMG_BLUNT) | DMG_BIT(DMG_FIRE)));   /* a mask, ORed on its own: not scaled */
    /* Timed modifiers scale, and go with the effect */
    reset();
    g = goblin();
    k = stats_speed[g];
    effect_apply(EF_HASTE, 3, g, NGE_ENTITY_INVALID);
    CHECK(stats_speed[g] == k - 3 && effects_check(g));
    effect_end(effect_on(g, EF_HASTE));
    CHECK(stats_speed[g] == k);
    /* Refreshed stronger: the modifier follows at once */
    effect_apply(EF_MIGHT, 1, g, NGE_ENTITY_INVALID);
    k = stats_ability[ABIL_STR][g];
    effect_apply(EF_MIGHT, 3, g, NGE_ENTITY_INVALID);
    CHECK(stats_ability[ABIL_STR][g] == k + 2 && count_effects(g, EF_MIGHT) == 1);

    /* ---- uncurse: everything equipped, not what is stowed or on the floor; worn effects still count ---- */
    reset();
    g = goblin();
    ring = gen_item(IT_RING_FRAILTY, g);
    item_slot[ring] = SLOT_RING_L;
    effects_recompute(g);
    k = health_hp_max[g];
    e = gen_item(IT_RUSTY_SWORD, g);
    CHECK(inv_cursed(ring) && inv_cursed(e));
    CHECK(effect_apply(EF_UNCURSE, 1, g, NGE_ENTITY_INVALID) == 1);
    CHECK(!inv_cursed(ring) && inv_cursed(e) && health_hp_max[g] == k);

    /* ---- pending effects: safe to apply, doing nothing yet ---- */
    reset();
    g = goblin();
    for (i = EF_LIGHT; i <= EF_SUMMON; i++) {
        CHECK(effect_apply(i, 1, g, NGE_ENTITY_INVALID) == 1 && health_hp[g] == 6);
    }
    /* Mana: restored for what has it, nothing for what hasn't */
    CHECK(effect_apply(EF_MANA, 3, g, NGE_ENTITY_INVALID) == 1);
    player = gen_creature(CR_PLAYER, AREA);
    if (MANA_IS(player)) {
        mana_mp[player] = 0;
        CHECK(effect_apply(EF_MANA, 2, player, NGE_ENTITY_INVALID) == 1 && mana_mp[player] == 2);
    }

    /* ---- power: instant, the amount scales ---- */
    reset();
    g = goblin();
    health_damage(g, NGE_ENTITY_INVALID, 5, DMG_SLASH);
    CHECK(effect_apply(EF_HEALING, 2, g, NGE_ENTITY_INVALID) == 1 && health_hp[g] == 3);
    i = health_hp_max_base[g];
    CHECK(effect_apply(EF_VITALITY, 3, g, NGE_ENTITY_INVALID) == 1 && health_hp_max_base[g] == i + 3);
    CHECK(effect_apply(EF_VITALITY, -2, g, NGE_ENTITY_INVALID) == 1 && health_hp_max_base[g] == i + 1);
    CHECK(effects_check(g));
    /* Past what an int8_t amount holds: clamped, not wrapped */
    health_damage(g, NGE_ENTITY_INVALID, 2, DMG_SLASH);
    CHECK(effect_apply(EF_HEALING, 127, g, NGE_ENTITY_INVALID) == 1 && health_hp[g] == health_hp_max[g]);

    /* ---- power: timed, kept for every firing, after what started it has gone ---- */
    reset();
    g = goblin();
    effect_apply(EF_POISON, 2, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_POISON);
    CHECK(EFFECT_POWER_OF(e) == 2);
    srand(seed_for_rolls(1, 1, 1));
    fire(1);
    CHECK(health_hp[g] == 4);
    /* Refreshed: the stronger power stays, the weaker can't dilute it */
    CHECK(effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID) == 1 && EFFECT_POWER_OF(e) == 2);
    CHECK(effect_apply(EF_POISON, 3, g, NGE_ENTITY_INVALID) == 1 && EFFECT_POWER_OF(e) == 3 && count_effects(g, EF_POISON) == 1);
    /* Stacked: each instance keeps its own */
    reset();
    g = goblin();
    effect_apply(EF_REGEN, 1, g, NGE_ENTITY_INVALID);
    e = effect_on(g, EF_REGEN);
    effect_apply(EF_REGEN, 2, g, NGE_ENTITY_INVALID);
    CHECK(count_effects(g, EF_REGEN) == 2 && EFFECT_POWER_OF(e) == 1);

    /* ---- level teleport (docs/effects_extensions.md 3.2) ---- */
    /* The player at depth 1: never up (R3), so always a request for depth 2, anywhere */
    reset();
    level_clear_request();
    level_depth = 1;
    player = gen_creature(CR_PLAYER, LEVEL_AREA);
    ok = 1;
    for (k = 0; k < 20 && ok; k++) {
        uint8_t depth;
        uint8_t arrival;

        ok = effect_apply(EF_LEVEL_TELEPORT, 1, player, NGE_ENTITY_INVALID) == 1 && level_pending();
        events();
        if (ok) {
            level_take_request(&depth, &arrival);
            ok = depth == 2 && arrival == LEVEL_ARRIVE_ANY;
        }
    }
    CHECK(ok);
    /* At the bottom: always up */
    level_depth = LEVEL_DEPTH_BOTTOM;
    nge_owner_set(player, LEVEL_AREA);   /* the player goes with the level */
    ok = 1;
    for (k = 0; k < 20 && ok; k++) {
        uint8_t depth;
        uint8_t arrival;

        ok = effect_apply(EF_LEVEL_TELEPORT, 1, player, NGE_ENTITY_INVALID) == 1 && level_pending();
        events();
        if (ok) {
            level_take_request(&depth, &arrival);
            ok = depth == LEVEL_DEPTH_BOTTOM - 1;
        }
    }
    CHECK(ok);
    /* Delayed descent at the bottom: it fires once, after 15 turns, and fizzles */
    events();
    CHECK(effect_apply(EF_DESCENT, 1, player, NGE_ENTITY_INVALID) == 1);
    e = effect_on(player, EF_DESCENT);
    CHECK(e != NGE_ENTITY_INVALID && nge_timer_period[e] == 150 && nge_timer_cycles[e] == 1);
    run_out(player, EF_DESCENT);
    CHECK(!level_pending() && effect_on(player, EF_DESCENT) == NGE_ENTITY_INVALID);
    events();
    r = find_event(0, EV_FIZZLED);
    CHECK(r >= 0 && is(r, player, EV_FIZZLED, NGE_EVENT_NONE, OP_LEVEL_TELEPORT) &&
          find_event(r, EV_EFFECT_ENDED) > r);   /* the op, then the ending */
    /* Recall at power 10 from the middle: depth 1, never 0 (R6) */
    level_depth = 5;
    nge_owner_set(player, LEVEL_AREA);
    effect_apply(EF_RECALL, 10, player, NGE_ENTITY_INVALID);
    run_out(player, EF_RECALL);
    {
        uint8_t depth = 0;
        uint8_t arrival = 0;

        CHECK(level_pending());
        level_take_request(&depth, &arrival);
        CHECK(depth == 1 && arrival == LEVEL_ARRIVE_ANY);
    }
    /* Delayed descent at power 2 from the middle: two down */
    effect_apply(EF_DESCENT, 2, player, NGE_ENTITY_INVALID);
    run_out(player, EF_DESCENT);
    {
        uint8_t depth = 0;
        uint8_t arrival = 0;

        CHECK(level_pending());
        level_take_request(&depth, &arrival);
        CHECK(depth == 7);
    }
    /* A monster leaves the level, up or down one (R4) */
    g = gen_creature(CR_GOBLIN, LEVEL_AREA);
    events();
    CHECK(effect_apply(EF_LEVEL_TELEPORT, 1, g, NGE_ENTITY_INVALID) == 1 && !NGE_ENTITY_IS_ALIVE(g) && !level_pending());
    events();
    r = find_event(0, EV_LEFT_LEVEL);
    CHECK(r >= 0 && seen[r].target == g && (seen[r].p1 == 4 || seen[r].p1 == 6) && find_event(0, EV_FIZZLED) < 0);
    /* A persistent one is immune: it fizzles and stays */
    g = gen_creature(CR_GOBLIN, LEVEL_AREA);
    nge_entity_set_flags(g, NGE_ENTITY_PERSISTENT);
    events();
    CHECK(effect_apply(EF_LEVEL_TELEPORT, 1, g, NGE_ENTITY_INVALID) == 1 && NGE_ENTITY_IS_ALIVE(g) && level_holds(g));
    events();
    r = find_event(0, EV_FIZZLED);
    CHECK(r >= 0 && is(r, g, EV_FIZZLED, NGE_EVENT_NONE, OP_LEVEL_TELEPORT) && find_event(0, EV_LEFT_LEVEL) < 0);
    /* Parked with a delayed descent: it ends, and its on_end does nothing off the level (D3) */
    effect_apply(EF_DESCENT, 1, g, NGE_ENTITY_INVALID);
    events();
    level_park(g, NGE_OWNER_AREA(6));
    CHECK(effect_on(g, EF_DESCENT) == NGE_ENTITY_INVALID && NGE_ENTITY_IS_ALIVE(g) && !level_pending());
    events();
    CHECK(find_event(0, EV_FIZZLED) < 0 && find_event(0, EV_LEFT_LEVEL) < 0 && find_event(0, EV_EFFECT_ENDED) >= 0);
    level_depth = 0;   /* AREA again, for what follows */

    /* ---- asserts ---- */
    reset();
    g = goblin();
    e = nge_entity_create();
    TEST_EXPECT_HALT(effect_apply(EF_NONE, 1, g, NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effect_apply(EF_COUNT, 1, g, NGE_ENTITY_INVALID));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effect_apply(EF_POISON, 0, g, NGE_ENTITY_INVALID));       /* a power of 0 does nothing: a slip */
    CHECK(test_halt_count == 1);
    free_before = nge_entity_free_count();
    TEST_EXPECT_HALT(effect_apply(EF_WORN_HP, 1, g, NGE_ENTITY_INVALID));   /* worn: never applied */
    CHECK(test_halt_count == 1 && health_hp_max[g] == 6 && nge_entity_free_count() == free_before);
    TEST_EXPECT_HALT(effect_end(e));                               /* not an effect */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effects_set_base(g, ATTR_COUNT, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effect_text(EF_POISON, EFFECT_TEXT_COUNT, text, sizeof(text)));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(effects_set_base(e, ATTR_AC, 1));             /* no health */
    CHECK(test_halt_count == 1);
    effect_apply(EF_POISON, 1, g, NGE_ENTITY_INVALID);
    e2 = effect_on(g, EF_POISON);
    effect_end(e2);
    TEST_EXPECT_HALT(effect_end(e2));                              /* ended already */
    CHECK(test_halt_count == 1);

    return test_summary("effects");
}
