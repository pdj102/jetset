/*
 * The AI's senses: banked in page 130 with the rest of the module. The
 * turn's context (ai_sense()), hearing, and sight (ai_in_sight(), on
 * nge's line test). See docs/ai_default_brain.md, R6 to R9 and D5.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/components/nge_block.h"
#include "ecs/components/nge_location.h"
#include "map/nge_map.h"

#include "game/actors.h"
#include "game/components/stats.h"

/* Alive and placed in area: an entity that can be seen, heard or reached there */
static uint8_t present(nge_entity_id_t id, nge_owner_t area)
{
    return id < NGE_ENTITY_MAX && NGE_ENTITY_IS_ALIVE(id) &&
           NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) && NGE_OWNER_OF(id) == area;
}

/* Asleep: 1 if it hears the player at (px, py) this turn, 1d20 + its Wisdom modifier */
static uint8_t hears(nge_entity_id_t id, uint8_t x, uint8_t y, uint8_t px, uint8_t py)
{
    int8_t mod = 0;
    int16_t roll;

    if (!nge_map_within(x, y, px, py, AI_HEAR)) {
        return 0;
    }
    if (NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_STATS)) {
        mod = STATS_MOD(stats_ability[ABIL_WIS][id]);
    }
    roll = (int16_t)(1 + (unsigned int)rand() % 20) + mod;
    return roll >= AI_WAKE_DC;
}

uint8_t ai_in_sight(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    return nge_map_within(x0, y0, x1, y1, AI_SIGHT) && nge_block_line(area, x0, y0, x1, y1, NGE_BLOCK_SOLID);
}

uint8_t ai_sees(nge_entity_id_t viewer, nge_entity_id_t id) NGE_BANKED
{
    nge_owner_t area;

    if (viewer >= NGE_ENTITY_MAX || !NGE_ENTITY_IS_ALIVE(viewer) ||
        !NGE_ENTITY_HAS_COMPONENT(viewer, NGE_COMPONENT_LOCATION)) {
        return 0;
    }
    area = NGE_OWNER_OF(viewer);
    return present(id, area) &&
           ai_in_sight(area, NGE_LOCATION_X(viewer), NGE_LOCATION_Y(viewer), NGE_LOCATION_X(id), NGE_LOCATION_Y(id));
}

void ai_sense(nge_entity_id_t id, ai_ctx_t *c)
{
    nge_owner_t area = NGE_OWNER_OF(id);
    uint8_t x = NGE_LOCATION_X(id);
    uint8_t y = NGE_LOCATION_Y(id);
    nge_entity_id_t target = ai_target[id];
    nge_entity_id_t look;
    uint8_t gx = ai_goal_x[id];
    uint8_t gy = ai_goal_y[id];
    uint8_t track = ai_track[id];
    uint8_t lx;
    uint8_t ly;
    ai_stim_t stim = AI_STIM_NONE;
    uint8_t sees = 0;

    nge_util_assert(AI_IS(id) && NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION));

    /* A target gone (dead, freed, on another level) is no one; where it was, nowhere */
    if (target != NGE_ENTITY_INVALID && !present(target, area)) {
        target = NGE_ENTITY_INVALID;
        gx = NGE_MAP_X_INVALID;
        gy = NGE_MAP_Y_INVALID;
        track = 0;
    }

    /* Asleep, it may hear the player; awake, it looks for its target, or the player */
    if (ai_state[id] == AI_STATE_ASLEEP) {
        if (present(actor_player, area) &&
            hears(id, x, y, NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player))) {
            stim = AI_STIM_NOISE;
        }
    } else {
        look = (target != NGE_ENTITY_INVALID) ? target : actor_player;
        if (present(look, area)) {
            lx = NGE_LOCATION_X(look);
            ly = NGE_LOCATION_Y(look);
            if (ai_in_sight(area, x, y, lx, ly)) {
                target = look;
                gx = lx;
                gy = ly;
                sees = 1;
                track = AI_TRACK;
                stim = AI_STIM_SEEN;
            } else if (look == target && track != 0 && ((x == gx && y == gy) || track < AI_TRACK)) {
                /* Out of sight: first to where it saw them, then it hears where they go */
                gx = lx;
                gy = ly;
                track--;
            }
        }
    }
    ai_target[id] = target;
    ai_goal_x[id] = gx;
    ai_goal_y[id] = gy;
    ai_track[id] = track;

    /* After the banked calls: the context is only written, from locals */
    c->id = id;
    c->can = ai_can(id);
    c->area = area;
    c->x = x;
    c->y = y;
    c->target = target;
    c->gx = gx;
    c->gy = gy;
    c->sees = sees;
    c->stim = stim;
}
