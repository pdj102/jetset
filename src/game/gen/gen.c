/*
 * gen_name(), in common memory: each class's names are in its own page,
 * mapped by that class's banked copy function.
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
    } else {
        buf[0] = '\0';
        strncat(buf, "something", size - 1);
    }
}
