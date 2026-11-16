/*
 * The noise module: banked in page 142 with its table (a banked game
 * module, see "Banked game modules" in docs/memory_map.md). Which events
 * make noise, where and how loud; who hears it is the AI's (ai_hear(),
 * game/ai/ai_hear.c). See docs/noise.md, 5.1 and 5.2.
 */
#ifdef __SDCC
#pragma codeseg PAGE_142
#pragma constseg PAGE_142
#endif

#include "game/noise/noise.h"

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"

#include "game/ai/ai.h"
#include "game/components/pc.h"

#define ROW(ev)  ((ev) - NGE_EVENT_USER)

/* By event, from NGE_EVENT_USER; rows left out are silent (docs/noise.md 5.1, to tune in play) */
static const noise_row_t noise_table[NOISE_EVENTS] = {
    [ROW(EV_MOVED)]     = { 4, NOISE_STEP },
    [ROW(EV_HIT)]       = { 6, 0 },
    [ROW(EV_MISSED)]    = { 6, 0 },
    [ROW(EV_DIED)]      = { 6, NOISE_ANYONE },
    [ROW(EV_FIRED)]     = { 3, NOISE_AT_SOURCE },
    [ROW(EV_CAST)]      = { 5, NOISE_AT_SOURCE },
    [ROW(EV_AREA)]      = { 8, NOISE_AT_P1P2 | NOISE_ANYONE },
    [ROW(EV_TRIGGERED)] = { 8, NOISE_ANYONE },
    [ROW(EV_OPENED)]    = { 3, 0 },
    [ROW(EV_CLOSED)]    = { 3, 0 },
    [ROW(EV_UNLOCKED)]  = { 2, 0 },
};

/* The player's step: the one place its loudness is read (R12) */
static uint8_t step_loudness(void)
{
    return noise_table[ROW(EV_MOVED)].loud;
}

/* id is the player */
static uint8_t is_player(nge_entity_id_t id)
{
    return id < NGE_ENTITY_MAX && PC_IS(id);
}

/*
 * id is placed in an area: in use, so a creature dying of the blow still
 * has its tile for the hit and its death cry
 */
static uint8_t placed(nge_entity_id_t id)
{
    return id < NGE_ENTITY_MAX && NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE &&
           NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) && NGE_OWNER_IS_AREA(NGE_OWNER_OF(id));
}

void noise_init(void) NGE_BANKED
{
    uint8_t i;
    uint8_t bit = 1;   /* row i's bit in its byte: no variable shift, so no library routine */

    for (i = 0; i < sizeof(noise_mask); i++) {
        noise_mask[i] = 0;
    }
    for (i = 0; i < NOISE_EVENTS; i++) {
        if (noise_table[i].loud != 0) {
            noise_mask[i >> 3] |= bit;
        }
        bit = (uint8_t)(bit << 1);
        if (bit == 0) {
            bit = 1;
        }
    }
}

void noise_event(const nge_event_t *e) NGE_BANKED
{
    nge_entity_id_t target = e->target;   /* locals: see T-ZSDCC-1 */
    nge_entity_id_t source = e->source;
    uint8_t p1 = e->p1;
    uint8_t p2 = e->p2;
    uint8_t i = (uint8_t)(e->id - NGE_EVENT_USER);
    uint8_t loud;
    uint8_t flags;
    nge_entity_id_t at;
    ai_noise_t n;

    if (i >= NOISE_EVENTS) {
        return;
    }
    loud = noise_table[i].loud;
    flags = noise_table[i].flags;
    if (loud == 0) {
        return;
    }
    /* Monsters are allies: their steps, doors and blows don't count (D6) */
    if (!(flags & NOISE_ANYONE) && !is_player(source) && !is_player(target)) {
        return;
    }
    if (flags & NOISE_STEP) {
        if (p1 == 0 && p2 == 0) {
            return;   /* a teleport or a push: no step (R5) */
        }
        loud = step_loudness();
    }
    at = (flags & (NOISE_AT_SOURCE | NOISE_AT_P1P2)) ? source : target;
    if (!placed(at)) {
        return;
    }
    n.area = NGE_OWNER_OF(at);
    if (flags & NOISE_AT_P1P2) {
        n.x = p1;
        n.y = p2;
    } else {
        n.x = NGE_LOCATION_X(at);
        n.y = NGE_LOCATION_Y(at);
    }
    n.loud = loud;
    n.source = source;
    n.target = target;
    ai_hear(&n);
}

uint8_t noise_step_loudness(void) NGE_BANKED
{
    return step_loudness();
}
