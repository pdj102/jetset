/*
 * Helpers shared by every class's generator: banked in page 110 (with the
 * game components' add functions). They take copies of kind record parts
 * on the stack, in common memory: the class's table is mapped out while
 * they run.
 */
#ifdef __SDCC
#pragma codeseg PAGE_110
#pragma constseg PAGE_110
#endif

#include "game/gen/gen_internal.h"

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
    /* Every sprite is a 4-bit pattern, its bank in its flags (data/sprite_data.h) */
    nge_render_add(id, r->pattern, r->flags, NGE_SPRITE_PATTERN4 | NGE_SPRITE_SCALE_1, r->layer);
    if (r->anim_count > 1) {
        nge_render_set_anim(id, r->pattern, r->anim_count, r->anim_rate);
    }
}
