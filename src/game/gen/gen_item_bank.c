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
    gen_render_t render = gen_items[kind].render;   /* a copy on the stack: the calls below map other pages */
    nge_entity_id_t id = gen_begin(CLASS_ITEM, kind, owner);

    if (id != NGE_ENTITY_INVALID) {
        gen_render(id, &render);   /* items block nothing: no block component */
        item_add(id);
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

void gen_item_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    GEN_COPY_NAME(gen_items[kind].name, buf, size);
}
