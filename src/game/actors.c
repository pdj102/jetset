#include "game/actors.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "ecs/nge_owner.h"
#include "game/dev.h"
#include "game/components/attrs.h"
#include "game/components/health.h"
#include "game/components/kind.h"
#include "game/effects/effects.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "sprite/nge_sprite.h"

#if GAME_DEV_KIT
/* The development kit's items, owned by the player: in their pack */
static const uint8_t dev_kit[] = {
    IT_SWORD, IT_RING, IT_RING, IT_RING, IT_POTION, IT_ANTIDOTE, IT_FOOD, IT_ELIXIR, IT_STRENGTH,
};
#endif

/* A persistent entity: made once a game, in its depth's area (game/level/level.h) */
typedef struct actor_persistent {
    uint8_t cls;     /* CLASS_CREATURE or CLASS_ITEM */
    uint8_t kind;    /* CR_* or IT_* */
    uint8_t depth;   /* 1 to LEVEL_DEPTH_BOTTOM */
} actor_persistent_t;

/* The persistent entities besides the player: later the quest object and the final boss */
static const actor_persistent_t persistents[] = {
#if GAME_DEV_KIT
    { CLASS_CREATURE, CR_GOBLIN, 2 },   /* for trying out parking: the same goblin on every visit */
#endif
    { CLASS_NONE, 0, 0 },               /* the end */
};

nge_entity_id_t actor_player;

void actors_load_graphics(void)
{
    /* The loaders map the graphics' page into slot 7 while they copy */
    nge_sprite_load_patterns(DEMO_SPRITE_PAGE, 0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
    nge_sprite_load_palette(DEMO_SPRITE_PAGE, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);
}

void actors_create(void)
{
    /*
     * The player, from its base data: placed by level_generate(). Created
     * first, so entity 0: on a tick where others are due too, its turn
     * comes first (timers fire in ID order)
     */
    const actor_persistent_t *p;
    nge_entity_id_t id;

    actor_player = gen_creature(CR_PLAYER, NGE_OWNER_WORLD);
    nge_util_assert(actor_player != NGE_ENTITY_INVALID);

    /* Not placed: each is placed when its depth's level is generated */
    for (p = persistents; p->cls != CLASS_NONE; p++) {
        nge_util_assert(p->depth >= 1 && p->depth <= LEVEL_DEPTH_BOTTOM);
        id = (p->cls == CLASS_CREATURE) ? gen_creature(p->kind, NGE_OWNER_AREA(p->depth))
                                        : gen_item(p->kind, NGE_OWNER_AREA(p->depth));
        nge_util_assert(id != NGE_ENTITY_INVALID);
        nge_entity_set_flags(id, NGE_ENTITY_PERSISTENT);
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
