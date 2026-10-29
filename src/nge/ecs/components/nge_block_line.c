/*
 * The block component's line test: banked in page 56. It reads the map
 * (slot 1) and the block and location tables (slot 0), which are always
 * mapped, so it needs no other page. The rest of the component is in
 * common memory (nge_block.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_56
#pragma constseg PAGE_56
#endif

#include "ecs/components/nge_block.h"

uint8_t nge_block_line(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                       nge_block_t block) NGE_BANKED
{
    uint8_t x = x0;
    uint8_t y = y0;
    uint8_t dx = (x1 > x0) ? (uint8_t)(x1 - x0) : (uint8_t)(x0 - x1);
    uint8_t dy = (y1 > y0) ? (uint8_t)(y1 - y0) : (uint8_t)(y0 - y1);
    int8_t sx = (x1 > x0) ? 1 : -1;
    int8_t sy = (y1 > y0) ? 1 : -1;
    int16_t err = (int16_t)dx - (int16_t)dy;
    int16_t e2;

    block |= NGE_BLOCK_EDGE;   /* nothing sees or passes out of the map */

    /* Bresenham's line from (x0, y0): each tile strictly between the ends */
    for (;;) {
        e2 = (int16_t)(err * 2);
        if (e2 > -(int16_t)dy) {
            err -= dy;
            x = (uint8_t)(x + sx);
        }
        if (e2 < (int16_t)dx) {
            err += dx;
            y = (uint8_t)(y + sy);
        }
        if (x == x1 && y == y1) {
            return 1;
        }
        /* The map first: no entity search if a wall blocks */
        if ((nge_map_block(x, y) & block) != 0 ||
            nge_block_find(area, x, y, (nge_block_t)~block, NGE_ENTITY_INVALID) != NGE_ENTITY_INVALID) {
            return 0;
        }
    }
}
