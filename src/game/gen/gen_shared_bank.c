/*
 * Helpers shared by every class's generator: banked in page 110 (with the
 * game components' add functions). They take copies of base record parts
 * on the stack, in common memory: the class's table is mapped out while
 * they run.
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/gen/gen_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"
#include "ecs/systems/nge_render.h"
#include "sprite/nge_sprite.h"

nge_entity_id_t gen_begin(uint8_t c, uint8_t kind, nge_owner_t owner) NGE_BANKED
{
    nge_entity_id_t id = nge_entity_create();

    if (id != NGE_ENTITY_INVALID) {
        nge_owner_set(id, owner);
        kind_add(id, c, kind);
    }
    return id;
}

void gen_render(nge_entity_id_t id, const gen_render_t *r) NGE_BANKED
{
    nge_render_add(id, r->pattern, r->flags, NGE_SPRITE_SCALE_1, r->layer);
    if (r->anim_count > 1) {
        nge_render_set_anim(id, r->pattern, r->anim_count, r->anim_rate);
    }
}

uint8_t gen_pick(const gen_where_t *where, uint8_t count, uint8_t depth) NGE_BANKED
{
    uint16_t total = 0;
    uint16_t pick;
    uint8_t kind;

    for (kind = 0; kind < count; kind++) {
        if (depth >= where[kind].depth) {
            total += where[kind].rarity;
        }
    }
    if (total == 0) {
        return 0xFF;
    }

    pick = (uint16_t)((unsigned int)rand() % total);
    for (kind = 0; kind < count; kind++) {
        if (depth >= where[kind].depth) {
            if (pick < where[kind].rarity) {
                return kind;
            }
            pick -= where[kind].rarity;
        }
    }
    return 0xFF;   /* not reached */
}
