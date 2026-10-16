#ifndef NGE_RENDER_H
#define NGE_RENDER_H

/**
 * @file nge_render.h
 * @brief Render system: draws entities with the sprite component as hardware sprites.
 *
 * Each frame nge_render_update() draws every entity that is alive, has the
 * location (ecs/components/nge_location.h) and sprite components, and is
 * owned directly by the area set with nge_render_set_view(): one 16x16
 * hardware sprite per entity, at its location in map pixels plus its draw
 * offset, converted to the screen through the map's camera. Entities
 * entirely outside the view are skipped; the sprite clip window
 * (nge_map_clip_sprites()) cuts off sprites partly outside it.
 *
 * The system owns hardware sprites 0 to NGE_RENDER_SPRITES-1 and draws its
 * entities in layer order: layer 0 underneath, NGE_RENDER_LAYERS-1 on top.
 * If more entities are visible than it has sprites, the lowest layers are
 * left out (nge_render_dropped()).
 *
 * Animation is part of the system: an entity shows `count` patterns from
 * `first`, each for `rate` frames, advanced by nge_render_update().
 *
 * Per frame, two banked calls:
 *
 *     nge_frame_wait_vblank();
 *     nge_render_upload();     // last frame's sprites, in the blanking period
 *     ...game logic...
 *     nge_render_update();     // build the next frame's sprites, any time
 *
 * The sprite component's data is private to the system, page data in
 * page 53: change it only with the functions below, when something
 * happens in the game (never once per entity per frame: each call is a
 * banked call). Banking: every function is banked (NGE_BANKED, page 52):
 * don't call them while game code has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"

#ifndef NGE_RENDER_SPRITES
/** Hardware sprites owned by the system: 0 to NGE_RENDER_SPRITES-1 (1-128). */
#define NGE_RENDER_SPRITES  64
#endif

/** Number of layers: 0 is drawn underneath, NGE_RENDER_LAYERS-1 on top. */
#define NGE_RENDER_LAYERS  4

/** Component bit: component number 1, reserved for nge (see nge_entity.h). */
#define NGE_COMPONENT_SPRITE  NGE_COMPONENT(1)

/**
 * @brief Set up the system: nothing drawn, no view. Call after
 *        nge_sprite_init(); call again to start afresh (a new game): the
 *        next upload hides all the system's sprites.
 */
void nge_render_init(void) NGE_BANKED;

/**
 * @brief Give an entity in use the sprite component: a still pattern, no
 *        draw offset, no animation.
 * @param pattern 8-bit pattern 0-63, or 4-bit pattern 0-127 with NGE_SPRITE_PATTERN4 in mode.
 * @param flags NGE_SPRITE_MIRROR_X, NGE_SPRITE_MIRROR_Y, NGE_SPRITE_ROTATE, NGE_SPRITE_PALETTE(n).
 * @param mode NGE_SPRITE_SCALE_X* | NGE_SPRITE_SCALE_Y*, and NGE_SPRITE_PATTERN4.
 * @param layer 0 to NGE_RENDER_LAYERS-1.
 */
void nge_render_add(nge_entity_id_t id, uint8_t pattern, uint8_t flags, uint8_t mode,
                    uint8_t layer) NGE_BANKED;

/** @brief Remove the sprite component. */
void nge_render_remove(nge_entity_id_t id) NGE_BANKED;

/** @brief Show a still pattern, stopping any animation. */
void nge_render_set_pattern(nge_entity_id_t id, uint8_t pattern) NGE_BANKED;

/**
 * @brief Animate: show patterns first to first+count-1, each for rate
 *        frames, from the first. count 1 is a still pattern.
 */
void nge_render_set_anim(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED;

/** @brief Replace the mirror, rotate and palette flags (for example mirror when turning). */
void nge_render_set_flags(nge_entity_id_t id, uint8_t flags) NGE_BANKED;

/** @brief Replace the scale (NGE_SPRITE_SCALE_*), keeping the pattern type. */
void nge_render_set_scale(nge_entity_id_t id, uint8_t scale) NGE_BANKED;

/** @brief Draw the sprite (ox, oy) pixels from the location, e.g. (0, -8) to stand on the tile. */
void nge_render_set_offset(nge_entity_id_t id, int8_t ox, int8_t oy) NGE_BANKED;

/** @brief Move the entity to another layer. */
void nge_render_set_layer(nge_entity_id_t id, uint8_t layer) NGE_BANKED;

/** set_view's area for drawing nothing (the state after nge_render_init()). */
#define NGE_RENDER_NO_VIEW  0xFF

/**
 * @brief Draw the entities owned by area, through the map's camera and
 *        window (map/nge_map.h).
 * @param area The area owner (NGE_OWNER_AREA(a)), any other owner, or
 *             NGE_RENDER_NO_VIEW to draw nothing.
 */
void nge_render_set_view(nge_owner_t area) NGE_BANKED;

/** @brief Advance animations and build the sprites for the next nge_render_upload(). */
void nge_render_update(void) NGE_BANKED;

/** @brief Send the sprites built by the last nge_render_update() to the hardware. */
void nge_render_upload(void) NGE_BANKED;

/** @brief Number of visible entities left out by the last update (more than NGE_RENDER_SPRITES). */
uint8_t nge_render_dropped(void) NGE_BANKED;

#endif
