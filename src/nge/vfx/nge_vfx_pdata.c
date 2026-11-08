/* nge-dataseg: PAGE_93 */
/*
 * VFX page data, in page 93 (slot 7 while the module's code in page 92
 * runs): the slots and their sprite records. Uninitialised:
 * nge_vfx_init() sets it. See "Page data" in docs/memory_map.md.
 */
#include "vfx/nge_vfx_internal.h"

uint8_t  nge_vfx_kind[NGE_VFX_SLOTS];
uint16_t nge_vfx_x[NGE_VFX_SLOTS];
uint16_t nge_vfx_y[NGE_VFX_SLOTS];
uint16_t nge_vfx_ex[NGE_VFX_SLOTS];
uint16_t nge_vfx_ey[NGE_VFX_SLOTS];
uint16_t nge_vfx_dx[NGE_VFX_SLOTS];
uint16_t nge_vfx_dy[NGE_VFX_SLOTS];
int16_t  nge_vfx_err[NGE_VFX_SLOTS];
uint16_t nge_vfx_left[NGE_VFX_SLOTS];
int8_t   nge_vfx_sx[NGE_VFX_SLOTS];
int8_t   nge_vfx_sy[NGE_VFX_SLOTS];
uint8_t  nge_vfx_speed[NGE_VFX_SLOTS];
uint8_t  nge_vfx_first[NGE_VFX_SLOTS];
uint8_t  nge_vfx_count[NGE_VFX_SLOTS];
uint8_t  nge_vfx_rate[NGE_VFX_SLOTS];
uint8_t  nge_vfx_tick[NGE_VFX_SLOTS];
uint8_t  nge_vfx_frame[NGE_VFX_SLOTS];
uint8_t  nge_vfx_flags[NGE_VFX_SLOTS];
uint8_t  nge_vfx_mode[NGE_VFX_SLOTS];
uint8_t  nge_vfx_delay[NGE_VFX_SLOTS];

nge_sprite_attr_t nge_vfx_attrs[NGE_VFX_SLOTS];
uint8_t nge_vfx_sprite;
uint8_t nge_vfx_active;
uint8_t nge_vfx_drawn;
uint8_t nge_vfx_shown;
