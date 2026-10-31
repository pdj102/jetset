#ifndef NGE_BLOCK_H
#define NGE_BLOCK_H

/**
 * @file nge_block.h
 * @brief Block component: what an entity blocks, and what it can enter.
 *
 * Tiles and entities have blocking flags (nge_block_t, NGE_BLOCK_* in
 * map/nge_map.h): what they block. A mover has pass flags: the blocking
 * flags it can enter anyway. It can enter a tile if
 *
 *     (block & ~pass) == 0
 *
 * where block is the tile's flags (the map's block table, by tile ID)
 * combined with those of every entity on the tile that has this component.
 * Examples, with NGE_BLOCK_OCCUPIED on creatures:
 *
 *     mover    pass                               enters
 *     player   0                                  floor, items
 *     bird     NO_FLOOR                           also pits
 *     ghost    SOLID + NO_FLOOR                   also walls, not creatures
 *     cursor   everything but EDGE (0xFE)         anything inside the map
 *
 * NGE_BLOCK_EDGE (outside the map) blocks every mover.
 *
 * An entity without this component blocks nothing. The tables are in slot 0
 * (the ECS data page): movement, monster AI and targeting read them every
 * turn. Everything here is in common memory, except the line test
 * nge_block_line() and the walk along a line, banked (NGE_BANKED, page 56).
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/components/nge_components.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "map/nge_map.h"

/* Tables in slot 0 (nge_block_cdata.c): meaningful only for entities with NGE_COMPONENT_BLOCK */
extern nge_block_t nge_block_of[NGE_ENTITY_MAX];     /**< What the entity blocks. */
extern nge_block_t nge_block_pass[NGE_ENTITY_MAX];   /**< What it can enter anyway. */

#define NGE_BLOCK_OF(id)    (nge_block_of[id])
#define NGE_BLOCK_PASS(id)  (nge_block_pass[id])

/** @brief Give an entity in use the component: what it blocks and what it can enter. */
void nge_block_add(nge_entity_id_t id, nge_block_t block, nge_block_t pass);

/** @brief Remove the component: the entity blocks nothing. */
void nge_block_remove(nge_entity_id_t id);

/**
 * @brief The first entity on tile (x, y) of an area that blocks a mover
 *        with these pass flags.
 *
 * Looks at alive entities with the location and block components, by ID.
 * @param ignore An entity to skip (the mover itself), or NGE_ENTITY_INVALID.
 * @return The blocking entity, or NGE_ENTITY_INVALID if none blocks.
 */
nge_entity_id_t nge_block_find(nge_owner_t area, uint8_t x, uint8_t y, nge_block_t pass,
                               nge_entity_id_t ignore);

/**
 * @brief Can a mover with these pass flags enter tile (x, y): neither the
 *        map nor an entity on the tile blocks it?
 *
 * Checks the map (map/nge_map.h; outside it is NGE_BLOCK_EDGE) first, then
 * (only if the map lets it in) the entities.
 * @param ignore An entity to skip (the mover itself), or NGE_ENTITY_INVALID.
 * @param blocker If not NULL, receives the blocking entity (for example to
 *                attack it), or NGE_ENTITY_INVALID if the map blocks or
 *                nothing does.
 * @return 1 if it can enter, 0 if not.
 */
uint8_t nge_block_can_enter(nge_owner_t area, uint8_t x, uint8_t y, nge_block_t pass,
                            nge_entity_id_t ignore, nge_entity_id_t *blocker);

/**
 * @brief Is the line from (x0, y0) to (x1, y1) in an area clear: does no
 *        tile strictly between the two ends have any of the flags in
 *        block, in the map or on an entity there with this component?
 *
 * For sight (NGE_BLOCK_SOLID: walls and closed doors block it, creatures
 * don't) and later a line of fire. NGE_BLOCK_EDGE always blocks. The ends
 * aren't tested, so a creature in a doorway sees out; a tile and its
 * neighbours are always clear. The tiles are Bresenham's line from
 * (x0, y0), which isn't symmetric: from the other end a line may differ.
 * @return 1 if clear, 0 if blocked.
 */
uint8_t nge_block_line(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                       nge_block_t block) NGE_BANKED;

/**
 * A walk along a line, kept by the caller, usually on its stack. For a
 * caller that needs to know what is on each tile, or which stops it, where
 * nge_block_line() would only say whether the line is clear. x and y are
 * the tile reached; the other fields are private.
 */
typedef struct nge_block_walk {
    uint8_t x;
    uint8_t y;
    uint8_t x1;
    uint8_t y1;
    uint8_t dx;
    uint8_t dy;
    int8_t  sx;
    int8_t  sy;
    int16_t err;
} nge_block_walk_t;

/** @brief Start a walk at (x0, y0), towards (x1, y1): nge_block_line()'s tiles. */
void nge_block_walk_start(nge_block_walk_t *w, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) NGE_BANKED;

/**
 * @brief Step to the next tile of the line, in w->x and w->y: the first
 *        step is the first tile after the start, and the last is (x1, y1).
 *
 * Tests nothing, so a caller can step beyond the map's edge if the line
 * does: nge_map_block() answers for such a tile.
 * @return 1 if it stepped, 0 if the walk had already reached (x1, y1), as
 *         it has at once if the two ends are the same tile.
 */
uint8_t nge_block_walk_next(nge_block_walk_t *w) NGE_BANKED;

#endif
