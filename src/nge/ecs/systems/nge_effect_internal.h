#ifndef NGE_EFFECT_INTERNAL_H
#define NGE_EFFECT_INTERNAL_H

/*
 * Shared by the effect system's files; not for use outside it (except the
 * tests):
 * - nge_effect.c (page 58): the system's code;
 * - nge_effect_pdata.c (page 59): the effect component and the hook,
 *   mapped into slot 7 while its code runs;
 * - nge_effect_timer.c (common): the trampoline nge_timer fires
 *   NGE_TIMER_EFFECT through.
 */

#include "ecs/systems/nge_effect.h"

/* Effect component (NGE_COMPONENT_EFFECT): one table per field, indexed by entity ID */
extern uint8_t nge_effect_kinds[NGE_ENTITY_MAX];
extern int8_t  nge_effect_powers[NGE_ENTITY_MAX];   /* not 0: what started it may be gone */

/* The game's dispatch function, from nge_effect_init() */
extern nge_effect_hook_fn nge_effect_game_hook;

/* NGE_TIMER_EFFECT's handler, in common memory: calls nge_effect_on_timer() */
void nge_effect_timer(nge_entity_id_t effect, uint8_t cycles);

/* An active effect's timer fired: the TICK hook, then its end on its last cycle */
void nge_effect_on_timer(nge_entity_id_t effect, uint8_t cycles) NGE_BANKED;

#endif
