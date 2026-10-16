/*
 * The creature generator: banked in page 106, with its table of base
 * creatures in page 107 (gen_creature_pdata.c), mapped into slot 7 while
 * this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_106
#pragma constseg PAGE_106
#endif

#include "game/gen/gen_internal.h"

#include <string.h>

#include "core/nge_util.h"
#include "ecs/systems/nge_timer.h"
#include "game/timers.h"

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    gen_creature_base_t b;   /* a copy on the stack: the calls below map other pages */
    nge_entity_id_t id;

    memcpy(&b, &gen_creatures[kind], sizeof(b));
    id = gen_begin(CLASS_CREATURE, kind, owner);
    if (id == NGE_ENTITY_INVALID) {
        return id;
    }
    gen_render(id, &b.render);
    nge_block_add(id, b.block.block, b.block.pass);
    stats_add(id, &b.stats);
    /* Its turns: the player's wait for a command, a monster's come every move */
    if (b.flags & CREATURE_PLAYER) {
        nge_timer_add(id, TIMER_PLAYER, 1, 0, 0);
    } else {
        nge_timer_add(id, TIMER_MONSTER, stats_speed[id], 0, 0);
    }
    return id;
}

nge_entity_id_t gen_creature(uint8_t kind, nge_owner_t owner) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    return create(kind, owner);
}

nge_entity_id_t gen_creature_random(uint8_t depth, nge_owner_t owner) NGE_BANKED
{
    gen_where_t where[CR_COUNT];   /* copies on the stack, for gen_pick() */
    uint8_t kind;

    for (kind = 0; kind < CR_COUNT; kind++) {
        where[kind] = gen_creatures[kind].where;
    }
    kind = gen_pick(where, CR_COUNT, depth);
    return (kind == 0xFF) ? NGE_ENTITY_INVALID : create(kind, owner);
}

void gen_creature_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < CR_COUNT);

    GEN_COPY_NAME(gen_creatures[kind].name, buf, size);
}
