/* game/chargen: the classes, and making the player of one (docs/character_creation.md). */
#include "test.h"

#include <string.h>

#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "event/nge_event.h"
#include "map/nge_map.h"

#include "game/chargen/chargen.h"
#include "game/attrs/attrs_internal.h"
#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/item.h"
#include "game/components/kind.h"
#include "game/components/mana.h"
#include "game/components/pc.h"
#include "game/components/stats.h"
#include "game/dev.h"
#include "game/gen/gen.h"
#include "game/spells/spells.h"

/* What actors.c and the main screen provide in the game (not linked here) */
nge_entity_id_t actor_player;
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

/* The item of kind it the player owns (stowed or equipped), or NGE_ENTITY_INVALID */
static nge_entity_id_t carried(uint8_t it)
{
    nge_entity_id_t i;

    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        if (NGE_ENTITY_IS_ALIVE(i) && NGE_ENTITY_HAS_COMPONENT(i, COMPONENT_ITEM) &&
            nge_owner_get(i) == actor_player && kind_index[i] == it) {
            return i;
        }
    }
    return NGE_ENTITY_INVALID;
}

/* The items the player owns, stowed or equipped */
static int items(void)
{
    nge_entity_id_t i;
    int n = 0;

    for (i = 0; i < NGE_ENTITY_MAX; i++) {
        n += NGE_ENTITY_IS_ALIVE(i) && NGE_ENTITY_HAS_COMPONENT(i, COMPONENT_ITEM) && nge_owner_get(i) == actor_player;
    }
    return n;
}

/* The item of kind it is carried, in slot (SLOT_NONE: carried, not equipped) */
static int in_slot(uint8_t it, uint8_t slot)
{
    nge_entity_id_t i = carried(it);

    return i != NGE_ENTITY_INVALID && item_slot[i] == slot;
}

static int abilities(uint8_t str, uint8_t dex, uint8_t con, uint8_t in, uint8_t wis, uint8_t cha)
{
    nge_entity_id_t p = actor_player;

    return stats_ability[ABIL_STR][p] == str && stats_ability[ABIL_DEX][p] == dex &&
           stats_ability[ABIL_CON][p] == con && stats_ability[ABIL_INT][p] == in &&
           stats_ability[ABIL_WIS][p] == wis && stats_ability[ABIL_CHA][p] == cha;
}

int main(void)
{
    char buf[CHARGEN_NAME_MAX + 1];
    uint8_t used_fighter;
    uint8_t used_mage;

    test_init();

    /* The offer: the Dev only in debug builds */
    CHECK(CHARGEN_CLASSES == (GAME_DEV_KIT ? 3 : 2));

    /* ---- the Fighter: no mana, no spells; the kit worn; the default name ---- */
    chargen_make(PC_CLASS_FIGHTER, "");
    CHECK(actor_player == 0 && PC_IS(actor_player) && pc_class == PC_CLASS_FIGHTER);
    CHECK(kind_index[actor_player] == CR_FIGHTER);
    CHECK(strcmp(pc_name, "Bran") == 0);
    CHECK(abilities(16, 14, 14, 9, 9, 10));
    CHECK(health_hp_max[actor_player] == 12 && health_hp[actor_player] == 12);
    CHECK(!MANA_IS(actor_player) && !SPELLS_IS(actor_player));
    CHECK(in_slot(IT_SHORT_SWORD, SLOT_WEAPON));
    CHECK(in_slot(IT_LEATHER_ARMOUR, SLOT_BODY));
    CHECK(in_slot(IT_SHIELD, SLOT_SHIELD));
    CHECK(items() == 3 && inv_count(actor_player) == 0);   /* all equipped */
    CHECK(health_ac[actor_player] > health_ac_base[actor_player]);   /* the armour and shield count */
    CHECK(nge_event_count() == 0);                                     /* no "You wield..." */
    used_fighter = (uint8_t)(NGE_ENTITY_MAX - nge_entity_free_count());

    /* ---- the Mage, made over the Fighter: frail, with mana and Magic Bolt; a typed name ---- */
    chargen_make(PC_CLASS_MAGE, "Ana");
    CHECK(actor_player == 0 && pc_class == PC_CLASS_MAGE && kind_index[actor_player] == CR_MAGE);
    CHECK(strcmp(pc_name, "Ana") == 0);
    CHECK(abilities(9, 10, 12, 16, 14, 10));
    CHECK(health_hp_max[actor_player] == 8);
    CHECK(MANA_IS(actor_player) && mana_mp_max[actor_player] == 8 && mana_mp[actor_player] == 8);
    CHECK(SPELLS_IS(actor_player) && spell_knows(actor_player, SP_MAGIC_BOLT));
    CHECK(in_slot(IT_DAGGER, SLOT_WEAPON));
    CHECK(in_slot(IT_SLING, SLOT_RANGED));
    CHECK(in_slot(IT_STONES, SLOT_AMMO));
    CHECK(items() == 3 && carried(IT_SHORT_SWORD) == NGE_ENTITY_INVALID);
    CHECK(nge_event_count() == 0);
    used_mage = (uint8_t)(NGE_ENTITY_MAX - nge_entity_free_count());
    CHECK(used_mage == used_fighter + 1);   /* the same, and mana's regeneration: nothing of the Fighter's is left */
    chargen_make(PC_CLASS_MAGE, "Ana");
    CHECK((uint8_t)(NGE_ENTITY_MAX - nge_entity_free_count()) == used_mage);

    /* A long name is cut to PC_NAME_MAX */
    chargen_make(PC_CLASS_MAGE, "Bartholomew");
    CHECK(strlen(pc_name) == PC_NAME_MAX);

#if GAME_DEV_KIT
    /* ---- the Dev: 12s; its four equipped, the rest of the development kit carried ---- */
    chargen_make(PC_CLASS_DEV, "");
    CHECK(pc_class == PC_CLASS_DEV && kind_index[actor_player] == CR_PLAYER);
    CHECK(strcmp(pc_name, "Dev") == 0);
    CHECK(abilities(12, 12, 12, 12, 12, 12));
    CHECK(MANA_IS(actor_player) && mana_mp_max[actor_player] == 5 && SPELLS_IS(actor_player));
    CHECK(in_slot(IT_SWORD, SLOT_WEAPON));
    CHECK(in_slot(IT_PLATE_ARMOUR_2, SLOT_BODY));
    CHECK(in_slot(IT_SHORT_BOW, SLOT_RANGED));
    CHECK(in_slot(IT_RING_FRAILTY, SLOT_NONE) && in_slot(IT_POTION_HASTE, SLOT_NONE));   /* cursed: not put on */
    CHECK(items() == 17 && inv_count(actor_player) == 13);
    CHECK(health_immune[actor_player] == DMG_BIT(DMG_ACID) && health_resist[actor_player] == DMG_BIT(DMG_FIRE));
    CHECK(nge_event_count() == 0);
#endif

    /* A class that isn't offered */
    TEST_EXPECT_HALT(chargen_make(CHARGEN_CLASSES, ""));

    /* ---- the names ---- */
    chargen_class_name(PC_CLASS_FIGHTER, buf);
    CHECK(strcmp(buf, "Fighter") == 0);
    chargen_class_name(PC_CLASS_MAGE, buf);
    CHECK(strcmp(buf, "Mage") == 0);
    chargen_class_name(PC_CLASS_DEV, buf);
    CHECK(strcmp(buf, "Dev") == 0);
    TEST_EXPECT_HALT(chargen_class_name(PC_CLASS_COUNT, buf));

    return test_summary("chargen");
}
