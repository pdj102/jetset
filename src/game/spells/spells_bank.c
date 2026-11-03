/*
 * The spells' functions: banked in page 134, with their data in page 135
 * (spells_pdata.c). See docs/spells.md.
 */
#ifdef __SDCC
#pragma codeseg PAGE_134
#pragma constseg PAGE_134
#endif

#include "game/spells/spells_internal.h"

#include <string.h>

#include "core/nge_util.h"
#include "event/nge_event.h"
#include "game/events.h"

/* Spell sp's byte and bit in a set: the cast keeps zsdcc from making 1 << 7 a negative int (T-INT-2) */
#define BYTE_OF(sp)  ((uint8_t)((sp) >> 3))
#define BIT_OF(sp)   ((uint8_t)(1u << ((sp) & 7)))

/* An entity in use with the spells component */
static void check_caster(nge_entity_id_t id)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(SPELLS_IS(id));
}

static uint8_t knows(nge_entity_id_t id, uint8_t sp)
{
    return (spell_known[id][BYTE_OF(sp)] & BIT_OF(sp)) != 0;
}

void spell_add(nge_entity_id_t id, const uint8_t *list, uint8_t count) NGE_BANKED
{
    uint8_t i;
    uint8_t sp;

    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);

    for (i = 0; i < SPELL_SET_BYTES; i++) {
        spell_known[id][i] = 0;   /* nothing from an entity that had this ID before */
    }
    for (i = 0; i < count; i++) {
        sp = list[i];
        nge_util_assert(sp < SPELL_COUNT);
        if (sp != SP_NONE) {
            spell_known[id][BYTE_OF(sp)] |= BIT_OF(sp);
        }
    }
    NGE_ENTITY_ADD_COMPONENT(id, COMPONENT_SPELLS);
}

uint8_t spell_knows(nge_entity_id_t id, uint8_t sp) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(sp < SPELL_COUNT);
    return sp != SP_NONE && SPELLS_IS(id) && knows(id, sp);
}

void spell_learn(nge_entity_id_t id, uint8_t sp) NGE_BANKED
{
    check_caster(id);
    nge_util_assert(sp != SP_NONE && sp < SPELL_COUNT);
    if (knows(id, sp)) {
        return;
    }
    spell_known[id][BYTE_OF(sp)] |= BIT_OF(sp);
    nge_event_post(id, EV_SPELL_LEARNED, NGE_EVENT_NONE, sp, 0);
}

void spell_learn_level(nge_entity_id_t id, uint8_t level) NGE_BANKED
{
    uint8_t sp;

    check_caster(id);
    if (level == 0) {
        return;   /* a row with learn 0 is never learned this way */
    }
    for (sp = 1; sp < SPELL_COUNT; sp++) {
        if (spell_defs[sp].learn == level) {
            spell_learn(id, sp);
        }
    }
}

uint8_t spell_next(nge_entity_id_t id, uint8_t sp, uint8_t ai) NGE_BANKED
{
    uint8_t n;

    check_caster(id);
    nge_util_assert(sp < SPELL_COUNT);
    for (n = 1; n < SPELL_COUNT; n++) {
        sp = (sp + 1 < SPELL_COUNT) ? (uint8_t)(sp + 1) : 1;   /* wrapping past SP_NONE */
        if (knows(id, sp) && (ai == SPELL_AI_ANY || spell_defs[sp].ai == ai)) {
            return sp;
        }
    }
    return SP_NONE;
}

void spell_get(uint8_t sp, spell_def_t *def) NGE_BANKED
{
    nge_util_assert(sp < SPELL_COUNT);
    memcpy(def, &spell_defs[sp], sizeof(*def));
}

uint8_t spell_cost(uint8_t level) NGE_BANKED
{
    nge_util_assert(level <= SPELL_LEVEL_MAX);
    return spell_costs[level];
}

void spell_name(uint8_t sp, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(sp < SPELL_COUNT);
    nge_util_assert(size >= 1);
    buf[0] = '\0';
    if (spell_defs[sp].name != NULL) {
        strncat(buf, spell_defs[sp].name, size - 1);   /* common code: it may read this page's text */
    }
}
