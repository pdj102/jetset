/*
 * The block component's line test and walk: banked in page 56. It reads the map
 * (slot 1) and the block and location tables (slot 0), which are always
 * mapped, so it needs no other page. The rest of the component is in
 * common memory (nge_block.c).
 */
#ifdef __SDCC
#pragma codeseg PAGE_56
#pragma constseg PAGE_56
#endif

#include "ecs/components/nge_block.h"

/* walk_init() and walk_step() aren't banked, so nge_block_line() nests no banked call */

/* Bresenham's line: one step */
static uint8_t walk_step(nge_block_walk_t *w)
{
    int16_t e2;

    if (w->x == w->x1 && w->y == w->y1) {
        return 0;
    }
    e2 = (int16_t)(w->err * 2);
    if (e2 > -(int16_t)w->dy) {
        w->err -= w->dy;
        w->x = (uint8_t)(w->x + w->sx);
    }
    if (e2 < (int16_t)w->dx) {
        w->err += w->dx;
        w->y = (uint8_t)(w->y + w->sy);
    }
    return 1;
}

static void walk_init(nge_block_walk_t *w, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    w->x = x0;
    w->y = y0;
    w->x1 = x1;
    w->y1 = y1;
    w->dx = (x1 > x0) ? (uint8_t)(x1 - x0) : (uint8_t)(x0 - x1);
    w->dy = (y1 > y0) ? (uint8_t)(y1 - y0) : (uint8_t)(y0 - y1);
    w->sx = (x1 > x0) ? 1 : -1;
    w->sy = (y1 > y0) ? 1 : -1;
    w->err = (int16_t)w->dx - (int16_t)w->dy;
}

void nge_block_walk_start(nge_block_walk_t *w, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) NGE_BANKED
{
    walk_init(w, x0, y0, x1, y1);
}

uint8_t nge_block_walk_next(nge_block_walk_t *w) NGE_BANKED
{
    return walk_step(w);
}

uint8_t nge_block_line(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                       nge_block_t block) NGE_BANKED
{
    nge_block_walk_t w;
    uint8_t x;
    uint8_t y;

    block |= NGE_BLOCK_EDGE;   /* nothing sees or passes out of the map */

    /* Each tile strictly between the ends */
    walk_init(&w, x0, y0, x1, y1);
    while (walk_step(&w)) {
        x = w.x;
        y = w.y;
        if (x == x1 && y == y1) {
            return 1;
        }
        /* The map first: no entity search if a wall blocks */
        if ((nge_map_block(x, y) & block) != 0 ||
            nge_block_find(area, x, y, (nge_block_t)~block, NGE_ENTITY_INVALID) != NGE_ENTITY_INVALID) {
            return 0;
        }
    }
    return 1;
}
