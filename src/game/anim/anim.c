/*
 * The anim module's common part: whether the world waits, asked by the
 * turn engine. The animations are started in anim_bank.c.
 */
#include "game/anim/anim.h"

#include "ecs/systems/nge_render.h"

uint8_t anim_holding(void)
{
    return nge_render_busy();   /* later: and the overlay visuals (projectiles, deaths) */
}
