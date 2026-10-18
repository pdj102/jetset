/*
 * gen_name() and gen_hook(), in common memory: each class's table is in
 * its own page, mapped by that class's banked function.
 */
#include "game/gen/gen_internal.h"

#include <string.h>

#include "core/nge_util.h"

void gen_name(nge_entity_id_t id, char *buf, uint8_t size)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(size >= 1);

    if (KIND_IS(id, CLASS_CREATURE)) {
        gen_creature_name(kind_index[id], buf, size);
    } else if (KIND_IS(id, CLASS_ITEM)) {
        gen_item_name(kind_index[id], buf, size);
    } else if (KIND_IS(id, CLASS_FEATURE)) {
        gen_feature_name(kind_index[id], buf, size);
    } else {
        buf[0] = '\0';
        strncat(buf, "something", size - 1);
    }
}

uint8_t gen_hook(nge_entity_id_t id, uint8_t hook)
{
    nge_util_assert(id < NGE_ENTITY_MAX);
    nge_util_assert(hook < GEN_HOOKS);

    if (KIND_IS(id, CLASS_CREATURE)) {
        return gen_creature_hook(kind_index[id], hook);
    }
    if (KIND_IS(id, CLASS_ITEM)) {
        return gen_item_hook(kind_index[id], hook);
    }
    if (KIND_IS(id, CLASS_FEATURE)) {
        return gen_feature_hook(kind_index[id], hook);
    }
    return EF_NONE;
}
