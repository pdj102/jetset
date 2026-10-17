/*
 * The item generator: banked in page 108, with its table of base items in
 * page 109 (gen_item_pdata.c), mapped into slot 7 while this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_108
#pragma constseg PAGE_108
#endif

#include "game/gen/gen_internal.h"

#include "core/nge_util.h"

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    /* Copies on the stack: the calls below map other pages */
    gen_render_t render = gen_items[kind].render;
    health_base_t health = gen_items[kind].health;
    attack_base_t melee = gen_items[kind].melee;
    nge_entity_id_t id = gen_begin(CLASS_ITEM, kind, owner);

    if (id != NGE_ENTITY_INVALID) {
        gen_render(id, &render);   /* items block nothing: no block component */
        item_add(id);
        if (HEALTH_BASE_HAS(&health)) {
            health_add(id, &health);
        }
        if (ATTACK_BASE_HAS(&melee)) {
            attack_add(id, ATTACK_MELEE, &melee);
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
    gen_where_t where[IT_COUNT];   /* copies on the stack, for gen_pick() */
    uint8_t kind;

    for (kind = 0; kind < IT_COUNT; kind++) {
        where[kind] = gen_items[kind].where;
    }
    kind = gen_pick(where, IT_COUNT, depth);
    return (kind == 0xFF) ? NGE_ENTITY_INVALID : create(kind, owner);
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

uint8_t gen_item_hook(uint8_t kind, uint8_t hook) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);
    nge_util_assert(hook < GEN_HOOKS);

    return gen_items[kind].hooks[hook];
}

void gen_item_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    GEN_COPY_NAME(gen_items[kind].name, buf, size);
}
