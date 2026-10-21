/*
 * Render system: banked in page 52, with the sprite component and its
 * working data as page data in page 53 (nge_render_pdata.c), which
 * banked_call maps into slot 7 for each call. It reads the entity, owner
 * and location tables in slot 0, the map in common memory, and calls the
 * sprite module's common functions with its records in page 53.
 */
#ifdef __SDCC
#pragma codeseg PAGE_52
#pragma constseg PAGE_52
#endif

#include "ecs/systems/nge_render_internal.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/components/nge_location.h"

typedef char nge_render_sprites_check[(NGE_RENDER_SPRITES >= 1 && NGE_RENDER_SPRITES <= NGE_SPRITE_MAX) ? 1 : -1];

#define SPRITE_PIXELS  16

/* An entity in use, with the sprite component unless adding it */
static void check_entity(nge_entity_id_t id, uint8_t with_sprite)
{
    (void)id;   /* unused when asserts are compiled out (NDEBUG) */
    (void)with_sprite;
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(NGE_ENTITY_STATE(id) != NGE_ENTITY_FREE);
    nge_util_assert(!with_sprite || (nge_entity_components[id] & NGE_COMPONENT_SPRITE));
}

/* Patterns first to first+count-1 exist for the pattern type in mode */
static void check_patterns(uint8_t first, uint8_t count, uint8_t mode)
{
    (void)first;
    (void)count;
    (void)mode;
    nge_util_assert(count >= 1);
    nge_util_assert((uint16_t)first + count <=
                    ((mode & NGE_SPRITE_PATTERN4) ? NGE_SPRITE_PATTERNS4 : NGE_SPRITE_PATTERNS));
}

/* Show the rest animation from its first pattern: no one-shot */
static void rest(nge_entity_id_t id)
{
    nge_render_sprite_first[id] = nge_render_sprite_rest_first[id];
    nge_render_sprite_anim_count[id] = nge_render_sprite_rest_count[id];
    nge_render_sprite_anim_rate[id] = nge_render_sprite_rest_rate[id];
    nge_render_sprite_anim_tick[id] = 0;
    nge_render_sprite_anim_frame[id] = 0;
    nge_render_sprite_oneshot[id] = 0;
}

/* Set the rest animation: shown now, or after a one-shot playing */
static void set_rest(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate)
{
    nge_render_sprite_rest_first[id] = first;
    nge_render_sprite_rest_count[id] = count;
    nge_render_sprite_rest_rate[id] = rate;
    if (!nge_render_sprite_oneshot[id]) {
        rest(id);
    }
}

/* No motion: drawn at the location and draw offset */
static void stop_motion(nge_entity_id_t id)
{
    nge_render_sprite_mox[id] = 0;
    nge_render_sprite_moy[id] = 0;
    nge_render_sprite_mtx[id] = 0;
    nge_render_sprite_mty[id] = 0;
    nge_render_sprite_mspeed[id] = 0;
}

/* A motion or one-shot is playing */
static uint8_t busy(nge_entity_id_t id)
{
    return nge_render_sprite_mspeed[id] != 0 || nge_render_sprite_oneshot[id];
}

/* Its motion and one-shot end now: it shows at rest, at its location */
static void finish(nge_entity_id_t id)
{
    stop_motion(id);
    if (nge_render_sprite_oneshot[id]) {
        rest(id);
    }
}

/*
 * The entity's sprite, (mx, my) pixels from its location and draw offset,
 * would show at least partly in the view: rx and ry are its position
 * relative to the view's top-left
 */
static uint8_t on_view(nge_entity_id_t id, int16_t mx, int16_t my, int16_t *rx, int16_t *ry)
{
    uint8_t mode = nge_render_sprite_mode[id];
    int16_t x = (int16_t)(NGE_LOCATION_PX(id) - NGE_MAP_CAMERA_PX()) + nge_render_sprite_ox[id] + mx;
    int16_t y = (int16_t)(NGE_LOCATION_PY(id) - NGE_MAP_CAMERA_PY()) + nge_render_sprite_oy[id] + my;
    /* Size with scaling: X scale in mode bits 3-4, Y scale in bits 1-2 */
    int16_t w = SPRITE_PIXELS << ((mode >> 3) & 3);
    int16_t h = SPRITE_PIXELS << ((mode >> 1) & 3);

    *rx = x;
    *ry = y;
    return x > -w && x < (int16_t)nge_map.view_w * NGE_MAP_TILE_PIXELS &&
           y > -h && y < (int16_t)nge_map.view_h * NGE_MAP_TILE_PIXELS;
}

/* The entity is drawn in the view's area, and its sprite at (mx, my) from its place would be in view */
static uint8_t in_view(nge_entity_id_t id, int16_t mx, int16_t my)
{
    int16_t rx;
    int16_t ry;

    return nge_render_area != NGE_RENDER_NO_VIEW && nge_owner_of[id] == nge_render_area &&
           NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION) && on_view(id, mx, my, &rx, &ry);
}

void nge_render_init(void) NGE_BANKED
{
    /*
     * Every record hidden, and counted as shown: the next upload hides all
     * the system's sprites, including any a previous game left on screen
     */
    nge_sprite_clear(nge_render_attrs, NGE_RENDER_SPRITES);
    nge_render_used = 0;
    nge_render_shown = NGE_RENDER_SPRITES;
    nge_render_dropped_count = 0;
    nge_render_area = NGE_RENDER_NO_VIEW;
    nge_render_busy_count = 0;
    nge_render_follow_id = NGE_ENTITY_INVALID;
}

void nge_render_add(nge_entity_id_t id, uint8_t pattern, uint8_t flags, uint8_t mode,
                    uint8_t layer) NGE_BANKED
{
    check_entity(id, 0);
    check_patterns(pattern, 1, mode);
    nge_util_assert(layer < NGE_RENDER_LAYERS);

    nge_render_sprite_oneshot[id] = 0;
    set_rest(id, pattern, 1, 1);
    stop_motion(id);
    nge_render_sprite_flags[id] = flags;
    nge_render_sprite_mode[id] = mode;
    nge_render_sprite_ox[id] = 0;
    nge_render_sprite_oy[id] = 0;
    nge_render_sprite_layer[id] = layer;
    nge_entity_components[id] |= NGE_COMPONENT_SPRITE;
}

void nge_render_remove(nge_entity_id_t id) NGE_BANKED
{
    check_entity(id, 0);

    nge_entity_components[id] &= (nge_component_mask_t)~NGE_COMPONENT_SPRITE;
}

void nge_render_set_pattern(nge_entity_id_t id, uint8_t pattern) NGE_BANKED
{
    check_entity(id, 1);
    check_patterns(pattern, 1, nge_render_sprite_mode[id]);

    set_rest(id, pattern, 1, 1);
}

void nge_render_set_anim(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED
{
    check_entity(id, 1);
    check_patterns(first, count, nge_render_sprite_mode[id]);
    nge_util_assert(rate >= 1);

    set_rest(id, first, count, rate);
}

void nge_render_play(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED
{
    check_entity(id, 1);
    check_patterns(first, count, nge_render_sprite_mode[id]);
    nge_util_assert(rate >= 1);

    if (!in_view(id, nge_render_sprite_mox[id], nge_render_sprite_moy[id])) {
        if (nge_render_sprite_oneshot[id]) {
            rest(id);   /* not seen: it ends at once */
        }
        return;
    }
    nge_render_sprite_first[id] = first;
    nge_render_sprite_anim_count[id] = count;
    nge_render_sprite_anim_rate[id] = rate;
    nge_render_sprite_anim_tick[id] = 0;
    nge_render_sprite_anim_frame[id] = 0;
    nge_render_sprite_oneshot[id] = 1;
    nge_render_busy_count++;
}

void nge_render_slide(nge_entity_id_t id, int8_t ox, int8_t oy, uint8_t speed) NGE_BANKED
{
    /* From where it is drawn now, which may be part-way through a motion */
    int16_t mx = (int16_t)nge_render_sprite_mox[id] + ox;
    int16_t my = (int16_t)nge_render_sprite_moy[id] + oy;

    check_entity(id, 1);
    nge_util_assert(speed >= 1);
    nge_util_assert(mx >= -128 && mx <= 127 && my >= -128 && my <= 127);

    /* Seen if it starts or ends in view */
    if (!in_view(id, mx, my) && !in_view(id, 0, 0)) {
        stop_motion(id);
        return;
    }
    nge_render_sprite_mox[id] = (int8_t)mx;
    nge_render_sprite_moy[id] = (int8_t)my;
    nge_render_sprite_mtx[id] = 0;
    nge_render_sprite_mty[id] = 0;
    nge_render_sprite_mspeed[id] = speed;
    nge_render_busy_count++;
}

void nge_render_lunge(nge_entity_id_t id, int8_t ox, int8_t oy, uint8_t speed) NGE_BANKED
{
    check_entity(id, 1);
    nge_util_assert(speed >= 1);
    nge_util_assert(ox != 0 || oy != 0);   /* (0, 0) is the way back */

    if (!in_view(id, nge_render_sprite_mox[id], nge_render_sprite_moy[id])) {
        stop_motion(id);
        return;
    }
    nge_render_sprite_mtx[id] = ox;
    nge_render_sprite_mty[id] = oy;
    nge_render_sprite_mspeed[id] = speed;
    nge_render_busy_count++;
}

uint8_t nge_render_busy(void) NGE_BANKED
{
    return nge_render_busy_count != 0;
}

void nge_render_follow(nge_entity_id_t id) NGE_BANKED
{
    nge_util_assert(id < NGE_ENTITY_MAX || id == NGE_ENTITY_INVALID);

    nge_render_follow_id = id;
    nge_render_follow_x = 0xFFFF;   /* not a drawn position: the next update centres on it */
}

void nge_render_set_flags(nge_entity_id_t id, uint8_t flags) NGE_BANKED
{
    check_entity(id, 1);

    nge_render_sprite_flags[id] = flags;
}

void nge_render_set_scale(nge_entity_id_t id, uint8_t scale) NGE_BANKED
{
    check_entity(id, 1);

    nge_render_sprite_mode[id] = (nge_render_sprite_mode[id] & NGE_SPRITE_PATTERN4) | (scale & (uint8_t)~NGE_SPRITE_PATTERN4);
}

void nge_render_set_offset(nge_entity_id_t id, int8_t ox, int8_t oy) NGE_BANKED
{
    check_entity(id, 1);

    nge_render_sprite_ox[id] = ox;
    nge_render_sprite_oy[id] = oy;
}

void nge_render_set_layer(nge_entity_id_t id, uint8_t layer) NGE_BANKED
{
    check_entity(id, 1);
    nge_util_assert(layer < NGE_RENDER_LAYERS);

    nge_render_sprite_layer[id] = layer;
}

void nge_render_set_view(nge_owner_t area) NGE_BANKED
{
    nge_util_assert(area < NGE_OWNER_COUNT || area == NGE_RENDER_NO_VIEW);

    nge_render_area = area;
}

/*
 * Advance an entity's animation by one frame. A one-shot shows each
 * pattern for rate frames, from this update, then the rest animation
 */
static void animate(nge_entity_id_t id)
{
    if (nge_render_sprite_oneshot[id]) {
        if (nge_render_sprite_anim_tick[id] >= nge_render_sprite_anim_rate[id]) {
            nge_render_sprite_anim_tick[id] = 0;
            if (++nge_render_sprite_anim_frame[id] >= nge_render_sprite_anim_count[id]) {
                rest(id);
                return;
            }
        }
        nge_render_sprite_anim_tick[id]++;
        return;
    }
    if (nge_render_sprite_anim_count[id] > 1 && ++nge_render_sprite_anim_tick[id] >= nge_render_sprite_anim_rate[id]) {
        nge_render_sprite_anim_tick[id] = 0;
        if (++nge_render_sprite_anim_frame[id] >= nge_render_sprite_anim_count[id]) {
            nge_render_sprite_anim_frame[id] = 0;
        }
    }
}

/* v moved speed pixels towards goal, stopping there */
static int8_t approach(int8_t v, int8_t goal, uint8_t speed)
{
    int16_t n;

    if (v < goal) {
        n = (int16_t)v + speed;
        return n > goal ? goal : (int8_t)n;
    }
    if (v > goal) {
        n = (int16_t)v - speed;
        return n < goal ? goal : (int8_t)n;
    }
    return v;
}

/* Advance an entity's motion by one frame: towards a lunge's turning point, then back to 0 */
static void move(nge_entity_id_t id)
{
    uint8_t speed = nge_render_sprite_mspeed[id];
    int8_t gx;
    int8_t gy;

    if (speed == 0) {
        return;
    }
    gx = nge_render_sprite_mtx[id];
    gy = nge_render_sprite_mty[id];
    nge_render_sprite_mox[id] = approach(nge_render_sprite_mox[id], gx, speed);
    nge_render_sprite_moy[id] = approach(nge_render_sprite_moy[id], gy, speed);
    if (nge_render_sprite_mox[id] == gx && nge_render_sprite_moy[id] == gy) {
        if (gx != 0 || gy != 0) {
            nge_render_sprite_mtx[id] = 0;   /* turned: back to the location */
            nge_render_sprite_mty[id] = 0;
        } else {
            nge_render_sprite_mspeed[id] = 0;
        }
    }
}

/* The entity is drawn: alive (the caller's loop), with location and sprite, in the view's area */
static uint8_t drawn(nge_entity_id_t id)
{
    return NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION | NGE_COMPONENT_SPRITE) &&
           nge_owner_of[id] == nge_render_area;
}

/*
 * The followed entity, already advanced this update: centre the camera on
 * its drawn position (its location plus its motion) if that changed since
 * the camera last centred on it. When it doesn't move, the camera stays
 * where the game put it
 */
static void follow(void)
{
    nge_entity_id_t id = nge_render_follow_id;
    int16_t x;
    int16_t y;

    if (id == NGE_ENTITY_INVALID || !NGE_ENTITY_IS_ALIVE(id) || !drawn(id)) {
        return;
    }
    x = (int16_t)NGE_LOCATION_PX(id) + nge_render_sprite_mox[id];
    y = (int16_t)NGE_LOCATION_PY(id) + nge_render_sprite_moy[id];
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if ((uint16_t)x != nge_render_follow_x || (uint16_t)y != nge_render_follow_y) {
        nge_render_follow_x = (uint16_t)x;
        nge_render_follow_y = (uint16_t)y;
        nge_map_centre_camera_px((uint16_t)x, (uint16_t)y);
    }
}

/*
 * Advance the drawn entities, move the camera with the followed one, then
 * collect the visible entities and their screen positions in
 * nge_render_vis, nge_render_sx and nge_render_sy; returns how many.
 * Counts the busy entities in view in nge_render_busy_count
 */
static uint8_t collect(void)
{
    uint16_t base_x = (uint16_t)nge_map.window.x1 * NGE_TILEMAP_TILE_PIXELS;
    uint16_t base_y = (uint16_t)nge_map.window.y1 * NGE_TILEMAP_TILE_PIXELS;
    nge_entity_id_t followed = nge_render_follow_id;
    uint8_t i;
    uint8_t v = 0;
    nge_entity_id_t id;
    int16_t rx;
    int16_t ry;

    /* The followed entity first, so the camera is where it will be before anything is placed */
    if (followed != NGE_ENTITY_INVALID && NGE_ENTITY_IS_ALIVE(followed) && drawn(followed)) {
        animate(followed);
        move(followed);
    }
    follow();

    nge_render_busy_count = 0;
    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        if (!drawn(id)) {
            continue;
        }
        if (id != followed) {
            animate(id);
            move(id);
        }

        if (!on_view(id, nge_render_sprite_mox[id], nge_render_sprite_moy[id], &rx, &ry)) {
            /*
             * Not drawn. One whose place is out of view too has left it (the
             * camera moved away): its animation ends now. One sliding in
             * from outside carries on
             */
            if (busy(id)) {
                if (on_view(id, 0, 0, &rx, &ry)) {
                    nge_render_busy_count++;
                } else {
                    finish(id);
                }
            }
            continue;
        }
        if (busy(id)) {
            nge_render_busy_count++;
        }

        /*
         * Positions left of or above the window wrap round to large values,
         * which the 9-bit sprite coordinates show just off the left or top
         * edge, so a sprite partly in view is placed correctly.
         */
        nge_render_vis[v] = id;
        nge_render_sx[v] = base_x + (uint16_t)rx;
        nge_render_sy[v] = base_y + (uint16_t)ry;
        v++;
    }
    return v;
}

void nge_render_update(void) NGE_BANKED
{
    uint8_t v = 0;
    uint8_t n;
    uint8_t k;
    uint8_t layer;
    uint8_t slot;
    nge_entity_id_t id;

    nge_render_dropped_count = 0;
    if (nge_render_area != NGE_RENDER_NO_VIEW) {
        v = collect();
    } else {
        nge_render_busy_count = 0;   /* nothing in view */
    }

    /*
     * Fill the records from the top layer down, from the highest record, so
     * the top layers get the highest (topmost) sprites and, if there are
     * too many, the lowest layers are the ones left out.
     */
    n = v < NGE_RENDER_SPRITES ? v : NGE_RENDER_SPRITES;
    slot = n;
    layer = NGE_RENDER_LAYERS;
    while (layer-- > 0) {
        for (k = 0; k < v; k++) {
            id = nge_render_vis[k];
            if (nge_render_sprite_layer[id] != layer) {
                continue;
            }
            if (slot == 0) {
                nge_render_dropped_count++;
                continue;
            }
            nge_sprite_set(&nge_render_attrs[--slot], nge_render_sx[k], nge_render_sy[k],
                                  nge_render_sprite_first[id] + nge_render_sprite_anim_frame[id],
                                  nge_render_sprite_flags[id], nge_render_sprite_mode[id]);
        }
    }

    /*
     * Records past n that the hardware may still show (drawn by an earlier
     * update, uploaded or not) are hidden, and sent by the next upload
     */
    k = nge_render_used > nge_render_shown ? nge_render_used : nge_render_shown;
    while (k > n) {
        nge_sprite_show(&nge_render_attrs[--k], 0);
    }
    nge_render_used = n;
}

void nge_render_upload(void) NGE_BANKED
{
    uint8_t count = nge_render_used > nge_render_shown ? nge_render_used : nge_render_shown;

    if (count != 0) {
        nge_sprite_upload(0, nge_render_attrs, count);
    }
    nge_render_shown = nge_render_used;
}

uint8_t nge_render_dropped(void) NGE_BANKED
{
    return nge_render_dropped_count;
}
