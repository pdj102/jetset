/*
 * The item generator: banked in page 108, with its table of item kinds in
 * page 109 (gen_item_pdata.c), mapped into slot 7 while this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_108
#pragma constseg PAGE_108
#endif

#include "game/gen/gen_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    /* Copies on the stack: the calls below map other pages */
    gen_render_t render = gen_items[kind].render;
    attack_kind_t attack = gen_items[kind].attack;
    uint8_t count = gen_items[kind].count;
    nge_entity_id_t id = gen_begin(CLASS_ITEM, kind, owner);

    if (id != NGE_ENTITY_INVALID) {
        gen_render(id, &render);   /* items block nothing: no block component */
        item_add(id, count);
        if (ATTACK_KIND_HAS(&attack)) {
            attack_add(id, (attack.range > 1) ? ATTACK_RANGED : ATTACK_MELEE, &attack);
        }
    }
    return id;
}

nge_entity_id_t gen_item(uint8_t kind, nge_owner_t owner) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return create(kind, owner);
}

nge_entity_id_t gen_item_random(uint8_t depth, nge_owner_t owner) NGE_BANKED
{
    uint8_t kind;

    GEN_PICK(gen_items, IT_COUNT, depth, kind);
    return (kind == KIND_INVALID) ? NGE_ENTITY_INVALID : create(kind, owner);
}

uint16_t gen_item_fits(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return gen_items[kind].fits;
}

uint8_t gen_item_subclass(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return gen_items[kind].subclass;
}

uint16_t gen_item_hook(uint8_t kind, uint8_t hook) NGE_BANKED
{
    gen_item_hook_t h;

    nge_util_assert(kind < IT_COUNT);
    nge_util_assert(hook < GEN_HOOK_COUNT);

    h = gen_items[kind].hook;
    return (h.hook == hook) ? GEN_HOOK_PACK(h.ef, h.power) : GEN_HOOK_PACK(EF_NONE, 0);
}

void gen_item_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    GEN_COPY_NAME(gen_items[kind].name, buf, size);
}
