/*
 * The anim module: banked in page 126, with the creature kinds' sequences
 * in page 127 (anim_pdata.c). It turns events into the render system's
 * slides, lunges and one-shots.
 */
#ifdef __SDCC
#pragma codeseg PAGE_126
#pragma constseg PAGE_126
#endif

#include "game/anim/anim_internal.h"

#include "ecs/components/nge_location.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "map/nge_map.h"

#include "game/components/kind.h"
#include "game/events.h"
#include "game/level/level.h"

void anim_init(void) NGE_BANKED
{
    gen_seq_t seq[GEN_SEQ_COUNT];   /* on the stack: the creature table's page is mapped over this one's */
    uint8_t kind;
    uint8_t s;

    for (kind = 0; kind < CR_COUNT; kind++) {
        gen_creature_seqs(kind, seq);
        for (s = 0; s < GEN_SEQ_COUNT; s++) {
            anim_seqs[kind][s] = seq[s];
        }
    }
}

/* Drawn on the current level: the only entities an animation can show */
static uint8_t shown(nge_entity_id_t id)
{
    return id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id) &&
           NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) && NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_SPRITE) &&
           NGE_OWNER_OF(id) == LEVEL_AREA;
}

/* Play a creature's sequence for an action (GEN_SEQ_*), if its kind has one */
static void play(nge_entity_id_t id, uint8_t action)
{
    const gen_seq_t *seq;
    uint8_t count;

    if (!KIND_IS(id, CLASS_CREATURE)) {
        return;
    }
    seq = &anim_seqs[kind_index[id]][action];
    count = seq->count;
    if (count != 0) {
        nge_render_play(id, seq->first, count, seq->rate);
    }
}

/* -1, 0 or 1: the direction from a to b on one axis */
static int8_t sign(uint8_t a, uint8_t b)
{
    return (b > a) ? 1 : (b < a) ? -1 : 0;
}

void anim_on_event(const nge_event_t *e) NGE_BANKED
{
    /* Locals, not fields: see T-ZSDCC-1 */
    nge_event_id_t ev = e->id;
    nge_entity_id_t target = e->target;
    nge_entity_id_t source = e->source;
    int8_t dx = (int8_t)e->p1;
    int8_t dy = (int8_t)e->p2;

    switch (ev) {
    case EV_MOVED:
        /* From the old tile: (dx, dy) tiles back */
        if (shown(target)) {
            nge_render_slide(target, (int8_t)(-dx * NGE_MAP_TILE_PIXELS), (int8_t)(-dy * NGE_MAP_TILE_PIXELS),
                             ANIM_SLIDE_SPEED);
            play(target, GEN_SEQ_MOVE);
        }
        break;
    case EV_HIT:
    case EV_MISSED:
        /* The attacker lunges towards what it attacked */
        if (shown(source) && target < NGE_ENTITY_MAX && NGE_ENTITY_HAS_COMPONENT(target, NGE_COMPONENT_LOCATION)) {
            dx = sign(NGE_LOCATION_X(source), NGE_LOCATION_X(target));
            dy = sign(NGE_LOCATION_Y(source), NGE_LOCATION_Y(target));
            if (dx != 0 || dy != 0) {
                nge_render_lunge(source, (int8_t)(dx * ANIM_LUNGE_PIXELS), (int8_t)(dy * ANIM_LUNGE_PIXELS),
                                 ANIM_LUNGE_SPEED);
                play(source, GEN_SEQ_ATTACK);
            }
        }
        break;
    default:
        break;
    }
}
