#ifndef NGE_RENDER_INTERNAL_H
#define NGE_RENDER_INTERNAL_H

/*
 * Shared by the render system's files; not for use outside it:
 * - nge_render.c (page 52): the system's code;
 * - nge_render_pdata.c (page 53): the sprite component and the system's
 *   working data below, mapped into slot 7 while its code runs.
 *
 * Names: nge_render_sprite_* is the sprite component's data (per entity,
 * set by the game through the functions in nge_render.h); the other
 * nge_render_* variables are the system's own working data.
 */

#include "ecs/systems/nge_render.h"
#include "sprite/nge_sprite.h"

/* Sprite component (NGE_COMPONENT_SPRITE): one table per field, indexed by entity ID */
extern uint8_t nge_render_sprite_first[NGE_ENTITY_MAX];       /* playing: pattern, or first animation pattern */
extern uint8_t nge_render_sprite_flags[NGE_ENTITY_MAX];       /* mirror, rotate, palette */
extern uint8_t nge_render_sprite_mode[NGE_ENTITY_MAX];        /* scale, NGE_SPRITE_PATTERN4 */
extern int8_t  nge_render_sprite_ox[NGE_ENTITY_MAX];          /* draw offset, pixels */
extern int8_t  nge_render_sprite_oy[NGE_ENTITY_MAX];
extern uint8_t nge_render_sprite_layer[NGE_ENTITY_MAX];
extern uint8_t nge_render_sprite_anim_count[NGE_ENTITY_MAX];  /* patterns in the animation, 1: still */
extern uint8_t nge_render_sprite_anim_rate[NGE_ENTITY_MAX];   /* frames per pattern */
extern uint8_t nge_render_sprite_anim_tick[NGE_ENTITY_MAX];   /* frames shown of the current pattern */
extern uint8_t nge_render_sprite_anim_frame[NGE_ENTITY_MAX];  /* current pattern - first */
extern uint8_t nge_render_sprite_rest_first[NGE_ENTITY_MAX];  /* the rest animation, shown when no one-shot plays */
extern uint8_t nge_render_sprite_rest_count[NGE_ENTITY_MAX];
extern uint8_t nge_render_sprite_rest_rate[NGE_ENTITY_MAX];
extern uint8_t nge_render_sprite_oneshot[NGE_ENTITY_MAX];     /* 1: a one-shot is playing, not the rest animation */
extern int8_t  nge_render_sprite_mox[NGE_ENTITY_MAX];         /* motion: offset now, added to the draw offset */
extern int8_t  nge_render_sprite_moy[NGE_ENTITY_MAX];
extern int8_t  nge_render_sprite_mtx[NGE_ENTITY_MAX];         /* a lunge's turning point; (0, 0) for a slide or on the way back */
extern int8_t  nge_render_sprite_mty[NGE_ENTITY_MAX];
extern uint8_t nge_render_sprite_mspeed[NGE_ENTITY_MAX];      /* pixels a frame; 0: no motion */

/* System working data */
extern nge_sprite_attr_t nge_render_attrs[NGE_RENDER_SPRITES];  /* records for sprites 0.. */
extern nge_entity_id_t   nge_render_vis[NGE_ENTITY_MAX];        /* visible entities this update */
extern uint16_t          nge_render_sx[NGE_ENTITY_MAX];         /* ...and their screen positions */
extern uint16_t          nge_render_sy[NGE_ENTITY_MAX];
extern uint8_t           nge_render_used;      /* sprites drawn by the last update */
extern uint8_t           nge_render_shown;     /* sprites visible on the hardware after the last upload */
extern uint8_t           nge_render_dropped_count;
extern nge_owner_t       nge_render_area;
extern uint8_t           nge_render_busy_count;  /* busy entities at the last update, plus any started since */
extern nge_entity_id_t   nge_render_follow_id;   /* the entity the camera follows, or NGE_ENTITY_INVALID */
extern uint16_t          nge_render_follow_x;    /* its drawn position when the camera last centred on it */
extern uint16_t          nge_render_follow_y;

#endif
