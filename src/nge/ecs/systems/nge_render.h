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
 * Animation is part of the system, advanced by nge_render_update():
 * - the rest animation: an entity shows `count` patterns from `first`, each
 *   for `rate` frames, over and over (nge_render_set_anim()); one pattern
 *   is a still sprite (nge_render_set_pattern());
 * - a one-shot (nge_render_play()) plays a sequence once over the rest
 *   animation, which then shows again by itself: the game never restores it;
 * - a motion moves the sprite away from its location and back, at a speed
 *   in pixels a frame, added to the draw offset: a slide from where it was
 *   (nge_render_slide(), after a move) or a lunge out and back
 *   (nge_render_lunge(), an attack).
 *
 * One-shots and motions play only in view: started out of view (where it
 * was and where it is, through the camera as it is then), they show their
 * end at once, and one whose place leaves the view ends then. While one
 * plays in view, the entity is busy, and nge_render_busy() says so: the
 * game waits for it before the world acts again. Rest animations are never
 * busy.
 *
 * The camera can follow an entity (nge_render_follow()): whenever its drawn
 * position (location and motion) changes, the update centres the camera on
 * it before placing the sprites, so the map scrolls with a slide a pixel at
 * a time. When it doesn't move, the camera stays where the game puts it.
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
#include "ecs/components/nge_components.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"

#ifndef NGE_RENDER_SPRITES
/** Hardware sprites owned by the system: 0 to NGE_RENDER_SPRITES-1 (1-128). */
#define NGE_RENDER_SPRITES  64
#endif

/** Number of layers: 0 is drawn underneath, NGE_RENDER_LAYERS-1 on top. */
#define NGE_RENDER_LAYERS  4

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

/**
 * @brief Make the rest animation a still pattern. A one-shot playing
 *        carries on, then shows it.
 */
void nge_render_set_pattern(nge_entity_id_t id, uint8_t pattern) NGE_BANKED;

/**
 * @brief Make the rest animation patterns first to first+count-1, each for
 *        rate frames, from the first. count 1 is a still pattern. A one-shot
 *        playing carries on, then shows it.
 */
void nge_render_set_anim(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED;

/**
 * @brief Play patterns first to first+count-1 once, each for rate frames,
 *        then the rest animation from its first pattern. Replaces a one-shot
 *        playing. Out of view: the rest animation at once (not busy).
 */
void nge_render_play(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED;

/**
 * @brief Slide from (ox, oy) pixels away to the location, speed pixels a
 *        frame on each axis: after a move, (ox, oy) is where it came from.
 *        From where it is drawn now if a motion is playing, which it
 *        replaces. Neither end in view: no motion (not busy).
 */
void nge_render_slide(nge_entity_id_t id, int8_t ox, int8_t oy, uint8_t speed) NGE_BANKED;

/**
 * @brief Lunge: move out to (ox, oy) pixels from the location (not (0, 0)),
 *        then back, speed pixels a frame on each axis. Replaces a motion
 *        playing, from where it is. Out of view: no motion (not busy).
 */
void nge_render_lunge(nge_entity_id_t id, int8_t ox, int8_t oy, uint8_t speed) NGE_BANKED;

/**
 * @brief An entity in view is busy: a motion or one-shot is playing. As the
 *        last update found, or started since. Cheap: ask it when deciding
 *        whether something may act, not every frame.
 */
uint8_t nge_render_busy(void) NGE_BANKED;

/**
 * @brief Follow an entity with the camera (nge_map_centre_camera_px()):
 *        each update in which its drawn position changed, starting with the
 *        next. NGE_ENTITY_INVALID: follow nothing (the state after
 *        nge_render_init()).
 */
void nge_render_follow(nge_entity_id_t id) NGE_BANKED;

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

/**
 * @brief Advance animations and motions, move the camera with the followed
 *        entity, and build the sprites for the next nge_render_upload().
 */
void nge_render_update(void) NGE_BANKED;

/** @brief Send the sprites built by the last nge_render_update() to the hardware. */
void nge_render_upload(void) NGE_BANKED;

/** @brief Number of visible entities left out by the last update (more than NGE_RENDER_SPRITES). */
uint8_t nge_render_dropped(void) NGE_BANKED;

#endif
