/*
 * The anim module's common part: whether the world waits, asked by the
 * turn engine. The animations are started in anim_bank.c, the visual
 * effects in anim_vfx.c.
 */
#include "game/anim/anim.h"

#include "ecs/systems/nge_render.h"
#include "vfx/nge_vfx.h"

uint8_t anim_holding(void)
{
    return nge_render_busy() || nge_vfx_busy();
}
