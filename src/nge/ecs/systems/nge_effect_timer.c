/*
 * The effect system's timer trampoline, in common memory: nge_timer fires
 * NGE_TIMER_EFFECT through a function pointer, which can't switch banks,
 * so this makes the banked call to the system in page 58.
 */
#include "ecs/systems/nge_effect_internal.h"

void nge_effect_timer(nge_entity_id_t effect, uint8_t cycles)
{
    nge_effect_on_timer(effect, cycles);
}
