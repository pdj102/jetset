/* nge-dataseg: PAGE_53 */
/*
 * Render system page data, in page 53 (slot 7 while the system's code in
 * page 52 runs): the sprite component and the system's working data.
 * Uninitialised: nge_render_init() and nge_render_add() set it. See "Page
 * data" in docs/memory_map.md.
 */
#include "ecs/systems/nge_render_internal.h"

/* Sprite component */
uint8_t nge_render_sprite_first[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_flags[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_mode[NGE_ENTITY_MAX];
int8_t  nge_render_sprite_ox[NGE_ENTITY_MAX];
int8_t  nge_render_sprite_oy[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_layer[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_anim_count[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_anim_rate[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_anim_tick[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_anim_frame[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_rest_first[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_rest_count[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_rest_rate[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_oneshot[NGE_ENTITY_MAX];
int8_t  nge_render_sprite_mox[NGE_ENTITY_MAX];
int8_t  nge_render_sprite_moy[NGE_ENTITY_MAX];
int8_t  nge_render_sprite_mtx[NGE_ENTITY_MAX];
int8_t  nge_render_sprite_mty[NGE_ENTITY_MAX];
uint8_t nge_render_sprite_mspeed[NGE_ENTITY_MAX];

/* System working data */
nge_sprite_attr_t nge_render_attrs[NGE_RENDER_SPRITES];
nge_entity_id_t   nge_render_vis[NGE_ENTITY_MAX];
uint16_t          nge_render_sx[NGE_ENTITY_MAX];
uint16_t          nge_render_sy[NGE_ENTITY_MAX];
uint8_t           nge_render_used;
uint8_t           nge_render_shown;
uint8_t           nge_render_dropped_count;
nge_owner_t       nge_render_area;
uint8_t           nge_render_busy_count;
nge_entity_id_t   nge_render_follow_id;
uint16_t          nge_render_follow_x;
uint16_t          nge_render_follow_y;
