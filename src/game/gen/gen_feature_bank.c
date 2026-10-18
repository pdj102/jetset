/*
 * The feature generator: banked in page 122, with its table of base
 * features in page 123 (gen_feature_pdata.c), mapped into slot 7 while
 * this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_122
#pragma constseg PAGE_122
#endif

#include "game/gen/gen_internal.h"

#include "core/nge_util.h"

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    uint8_t state = gen_features[kind].state;   /* a copy: the calls below map other pages */
    nge_entity_id_t id = gen_begin(CLASS_FEATURE, kind, owner);

    if (id != NGE_ENTITY_INVALID) {
        feature_add(id, state);   /* placed and drawn by feature_place() */
    }
    return id;
}

nge_entity_id_t gen_feature(uint8_t kind, nge_owner_t owner) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    return create(kind, owner);
}

void gen_feature_info(uint8_t kind, gen_feature_info_t *info) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    *info = gen_features[kind].info;
}

uint8_t gen_feature_hook(uint8_t kind, uint8_t hook) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);
    nge_util_assert(hook < GEN_HOOKS);

    return gen_features[kind].hooks[hook];
}

void gen_feature_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    GEN_COPY_NAME(gen_features[kind].name, buf, size);
}
