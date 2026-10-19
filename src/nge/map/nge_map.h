#ifndef NGE_MAP_H
#define NGE_MAP_H

/**
 * @file nge_map.h
 * @brief The map: a tile map, larger than the screen, shown through a window.
 *
 * Terms:
 * - map: the game's grid of tiles (map tile coordinates, (0,0) top-left).
 * - window: the fixed rectangle of the hardware tilemap the map is shown in.
 * - view: the part of the map that is visible; the same size as the window.
 * - camera: the top-left corner of the view, in map tiles.
 *
 * There is one map. All its data (its state nge_map, its tiles
 * nge_map_tiles and its block table nge_map_blocks) is in page
 * NGE_MAP_PAGE, which nge_map_init() maps permanently into slot 1
 * ($2000-$3FFF), so the data is at fixed addresses and no function takes a
 * map pointer. Reads such as collision checks are plain memory reads. Rows
 * are NGE_MAP_STRIDE tiles apart whatever the map's width, so a tile's
 * address needs no multiply by an arbitrary width. See "Always-mapped data
 * pages" in docs/memory_map.md.
 *
 * The camera functions and nge_map_set() only mark the view as needing a
 * redraw. Call nge_map_update() once per tick, straight after
 * nge_frame_wait_vblank(), to redraw it while the display is blanked:
 *
 *     nge_frame_wait_vblank();
 *     nge_render_upload();     (or nge_sprite_upload(...))
 *     nge_map_update();
 *     ...game logic: move entities, the camera, change tiles...
 *
 * Banking: the set-up functions (init, clear_blocks, set_block,
 * clip_sprites) are banked (NGE_BANKED, page 44): don't call them while game
 * code has its own page mapped in slot 6 or 7. The rest, used every frame
 * or every move, is in common memory.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "core/nge_types.h"
#include "tilemap/nge_tilemap.h"

#ifndef NGE_MAP_STRIDE
/** Tiles per map row in memory; the maximum map width. */
#define NGE_MAP_STRIDE      80
#endif

#ifndef NGE_MAP_HEIGHT_MAX
/** Maximum map height in tiles. 80 x 48 tiles of 2 bytes fill 7.5K of the 8K page. */
#define NGE_MAP_HEIGHT_MAX  48
#endif

/** Not a map column or row: maps are at most NGE_MAP_STRIDE wide and NGE_MAP_HEIGHT_MAX high. */
#define NGE_MAP_X_INVALID  0xFF
#define NGE_MAP_Y_INVALID  0xFF

/* The invalid coordinates are off every map */
typedef char nge_map_invalid_check[(NGE_MAP_X_INVALID >= NGE_MAP_STRIDE && NGE_MAP_Y_INVALID >= NGE_MAP_HEIGHT_MAX) ? 1 : -1];

/* nge_map_within()'s sums fit in 16 bits: the largest dx*dx + dy*dy on the biggest map */
typedef char nge_map_within_check[((uint32_t)(NGE_MAP_STRIDE - 1) * (NGE_MAP_STRIDE - 1) +
                                   (uint32_t)(NGE_MAP_HEIGHT_MAX - 1) * (NGE_MAP_HEIGHT_MAX - 1) <= 0xFFFF) ? 1 : -1];

/** Page holding the map's data (src/banks.inc), and the slot it is mapped into. */
#define NGE_MAP_PAGE  51
#define NGE_MAP_SLOT  1

/** Entries in the block table: one per tile ID. */
#define NGE_MAP_BLOCK_ENTRIES  256

/*
 * Blocking flags: what a tile (through the block table, by tile ID) or an
 * entity (ecs/components/nge_block.h) blocks. A mover with pass flags
 * `pass` can enter a tile if (block & ~pass) == 0, where block is the
 * tile's flags combined with those of every entity on it.
 */
typedef uint8_t nge_block_t;

#define NGE_BLOCK_NONE      0x00   /**< Blocks nothing: floor, an item. */
#define NGE_BLOCK_EDGE      0x01   /**< Blocks everything: outside the map. */
#define NGE_BLOCK_SOLID     0x02   /**< Solid matter: walls, closed doors. */
#define NGE_BLOCK_NO_FLOOR  0x04   /**< No floor to walk on: pits, chasms. */
#define NGE_BLOCK_OCCUPIED  0x08   /**< A creature: other creatures can't share the tile. */
#define NGE_BLOCK_USER      0x10   /**< First of the game's flags: 0x10 to 0x80. */

/** Pixels per tile. */
#define NGE_MAP_TILE_PIXELS  8

/** @brief The map's state. Set up with nge_map_init(); read-only for the game. */
typedef struct nge_map {
    uint8_t width;        /**< Map width in tiles. */
    uint8_t height;       /**< Map height in tiles. */
    nge_rect_t window;    /**< Window on the hardware tilemap, inclusive tile coordinates. */
    uint8_t view_w;       /**< Window width in tiles. */
    uint8_t view_h;       /**< Window height in tiles. */
    uint8_t cam_x;        /**< Left column of the view, map tiles. */
    uint8_t cam_y;        /**< Top row of the view, map tiles. */
    uint8_t dirty;        /**< The view needs redrawing. */
} nge_map_t;

/* The map's data, in page NGE_MAP_PAGE in slot 1 (nge_map_cdata.c) */
extern nge_block_t nge_map_blocks[NGE_MAP_BLOCK_ENTRIES];        /**< What each tile ID blocks. */
extern nge_map_t   nge_map;                                      /**< State: read-only for the game. */
extern nge_tile_t  nge_map_tiles[NGE_MAP_STRIDE * NGE_MAP_HEIGHT_MAX];   /**< Row-major tiles. */

/** Address of the first tile of map row y. */
#define NGE_MAP_ROW(y)  (nge_map_tiles + (uint16_t)(y) * NGE_MAP_STRIDE)

/**
 * The tile at (x, y), read or written directly with no checks: for loops
 * that test or build many tiles. nge_map_get() and nge_map_set() are the
 * checked versions (nge_map_set() also marks the view for redrawing).
 */
#define NGE_MAP_TILE(x, y)  (NGE_MAP_ROW(y)[x])

/**
 * The blocking flags of the tile at (x, y), read directly with no checks
 * (inside the map only). nge_map_block() is the checked version.
 */
#define NGE_MAP_BLOCK(x, y)  (nge_map_blocks[NGE_MAP_TILE(x, y).tile_id])

/* ---- Set-up: banked ---- */

/**
 * @brief Map the map's page into slot 1 and set up an empty map with its
 *        camera at (0,0), marked for drawing. Call before any other map
 *        function; call again to start afresh.
 *
 * Every tile is cleared to 0 (tile 0, attribute 0) and every tile ID in the
 * block table to NGE_BLOCK_NONE: fill them in (directly, with
 * NGE_MAP_TILE(), and with nge_map_clear_blocks() and nge_map_set_block()).
 *
 * @param width Map width in tiles, from the window width to NGE_MAP_STRIDE.
 * @param height Map height in tiles, from the window height to NGE_MAP_HEIGHT_MAX.
 * @param window Window on the hardware tilemap, inclusive tile coordinates.
 */
void nge_map_init(uint8_t width, uint8_t height, const nge_rect_t *window) NGE_BANKED;

/** @brief Set the blocking flags of every tile ID (e.g. NGE_BLOCK_SOLID as a safe default). */
void nge_map_clear_blocks(nge_block_t flags) NGE_BANKED;

/** @brief Set the blocking flags of a tile ID: every tile with this ID has them. */
void nge_map_set_block(uint8_t tile_id, nge_block_t flags) NGE_BANKED;

/**
 * @brief Clip sprites to the map window, so sprites partly outside the view
 *        are cut off at its edges.
 */
void nge_map_clip_sprites(void) NGE_BANKED;

/* ---- Common memory ---- */

/**
 * @brief The blocking flags of the tile at (x, y).
 * @return The flags of its tile ID, or NGE_BLOCK_EDGE outside the map.
 */
nge_block_t nge_map_block(uint8_t x, uint8_t y);

/**
 * @brief Move the camera to (x, y), clamped so the view stays inside the map.
 * @return 1 if the camera moved (the view will be redrawn), 0 if not.
 */
uint8_t nge_map_set_camera(uint8_t x, uint8_t y);

/**
 * @brief Move the camera by (dx, dy) tiles, clamped so the view stays inside the map.
 * @return 1 if the camera moved, 0 if not.
 */
uint8_t nge_map_move_camera(int8_t dx, int8_t dy);

/**
 * @brief Move the camera so map tile (x, y) is at the centre of the view, as
 *        far as the map edges allow.
 * @return 1 if the camera moved, 0 if not.
 */
uint8_t nge_map_centre_camera(uint8_t x, uint8_t y);

/**
 * @brief Get the tile at map position (x, y).
 * @return Pointer to the tile in the map data. (A pointer, not a copy: SDCC
 *         cannot return structs by value.)
 */
const nge_tile_t *nge_map_get(uint8_t x, uint8_t y);

/**
 * @brief Change the tile at map position (x, y).
 *
 * If the position is in the view, the view is redrawn by the next
 * nge_map_update().
 */
void nge_map_set(uint8_t x, uint8_t y, const nge_tile_t *tile);

/**
 * @brief Mark the view for redrawing by the next nge_map_update(): after
 *        something else has drawn over the map window (a screen shown in it).
 */
void nge_map_invalidate(void);

/** @brief Redraw the view if it changed since it was last drawn. */
void nge_map_update(void);

/** @brief Redraw the view now, whether it changed or not. */
void nge_map_draw(void);

/**
 * @brief The one range test: tile (x1, y1) is within range r of tile
 *        (x0, y0), on a circle: dx*dx + dy*dy <= r*r + r, which keeps the tiles
 *        whose centres are closer than r + 1/2. Range 0 is the tile itself,
 *        range 1 its 8 neighbours. Every range and distance test in a game
 *        uses it, so a range means the same tiles everywhere.
 *
 * Cheap on the Next: zsdcc uses the Z80N's mul instruction for the squares.
 * @return 1 if within range, 0 if not.
 */
uint8_t nge_map_within(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t r);

/**
 * @brief Convert a map pixel position to screen (sprite) coordinates.
 * @param map_x X in map pixels (tile x * 8 + pixel within the tile).
 * @param map_y Y in map pixels.
 * @param screen_x Receives the sprite X coordinate.
 * @param screen_y Receives the sprite Y coordinate.
 * @return 1 if the position is inside the view, 0 if not. The coordinates
 *         are set either way, so a sprite partly in view can still be placed.
 */
uint8_t nge_map_to_screen(uint16_t map_x, uint16_t map_y, uint16_t *screen_x, uint16_t *screen_y);

#endif
