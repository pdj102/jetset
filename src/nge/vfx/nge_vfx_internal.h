#ifndef NGE_VFX_INTERNAL_H
#define NGE_VFX_INTERNAL_H

/*
 * Shared by the VFX module's files; not for use outside it (the PC test
 * reads them):
 * - nge_vfx.c (page 92): the module's code;
 * - nge_vfx_pdata.c (page 93): the slots and sprite records below, mapped
 *   into slot 7 while its code runs.
 *
 * One table per field, indexed by slot. A bolt moves by a Bresenham step:
 * a pixel at a time along its long axis (major), with the error term
 * (err) deciding when the short axis follows.
 */

#include "vfx/nge_vfx.h"
#include "sprite/nge_sprite.h"

/* What a slot holds */
#define NGE_VFX_FREE   0
#define NGE_VFX_BURST  1
#define NGE_VFX_BOLT   2

extern uint8_t  nge_vfx_kind[NGE_VFX_SLOTS];     /* NGE_VFX_FREE, _BURST or _BOLT */
extern uint16_t nge_vfx_x[NGE_VFX_SLOTS];        /* map pixels now */
extern uint16_t nge_vfx_y[NGE_VFX_SLOTS];
extern uint16_t nge_vfx_ex[NGE_VFX_SLOTS];       /* a bolt's end */
extern uint16_t nge_vfx_ey[NGE_VFX_SLOTS];
extern uint16_t nge_vfx_dx[NGE_VFX_SLOTS];       /* a bolt's distance on each axis, pixels */
extern uint16_t nge_vfx_dy[NGE_VFX_SLOTS];
extern int16_t  nge_vfx_err[NGE_VFX_SLOTS];      /* a bolt's error term */
extern uint16_t nge_vfx_left[NGE_VFX_SLOTS];     /* a bolt's pixels left on its long axis */
extern int8_t   nge_vfx_sx[NGE_VFX_SLOTS];       /* a bolt's step on each axis: -1, 0 or 1 */
extern int8_t   nge_vfx_sy[NGE_VFX_SLOTS];
extern uint8_t  nge_vfx_speed[NGE_VFX_SLOTS];    /* a bolt's pixels a frame */
extern uint8_t  nge_vfx_first[NGE_VFX_SLOTS];    /* the pattern run, as in the look */
extern uint8_t  nge_vfx_count[NGE_VFX_SLOTS];
extern uint8_t  nge_vfx_rate[NGE_VFX_SLOTS];
extern uint8_t  nge_vfx_tick[NGE_VFX_SLOTS];     /* frames shown of the current pattern */
extern uint8_t  nge_vfx_frame[NGE_VFX_SLOTS];    /* current pattern - first */
extern uint8_t  nge_vfx_flags[NGE_VFX_SLOTS];    /* sprite flags, with a directed bolt's own */
extern uint8_t  nge_vfx_mode[NGE_VFX_SLOTS];     /* sprite mode and NGE_VFX_HOLD */
extern uint8_t  nge_vfx_delay[NGE_VFX_SLOTS];    /* frames still to wait */

extern nge_sprite_attr_t nge_vfx_attrs[NGE_VFX_SLOTS];   /* records for the module's sprites */
extern uint8_t nge_vfx_sprite;     /* the first hardware sprite */
extern uint8_t nge_vfx_active;     /* slots in use */
extern uint8_t nge_vfx_drawn;      /* sprites the last update made visible */
extern uint8_t nge_vfx_shown;      /* sprites visible on the hardware after the last upload */

#endif
