/*
 * The effects module's common code: nge_effect's dispatch function, called
 * through a pointer (ecs/systems/nge_effect.h), so in common memory. It
 * makes the banked call to the module in page 120.
 */
#include "game/effects/effects_internal.h"

void effects_hook(const nge_effect_hook_t *h)
{
    effects_on_hook(h);
}
