#ifndef NGE_RENDER_INTERNAL_H
#define NGE_RENDER_INTERNAL_H

/*
 * Shared by the render system's files; not for use outside it:
 * - nge_render.c (page 52): the system's code;
 * - nge_render_pdata.c (page 53): the sprite component and the system's
 *   working data below, mapped into slot 7 while its code runs.
 */

#include "ecs/systems/nge_render.h"
#include "sprite/nge_sprite.h"

/* Sprite component, one table per field, indexed by entity ID */
extern uint8_t nge_render_first[NGE_ENTITY_MAX];       /* pattern, or first animation pattern */
extern uint8_t nge_render_flags[NGE_ENTITY_MAX];       /* mirror, rotate, palette */
extern uint8_t nge_render_mode[NGE_ENTITY_MAX];        /* scale, NGE_SPRITE_PATTERN4 */
extern int8_t  nge_render_ox[NGE_ENTITY_MAX];          /* draw offset, pixels */
extern int8_t  nge_render_oy[NGE_ENTITY_MAX];
extern uint8_t nge_render_layer[NGE_ENTITY_MAX];
extern uint8_t nge_render_anim_count[NGE_ENTITY_MAX];  /* patterns in the animation, 1: still */
extern uint8_t nge_render_anim_rate[NGE_ENTITY_MAX];   /* frames per pattern */
extern uint8_t nge_render_anim_tick[NGE_ENTITY_MAX];   /* frames shown of the current pattern */
extern uint8_t nge_render_anim_frame[NGE_ENTITY_MAX];  /* current pattern - first */

/* Working data */
extern nge_sprite_attr_t nge_render_attrs[NGE_RENDER_SPRITES];  /* records for sprites 0.. */
extern nge_entity_id_t   nge_render_vis[NGE_ENTITY_MAX];        /* visible entities this update */
extern uint16_t          nge_render_sx[NGE_ENTITY_MAX];         /* ...and their screen positions */
extern uint16_t          nge_render_sy[NGE_ENTITY_MAX];
extern uint8_t           nge_render_used;      /* sprites drawn by the last update */
extern uint8_t           nge_render_shown;     /* sprites visible on the hardware after the last upload */
extern uint8_t           nge_render_dropped_count;
extern nge_owner_t       nge_render_area;
extern const nge_map_t  *nge_render_map;

#endif
