/*
 * The AI's way to act: banked in page 130 with the rest of the module.
 * Apart from ai_bank.c so that code needing only the component (the
 * generators and their tests) doesn't need the actions.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

#include "core/nge_util.h"

#include "game/components/feature.h"

/*
 * The 8 directions, 0 north, then clockwise: a step and its neighbours
 * are one apart. Worked out, not tables: zsdcc's code for a table in a
 * banked page made the assembler warn about its address (integer range)
 */
static int8_t dir_x(uint8_t d)
{
    return (d == 0 || d == 4) ? 0 : (d < 4) ? 1 : -1;
}

static int8_t dir_y(uint8_t d)
{
    return (d == 2 || d == 6) ? 0 : (d >= 3 && d <= 5) ? 1 : -1;
}


uint8_t ai_try(uint8_t action, act_t *a) NGE_BANKED
{
    if (act_do(action, a, ACT_CHECK) == ACT_SUCCESS) {
        act_do(action, a, ACT_DO);
    }
    return a->used;
}

uint8_t ai_step(const ai_ctx_t *c, uint8_t d, uint8_t open)
{
    act_t a = { 0 };
    nge_entity_id_t blocker;
    ai_can_t can = c->can;   /* locals: T-ZSDCC-1 */

    nge_util_assert(d < 8);

    a.actor = c->id;
    a.dx = dir_x(d);
    a.dy = dir_y(d);
    if (ai_try(ACT_MOVE, &a)) {
        return 1;
    }
    blocker = a.target;
    if (open && a.reason == ACT_R_BLOCKED_ENTITY && (can & AI_CAN_OPEN) &&
        FEATURE_IS(blocker) && !FEATURE_IS_OPEN(blocker)) {
        a.target = blocker;
        return ai_try(ACT_OPEN, &a);
    }
    return 0;
}

uint8_t ai_step_toward(const ai_ctx_t *c, uint8_t x, uint8_t y)
{
    uint8_t x0 = c->x;   /* locals: T-ZSDCC-1 */
    uint8_t y0 = c->y;
    int16_t ax = (int16_t)((x > x0) ? x - x0 : x0 - x);
    int16_t ay = (int16_t)((y > y0) ? y - y0 : y0 - y);
    int16_t e2 = (int16_t)((ax - ay) * 2);
    int8_t dx = 0;
    int8_t dy = 0;
    uint8_t d;

    if (ax == 0 && ay == 0) {
        return 0;   /* there already */
    }
    /*
     * The first tile of the line the AI sees along (nge_block_line()'s
     * Bresenham, from here): the goal was seen along it, so it leads
     * through the doorway the target was seen through, where a straight
     * diagonal would hit the wall beside it
     */
    if (e2 > -ay) {
        dx = (x > x0) ? 1 : -1;
    }
    if (e2 < ax) {
        dy = (y > y0) ? 1 : -1;
    }
    for (d = 0; dir_x(d) != dx || dir_y(d) != dy; d++) {
    }
    /* The direct step, then 45 degrees left, then right */
    return ai_step(c, d, 1) || ai_step(c, (uint8_t)((d + 7) & 7), 1) || ai_step(c, (uint8_t)((d + 1) & 7), 1);
}

void ai_wait(const ai_ctx_t *c)
{
    act_t a = { 0 };
    uint8_t used;

    a.actor = c->id;
    used = ai_try(ACT_WAIT, &a);
    nge_util_assert(used);   /* a wait is always possible */
    (void)used;
}
