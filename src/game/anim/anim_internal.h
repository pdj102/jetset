#ifndef ANIM_INTERNAL_H
#define ANIM_INTERNAL_H

/*
 * Shared by the anim module's files; not for use outside it:
 * - anim_bank.c (page 126): starting the animations;
 * - anim_vfx.c (page 126): starting the visual effects (docs/vfx.md), with
 *   their tables;
 * - anim_pdata.c (page 127): the creature kinds' sequences, mapped into
 *   slot 7 while its code runs.
 */

#include "game/anim/anim.h"
#include "game/gen/gen.h"

/* Each creature kind's sequences, by CR_* and GEN_SEQ_*, copied by anim_init() */
extern gen_seq_t anim_seqs[CR_COUNT][GEN_SEQ_COUNT];

/*
 * An area's VFX (docs/items_aimed.md 4.8), in page 127: the effect of the
 * last EV_AIMED or EV_CAST with no target, for EV_AREA (EF_NONE before any);
 * and while anim_area_held, the centre's map pixels, where a bolt was sent:
 * impacts and glows elsewhere wait for it. Forgotten at the next EV_FIRED,
 * EV_CAST, EV_AIMED or EV_AREA, and by anim_vfx_clear()
 */
extern uint8_t anim_area_ef;
extern uint8_t anim_area_held;
extern uint16_t anim_area_x;
extern uint16_t anim_area_y;

/* Start the VFX an event shows (anim_vfx.c): called by anim_on_event(), in the same page */
void anim_vfx_event(nge_event_id_t ev, nge_entity_id_t target, nge_entity_id_t source, uint8_t p1, uint8_t p2);

#endif
