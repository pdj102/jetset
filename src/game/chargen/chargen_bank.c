/*
 * Character creation: banked in page 140 with the class table and its
 * literals (a banked game module, see "Banked game modules" in
 * docs/memory_map.md). Names are copied to the stack before they go to
 * another page (N-BANK-5).
 */
#ifdef __SDCC
#pragma codeseg PAGE_140
#pragma constseg PAGE_140
#endif

#include "game/chargen/chargen.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"

#include "game/actions/act.h"
#include "game/actors.h"
#include "game/chargen/chargen_internal.h"
#include "game/attrs/attrs.h"
#include "game/components/health.h"
#include "game/components/item.h"
#include "game/components/pc.h"
#include "game/gen/gen.h"

static const chargen_kit_t kit_fighter[] = {
    { IT_SHORT_SWORD, 1 }, { IT_LEATHER_ARMOUR, 1 }, { IT_SHIELD, 1 },
};

static const chargen_kit_t kit_mage[] = {
    { IT_DAGGER, 1 }, { IT_SLING, 1 }, { IT_STONES, 1 }, { IT_BOOK_BASIC, 0 },
};

/* The development kit (game/dev.h): the pack keeps 2 places free */
static const chargen_kit_t kit_dev[] = {
    { IT_SWORD, 1 }, { IT_PLATE_ARMOUR_2, 1 }, { IT_SHORT_BOW, 1 }, { IT_ARROWS, 1 }, { IT_BOOK_BASIC, 0 },
#if GAME_DEV_KIT
    { IT_HEAL_DRAUGHT, 0 }, { IT_STAFF_HEALING, 0 }, { IT_RING_HEALTH_2, 0 }, { IT_RING_FRAILTY, 0 },
    { IT_POTION_HASTE, 0 }, { IT_WAND_SLEEP, 0 }, { IT_LOCK_PICK, 0 }, { IT_WAND_FIRE, 0 },
    { IT_LANTERN, 0 }, { IT_DARTS, 0 }, { IT_BREAD, 0 }, { IT_OIL, 0 }, { IT_ARROWS, 0 },
#endif
};

#define KIT(k)  (k), (uint8_t)(sizeof(k) / sizeof((k)[0]))

const chargen_class_t chargen_classes[PC_CLASS_COUNT] = {
    [PC_CLASS_FIGHTER] = { "Fighter", "Bran",  CR_FIGHTER, KIT(kit_fighter) },
    [PC_CLASS_MAGE]    = { "Mage",    "Elwyn", CR_MAGE,    KIT(kit_mage) },
    [PC_CLASS_DEV]     = { "Dev",     "Dev",   CR_PLAYER,  KIT(kit_dev) },
};

/* Copy a literal of this page into buf (on the caller's stack), max characters at most */
static void copy_name(char *buf, const char *from, uint8_t max)
{
    uint8_t i;

    for (i = 0; i < max && from[i] != '\0'; i++) {
        buf[i] = from[i];
    }
    buf[i] = '\0';
}

void chargen_make(uint8_t cls, const char *name) NGE_BANKED
{
    char own[PC_NAME_MAX + 1];   /* the default name, on the stack for page 110 */
    const chargen_class_t *c;
    const chargen_kit_t *k;
    nge_entity_id_t item;
    uint8_t i;
    uint8_t ok;

    nge_util_assert(cls < CHARGEN_CLASSES);
    nge_util_assert(name != NULL);
    c = &chargen_classes[cls];

    /* An empty world: the last game's (or the last preview's) entities are gone */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();

    actor_player = gen_creature(c->creature, NGE_OWNER_WORLD);
    nge_util_assert(actor_player == 0);   /* first: its turn comes first on a shared tick */
    pc_set_class(actor_player, cls);
    if (name[0] == '\0') {
        copy_name(own, c->default_name, PC_NAME_MAX);
        pc_set_name(actor_player, own);
    } else {
        pc_set_name(actor_player, name);
    }

    for (i = 0, k = c->kit; i < c->kit_count; i++, k++) {
        item = gen_item(k->kind, actor_player);   /* owned, not placed: stowed */
        nge_util_assert(item != NGE_ENTITY_INVALID);
        if (k->equip) {
            act_t a = { 0 };

            a.actor = actor_player;
            a.item = item;
            a.slot = SLOT_NONE;   /* the first free slot it fits */
            ok = act_equip(&a, ACT_DO);
            nge_util_assert(ok == ACT_SUCCESS);
            (void)ok;
        }
    }

#if GAME_DEV_KIT
    if (cls == PC_CLASS_DEV) {
        /* Every damage outcome can be seen with the X key (immune to acid: poison still hurts) */
        effects_set_base(actor_player, ATTR_IMMUNE, DMG_BIT(DMG_ACID));
        effects_set_base(actor_player, ATTR_RESIST, DMG_BIT(DMG_FIRE));
        effects_set_base(actor_player, ATTR_VULN, DMG_BIT(DMG_COLD));
    }
#endif

    nge_event_init();   /* no "You wield..." when play starts: the screen is drawn afresh */
}

void chargen_class_name(uint8_t cls, char *buf) NGE_BANKED
{
    nge_util_assert(cls < PC_CLASS_COUNT);
    nge_util_assert(buf != NULL);

    copy_name(buf, chargen_classes[cls].name, CHARGEN_NAME_MAX);
}
