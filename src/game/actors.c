#include "game/actors.h"

#include <stddef.h>
#include <arch/zxn.h>

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/nge_owner.h"
#include "hw/nge_palette.h"
#include "game/dev.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/kind.h"
#include "game/components/pc.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/quest/quest.h"
#include "sprite/nge_sprite.h"

#if GAME_DEV_KIT
/* The development kit's items, owned by the player: in their pack, which keeps 2 places free */
static const uint8_t dev_kit[] = {
    IT_SWORD, IT_HEAL_DRAUGHT, IT_HEAL_POTION, IT_RING_HEALTH_2, IT_RING_FRAILTY, IT_PLATE_ARMOUR_2, IT_HELMET,
    IT_POTION_HASTE, IT_POTION_HASTE, IT_SHORT_BOW, IT_ARROWS, IT_CLOAK, IT_LEGGINGS, IT_WAND_FIRE, IT_LANTERN,
    IT_SP_IDENTIFY, IT_BREAD, IT_WATERSKIN,
};
#endif

/* A persistent entity: made once a game, in its depth's area (game/level/level.h) */
typedef struct actor_persistent {
    uint8_t cls;     /* CLASS_CREATURE or CLASS_ITEM */
    uint8_t kind;    /* CR_* or IT_* */
    uint8_t depth;   /* 1 to LEVEL_DEPTH_BOTTOM */
} actor_persistent_t;

/* The persistent entities besides the player: the quest item (docs/winning.md), later the final boss */
static const actor_persistent_t persistents[] = {
    { CLASS_ITEM, IT_QUEST, LEVEL_DEPTH_BOTTOM },
#if GAME_DEV_KIT
    { CLASS_CREATURE, CR_GOBLIN, 2 },   /* for trying out parking: the same goblin on every visit */
#endif
    { CLASS_NONE, 0, 0 },               /* the end */
};

nge_entity_id_t actor_player;

/* The art's palette has every bank the game names: a missing one would show sprites in black */
typedef char actors_bank_check[(SPRITE_PALETTE_ENTRIES == BANK_COUNT * 16) ? 1 : -1];

void actors_load_graphics(void)
{
    /* The loaders map the graphics' pages into slot 7 while they copy */
    nge_sprite_load_patterns4(SPRITE_PATTERNS_PAGE_0, 0, sprite_patterns_0, SPRITE_PATTERNS_COUNT_0);
#if SPRITE_PATTERN_PAGES > 1
    nge_sprite_load_patterns4(SPRITE_PATTERNS_PAGE_1, SPRITE_PATTERNS_COUNT_0, sprite_patterns_1,
                              SPRITE_PATTERNS_COUNT_1);
#endif
    nge_palette_load9(SPRITE_PALETTE_PAGE, RPC_SELECT_SPRITES_PALETTE_0, 0, sprite_palette, SPRITE_PALETTE_ENTRIES);
}

void actors_create(const char *name)
{
    /*
     * The player, from its kind data: placed by level_generate(). Created
     * first, so entity 0: on a tick where others are due too, its turn
     * comes first (timers fire in ID order)
     */
    const actor_persistent_t *p;
    nge_entity_id_t id;

    actor_player = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    nge_util_assert(actor_player != NGE_ENTITY_INVALID);
    pc_set_name(actor_player, name);

    /* Not placed: each is placed when its depth's level is generated */
    for (p = persistents; p->cls != CLASS_NONE; p++) {
        nge_util_assert(p->depth >= 1 && p->depth <= LEVEL_DEPTH_BOTTOM);
        id = (p->cls == CLASS_CREATURE) ? gen_creature(p->kind, NGE_OWNER_AREA(p->depth))
                                        : gen_item(p->kind, NGE_OWNER_AREA(p->depth));
        nge_util_assert(id != NGE_ENTITY_INVALID);
        nge_entity_set_flags(id, NGE_ENTITY_PERSISTENT);
        if (p->cls == CLASS_ITEM && p->kind == IT_QUEST) {
            quest_set(id);
        }
    }

#if GAME_DEV_KIT
    {
        uint8_t i;

        for (i = 0; i < sizeof(dev_kit); i++) {
            gen_item(dev_kit[i], actor_player);   /* owned, not placed: stowed */
        }
        /* Every damage outcome can be seen with the X key (immune to acid: poison still hurts) */
        effects_set_base(actor_player, ATTR_IMMUNE, DMG_BIT(DMG_ACID));
        effects_set_base(actor_player, ATTR_RESIST, DMG_BIT(DMG_FIRE));
        effects_set_base(actor_player, ATTR_VULN, DMG_BIT(DMG_COLD));
    }
#endif
}
