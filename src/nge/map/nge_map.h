#ifndef NGE_MAP_H
#define NGE_MAP_H

/**
 * @file nge_map.h
 * @brief A tile map, larger than the screen, shown through a window.
 *
 * Terms:
 * - map: the game's grid of tiles (map tile coordinates, (0,0) top-left).
 * - window: the fixed rectangle of the hardware tilemap the map is shown in.
 * - view: the part of the map that is visible; the same size as the window.
 * - camera: the top-left corner of the view, in map tiles.
 *
 * Map data stays resident while it is in use (in slot 1, see
 * docs/memory_map.md), so reads such as collision checks are plain memory
 * reads. Rows are NGE_MAP_STRIDE tiles apart whatever the map's width, so a
 * tile's address needs no multiply by an arbitrary width.
 *
 * The camera functions and nge_map_set() only mark the view as needing a
 * redraw. Call nge_map_update() once per tick, straight after
 * nge_frame_wait_vblank(), to redraw it while the display is blanked:
 *
 *     nge_frame_wait_vblank();
 *     nge_sprite_update();
 *     nge_map_update(&map);
 *     ...game logic: move entities, the camera, change tiles...
 */

#include <stdint.h>

#include "core/nge_types.h"
#include "tilemap/nge_tilemap.h"

#ifndef NGE_MAP_STRIDE
/** Tiles per map row in memory; the maximum map width. */
#define NGE_MAP_STRIDE      80
#endif

#ifndef NGE_MAP_MAX_HEIGHT
/** Maximum map height in tiles. 80 x 48 tiles of 2 bytes fill 7.5K of an 8K slot. */
#define NGE_MAP_MAX_HEIGHT  48
#endif

/** Bytes needed for a map of the maximum size. */
#define NGE_MAP_MAX_BYTES  ((uint16_t)NGE_MAP_STRIDE * NGE_MAP_MAX_HEIGHT * sizeof(nge_tile_t))

/** Pixels per tile. */
#define NGE_MAP_TILE_PIXELS  8

/**
 * @brief A map shown in a tilemap window. Set up with nge_map_init(); the
 *        fields are read-only for the game.
 */
typedef struct nge_map {
    nge_tile_t *tiles;    /**< Row-major tiles, rows NGE_MAP_STRIDE tiles apart. */
    uint8_t width;        /**< Map width in tiles. */
    uint8_t height;       /**< Map height in tiles. */
    nge_rect_t window;    /**< Window on the hardware tilemap, inclusive tile coordinates. */
    uint8_t view_w;       /**< Window width in tiles. */
    uint8_t view_h;       /**< Window height in tiles. */
    uint8_t cam_x;        /**< Left column of the view, map tiles. */
    uint8_t cam_y;        /**< Top row of the view, map tiles. */
    uint8_t dirty;        /**< The view needs redrawing. */
} nge_map_t;

/** Address of the first tile of map row y. */
#define NGE_MAP_ROW(m, y)  ((m)->tiles + (uint16_t)(y) * NGE_MAP_STRIDE)

/**
 * The tile at (x, y), read directly with no checks: for loops that test many
 * tiles, such as collision checks. nge_map_get() is the checked version.
 */
#define NGE_MAP_TILE(m, x, y)  (NGE_MAP_ROW(m, y)[x])

/**
 * @brief Set up a map with its camera at (0,0), marked for drawing.
 * @param m Map to set up.
 * @param tiles Map data: height rows, NGE_MAP_STRIDE tiles apart. Must stay
 *              mapped in while the map is used.
 * @param width Map width in tiles, from the window width to NGE_MAP_STRIDE.
 * @param height Map height in tiles, from the window height to NGE_MAP_MAX_HEIGHT.
 * @param window Window on the hardware tilemap, inclusive tile coordinates.
 */
void nge_map_init(nge_map_t *m, nge_tile_t *tiles, uint8_t width, uint8_t height,
                  const nge_rect_t *window);

/**
 * @brief Move the camera to (x, y), clamped so the view stays inside the map.
 * @return 1 if the camera moved (the view will be redrawn), 0 if not.
 */
uint8_t nge_map_set_camera(nge_map_t *m, uint8_t x, uint8_t y);

/**
 * @brief Move the camera by (dx, dy) tiles, clamped so the view stays inside the map.
 * @return 1 if the camera moved, 0 if not.
 */
uint8_t nge_map_move_camera(nge_map_t *m, int8_t dx, int8_t dy);

/**
 * @brief Move the camera so map tile (x, y) is at the centre of the view, as
 *        far as the map edges allow.
 * @return 1 if the camera moved, 0 if not.
 */
uint8_t nge_map_centre_camera(nge_map_t *m, uint8_t x, uint8_t y);

/**
 * @brief Get the tile at map position (x, y).
 * @return Pointer to the tile in the map data. (A pointer, not a copy: SDCC
 *         cannot return structs by value.)
 */
const nge_tile_t *nge_map_get(const nge_map_t *m, uint8_t x, uint8_t y);

/**
 * @brief Change the tile at map position (x, y).
 *
 * If the position is in the view, the view is redrawn by the next
 * nge_map_update().
 */
void nge_map_set(nge_map_t *m, uint8_t x, uint8_t y, const nge_tile_t *tile);

/** @brief Redraw the view if it changed since it was last drawn. */
void nge_map_update(nge_map_t *m);

/** @brief Redraw the view now, whether it changed or not. */
void nge_map_draw(nge_map_t *m);

/**
 * @brief Convert a map pixel position to screen (sprite) coordinates.
 * @param map_x X in map pixels (tile x * 8 + pixel within the tile).
 * @param map_y Y in map pixels.
 * @param screen_x Receives the sprite X coordinate.
 * @param screen_y Receives the sprite Y coordinate.
 * @return 1 if the position is inside the view, 0 if not. The coordinates
 *         are set either way, so a sprite partly in view can still be placed.
 */
uint8_t nge_map_to_screen(const nge_map_t *m, uint16_t map_x, uint16_t map_y,
                          uint16_t *screen_x, uint16_t *screen_y);

/**
 * @brief Clip sprites to the map window, so sprites partly outside the view
 *        are cut off at its edges.
 */
void nge_map_clip_sprites(const nge_map_t *m);

#endif
