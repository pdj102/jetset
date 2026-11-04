/*
 * The feature generator: banked in page 122, with its table of feature
 * kinds in page 123 (gen_feature_pdata.c), mapped into slot 7 while
 * this code runs.
 */
#ifdef __SDCC
#pragma codeseg PAGE_122
#pragma constseg PAGE_122
#endif

#include "game/gen/gen_internal.h"

#include <stdlib.h>

#include "core/nge_util.h"

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    /* Copies on the stack: the calls below map other pages */
    uint8_t state = gen_features[kind].state;
    health_kind_t health = gen_features[kind].health;
    attack_kind_t attack = gen_features[kind].attack;
    nge_entity_id_t id = gen_begin(CLASS_FEATURE, kind, owner);

    if (id != NGE_ENTITY_INVALID) {
        feature_add(id, state);   /* placed and drawn by feature_place() */
        if (HEALTH_KIND_HAS(&health)) {
            health_add(id, &health);   /* it can be broken (docs/features.md 4.10) */
        }
        if (ATTACK_KIND_HAS(&attack)) {
            attack_add(id, ATTACK_MELEE, &attack);   /* a trap's: it strikes what stands on it */
        }
    }
    return id;
}

nge_entity_id_t gen_feature(uint8_t kind, nge_owner_t owner) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    return create(kind, owner);
}

nge_entity_id_t gen_feature_random(uint8_t depth, nge_owner_t owner) NGE_BANKED
{
    uint8_t kind;

    GEN_PICK(gen_features, FT_COUNT, depth, kind);
    if (kind == KIND_INVALID) {
        return NGE_ENTITY_INVALID;
    }
    return create(kind, owner);
}

uint8_t gen_feature_dc(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    return gen_features[kind].dc;
}

uint8_t gen_feature_holds(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    return gen_features[kind].holds;
}

uint8_t gen_feature_verb(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    return gen_features[kind].verb;
}

void gen_feature_info(uint8_t kind, gen_feature_info_t *info) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    *info = gen_features[kind].info;
}

uint16_t gen_feature_hook(uint8_t kind, uint8_t hook) NGE_BANKED
{
    gen_hook_t h;

    nge_util_assert(kind < FT_COUNT);
    nge_util_assert(hook < GEN_HOOK_COUNT);

    h = gen_features[kind].hooks[hook];
    return GEN_HOOK_PACK(h.ef, h.power);
}

void gen_feature_name(uint8_t kind, char *buf, uint8_t size) NGE_BANKED
{
    nge_util_assert(kind < FT_COUNT);

    GEN_COPY_NAME(gen_features[kind].name, buf, size);
}
