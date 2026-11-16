/*
 * Noises reaching the AI's creatures: banked in page 130 with the rest of
 * the module, the records in page 131. The noise module (game/noise) says
 * what makes a noise; this is who hears it. See docs/noise.md, R7, R8,
 * R18, D3 to D5.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "map/nge_map.h"

void ai_hear(const ai_noise_t *n) NGE_BANKED
{
    ai_noise_t c = *n;   /* a local copy before the loop: see T-ZSDCC-1 */
    nge_entity_id_t id;
    uint8_t i;
    uint8_t cx;
    uint8_t cy;
    uint8_t d;
    uint8_t dy;
    uint8_t heard;

    /* The cheapest rejects first: the AI bit, the area, then the distance (R18) */
    NGE_ENTITY_FOR_EACH_ALIVE(i, id) {
        if (!AI_IS(id) || NGE_OWNER_OF(id) != c.area || id == c.source || id == c.target ||
            !NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION)) {
            continue;
        }
        cx = NGE_LOCATION_X(id);
        cy = NGE_LOCATION_Y(id);
        /*
         * Its distance: the least range that reaches it, as nge_map_within()
         * measures every range. At least the larger of dx and dy, so beyond
         * the loudness by that is out of hearing with no range test
         */
        d = (cx > c.x) ? (uint8_t)(cx - c.x) : (uint8_t)(c.x - cx);
        dy = (cy > c.y) ? (uint8_t)(cy - c.y) : (uint8_t)(c.y - cy);
        if (dy > d) {
            d = dy;
        }
        while (d <= c.loud && !nge_map_within(c.x, c.y, cx, cy, d)) {
            d++;
        }
        if (d > c.loud) {
            continue;
        }
        heard = (uint8_t)(c.loud - d);
        if (!nge_block_line(c.area, c.x, c.y, cx, cy, NGE_BLOCK_SOLID)) {
            if (heard < AI_MUFFLE) {
                continue;
            }
            heard = (uint8_t)(heard - AI_MUFFLE);
        }
        heard++;   /* the record keeps heard + 1: 0 is none */
        if (heard > ai_heard[id]) {
            ai_heard[id] = heard;
            ai_heard_x[id] = c.x;
            ai_heard_y[id] = c.y;
        }
    }
}
