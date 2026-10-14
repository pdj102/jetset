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

static void still(nge_entity_id_t id, uint8_t pattern)
{
    nge_render_first[id] = pattern;
    nge_render_anim_count[id] = 1;
    nge_render_anim_rate[id] = 1;
    nge_render_anim_tick[id] = 0;
    nge_render_anim_frame[id] = 0;
}

void nge_render_init(void) NGE_BANKED
{
    nge_render_used = 0;
    nge_render_shown = 0;
    nge_render_dropped_count = 0;
    nge_render_area = NGE_OWNER_WORLD;
    nge_render_map = NULL;
}

void nge_render_add(nge_entity_id_t id, uint8_t pattern, uint8_t flags, uint8_t mode,
                    uint8_t layer) NGE_BANKED
{
    check_entity(id, 0);
    check_patterns(pattern, 1, mode);
    nge_util_assert(layer < NGE_RENDER_LAYERS);

    still(id, pattern);
    nge_render_flags[id] = flags;
    nge_render_mode[id] = mode;
    nge_render_ox[id] = 0;
    nge_render_oy[id] = 0;
    nge_render_layer[id] = layer;
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
    check_patterns(pattern, 1, nge_render_mode[id]);

    still(id, pattern);
}

void nge_render_set_anim(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED
{
    check_entity(id, 1);
    check_patterns(first, count, nge_render_mode[id]);
    nge_util_assert(rate >= 1);

    nge_render_first[id] = first;
    nge_render_anim_count[id] = count;
    nge_render_anim_rate[id] = rate;
    nge_render_anim_tick[id] = 0;
    nge_render_anim_frame[id] = 0;
}

void nge_render_set_flags(nge_entity_id_t id, uint8_t flags) NGE_BANKED
{
    check_entity(id, 1);

    nge_render_flags[id] = flags;
}

void nge_render_set_scale(nge_entity_id_t id, uint8_t scale) NGE_BANKED
{
    check_entity(id, 1);

    nge_render_mode[id] = (nge_render_mode[id] & NGE_SPRITE_PATTERN4) | (scale & (uint8_t)~NGE_SPRITE_PATTERN4);
}

void nge_render_set_offset(nge_entity_id_t id, int8_t ox, int8_t oy) NGE_BANKED
{
    check_entity(id, 1);

    nge_render_ox[id] = ox;
    nge_render_oy[id] = oy;
}

void nge_render_set_layer(nge_entity_id_t id, uint8_t layer) NGE_BANKED
{
    check_entity(id, 1);
    nge_util_assert(layer < NGE_RENDER_LAYERS);

    nge_render_layer[id] = layer;
}

void nge_render_set_view(nge_owner_t area, const nge_map_t *map) NGE_BANKED
{
    nge_util_assert(area < NGE_OWNER_COUNT);

    nge_render_area = area;
    nge_render_map = map;
}

/* Advance an entity's animation by one frame */
static void animate(nge_entity_id_t id)
{
    if (nge_render_anim_count[id] > 1 && ++nge_render_anim_tick[id] >= nge_render_anim_rate[id]) {
        nge_render_anim_tick[id] = 0;
        if (++nge_render_anim_frame[id] >= nge_render_anim_count[id]) {
            nge_render_anim_frame[id] = 0;
        }
    }
}

/*
 * Collect the visible entities and their screen positions in
 * nge_render_vis, nge_render_sx and nge_render_sy; returns how many.
 */
static uint8_t collect(const nge_map_t *m)
{
    uint16_t view_x = (uint16_t)m->cam_x * NGE_MAP_TILE_PIXELS;
    uint16_t view_y = (uint16_t)m->cam_y * NGE_MAP_TILE_PIXELS;
    int16_t view_w = (int16_t)m->view_w * NGE_MAP_TILE_PIXELS;
    int16_t view_h = (int16_t)m->view_h * NGE_MAP_TILE_PIXELS;
    uint16_t base_x = (uint16_t)m->window.x1 * NGE_MAP_TILE_PIXELS;
    uint16_t base_y = (uint16_t)m->window.y1 * NGE_MAP_TILE_PIXELS;
    uint8_t i;
    uint8_t v = 0;
    uint8_t mode;
    nge_entity_id_t id;
    int16_t rx;
    int16_t ry;
    int16_t w;
    int16_t h;

    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        if (!NGE_ENTITY_HAS_COMPONENTS(id, NGE_COMPONENT_LOCATION | NGE_COMPONENT_SPRITE) ||
            nge_owner_of[id] != nge_render_area) {
            continue;
        }
        animate(id);

        /* Position relative to the view's top-left, in pixels */
        rx = (int16_t)(NGE_LOCATION_PX(id) + nge_render_ox[id] - view_x);
        ry = (int16_t)(NGE_LOCATION_PY(id) + nge_render_oy[id] - view_y);
        /* Size with scaling: X scale in mode bits 3-4, Y scale in bits 1-2 */
        mode = nge_render_mode[id];
        w = SPRITE_PIXELS << ((mode >> 3) & 3);
        h = SPRITE_PIXELS << ((mode >> 1) & 3);
        if (rx <= -w || rx >= view_w || ry <= -h || ry >= view_h) {
            continue;
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
    if (nge_render_map != NULL) {
        v = collect(nge_render_map);
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
            if (nge_render_layer[id] != layer) {
                continue;
            }
            if (slot == 0) {
                nge_render_dropped_count++;
                continue;
            }
            nge_sprite_attr_build(&nge_render_attrs[--slot], nge_render_sx[k], nge_render_sy[k],
                                  nge_render_first[id] + nge_render_anim_frame[id],
                                  nge_render_flags[id], nge_render_mode[id]);
        }
    }

    /*
     * Records past n that the hardware may still show (drawn by an earlier
     * update, uploaded or not) are hidden, and sent by the next upload
     */
    k = nge_render_used > nge_render_shown ? nge_render_used : nge_render_shown;
    while (k > n) {
        NGE_SPRITE_ATTR_HIDE(&nge_render_attrs[--k]);
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
