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
#include "game/actors.h"

static nge_entity_id_t create(uint8_t kind, nge_owner_t owner)
{
    /* Copies on the stack: the calls below map other pages */
    gen_item_render_t look = gen_items[kind].render;
    gen_render_t render;
    attack_kind_t attack = gen_items[kind].attack;
    uint8_t count = gen_items[kind].count;
    nge_entity_id_t id = gen_begin(CLASS_ITEM, kind, owner);

    if (id != NGE_ENTITY_INVALID) {
        render.pattern = look.pattern;   /* a still sprite on the objects' layer */
        render.anim_count = 1;
        render.anim_rate = 0;
        render.flags = look.flags;
        render.layer = LAYER_OBJECTS;
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

/* The readers of the record's data (docs/items.md 4.7): each checks the subclass */
uint8_t gen_item_ammo_group(uint8_t kind) NGE_BANKED
{
    uint8_t subclass;

    nge_util_assert(kind < IT_COUNT);

    subclass = gen_items[kind].subclass;
    return (subclass == ITEM_RANGED || subclass == ITEM_AMMO) ? gen_items[kind].data.ammo_group : GROUP_NONE;
}

uint8_t gen_item_key_lock_group(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return (gen_items[kind].subclass == ITEM_KEY) ? gen_items[kind].data.key_lock_group : KEY_NONE;
}

uint8_t gen_item_pick_bonus(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return (gen_items[kind].subclass == ITEM_TOOL) ? gen_items[kind].data.pick_bonus : 0;
}

uint8_t gen_item_book_spells(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);
    nge_util_assert(gen_items[kind].subclass == ITEM_BOOK);   /* 0 is a list, so no "none" to return */

    return gen_items[kind].data.book_spells;
}

uint8_t gen_item_armour_ac(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return (gen_items[kind].subclass == ITEM_ARMOUR) ? gen_items[kind].data.armour_ac : 0;
}

uint8_t gen_item_flags(uint8_t kind) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    return gen_items[kind].flags;
}

/* The shapes by (flags & ITEMF_SHAPE) >> 2: the field's five valid values */
static const uint8_t shapes[] = {SPELL_BOLT, SPELL_DIRECT, SPELL_TILE, SPELL_BALL, SPELL_BURST};

uint8_t gen_item_shape(uint8_t kind) NGE_BANKED
{
    uint8_t i;

    nge_util_assert(kind < IT_COUNT);

    i = (uint8_t)((uint8_t)(gen_items[kind].flags & ITEMF_SHAPE) >> 2);   /* unsigned (T-INT-2) */
    nge_util_assert(i < sizeof(shapes) / sizeof(shapes[0]));   /* 0x14 to 0x1C are invalid */
    return shapes[i];
}

void gen_item_attack(uint8_t kind, attack_kind_t *attack) NGE_BANKED
{
    nge_util_assert(kind < IT_COUNT);

    *attack = gen_items[kind].attack;
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
