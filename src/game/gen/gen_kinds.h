#ifndef GEN_KINDS_H
#define GEN_KINDS_H

/**
 * @file gen_kinds.h
 * @brief The entities' kinds: an index into each class's table
 *        (gen_creature_pdata.c, gen_item_pdata.c).
 *
 * The tables use these as designated initialisers ([CR_RAT] = {...}), so
 * the order here is the only order. A future editing tool can generate this
 * file with the tables.
 */

/* Creatures (CLASS_CREATURE) */
#define CR_PLAYER  0
#define CR_RAT     1
#define CR_GOBLIN  2
#define CR_COUNT   3

/* Items (CLASS_ITEM): IT_QUEST is one a game, made by game/actors.c, never at random */
#define IT_CLUB              0
#define IT_DAGGER            1
#define IT_QUARTERSTAFF      2
#define IT_SHORT_SWORD       3
#define IT_HAND_AXE          4
#define IT_SPEAR             5
#define IT_MACE              6
#define IT_SWORD             7
#define IT_BATTLE_AXE        8
#define IT_WAR_HAMMER        9
#define IT_GREATSWORD        10
#define IT_HALBERD           11
#define IT_DAGGER_1          12
#define IT_SWORD_1           13
#define IT_SWORD_2           14
#define IT_SWORD_3           15
#define IT_MACE_1            16
#define IT_BATTLE_AXE_1      17
#define IT_GREATSWORD_2      18
#define IT_RUSTY_SWORD       19
#define IT_VENOM_DAGGER      20
#define IT_FLAME_SWORD       21
#define IT_FROST_AXE         22
#define IT_SLING             23
#define IT_SHORT_BOW         24
#define IT_LONG_BOW          25
#define IT_CROSSBOW          26
#define IT_HEAVY_CROSSBOW    27
#define IT_LONG_BOW_2        28
#define IT_STONES            29
#define IT_IRON_SHOT         30
#define IT_ARROWS            31
#define IT_ARROWS_1          32
#define IT_BOLTS             33
#define IT_BOLTS_1           34
#define IT_DARTS             35
#define IT_JAVELINS          36
#define IT_OIL               37
#define IT_ROBE              38
#define IT_LEATHER_ARMOUR    39
#define IT_RING_MAIL         40
#define IT_SCALE_MAIL        41
#define IT_CHAIN_MAIL        42
#define IT_PLATE_ARMOUR      43
#define IT_LEATHER_ARMOUR_1  44
#define IT_CHAIN_MAIL_1      45
#define IT_PLATE_ARMOUR_2    46
#define IT_CAP               47
#define IT_HELMET            48
#define IT_BRACERS           49
#define IT_VAMBRACES         50
#define IT_GLOVES            51
#define IT_GAUNTLETS         52
#define IT_GAUNTLETS_MIGHT   53
#define IT_LEGGINGS          54
#define IT_GREAVES           55
#define IT_BOOTS             56
#define IT_IRON_BOOTS        57
#define IT_BOOTS_SPEED       58
#define IT_CLOAK             59
#define IT_CLOAK_PROTECTION  60
#define IT_CLOAK_FIRE        61
#define IT_BUCKLER           62
#define IT_SHIELD            63
#define IT_TOWER_SHIELD      64
#define IT_APPLE             65
#define IT_BREAD             66
#define IT_WATERSKIN         67
#define IT_ALE               68
#define IT_PIE               69
#define IT_RATION            70
#define IT_ROAST             71
#define IT_DEAD_RAT          72
#define IT_HEAL_DRAUGHT      73
#define IT_HEAL_POTION       74
#define IT_HEAL_PHILTRE      75
#define IT_POTION_LIFE       76
#define IT_ANTIDOTE          77
#define IT_PANACEA           78
#define IT_MANA_DRAUGHT      79
#define IT_MANA_POTION       80
#define IT_ELIXIR            81
#define IT_POTION_STR        82
#define IT_POTION_DEX        83
#define IT_POTION_CON        84
#define IT_POTION_INT        85
#define IT_POTION_WIS        86
#define IT_POTION_CHA        87
#define IT_POTION_HASTE      88
#define IT_POTION_MIGHT      89
#define IT_POTION_STONE      90
#define IT_POTION_REGEN      91
#define IT_POTION_FIRE       92
#define IT_POTION_COLD       93
#define IT_POTION_POISON     94
#define IT_POTION_SLEEP      95
#define IT_POTION_BLIND      96
#define IT_POTION_SLOW       97
#define IT_POTION_WEAK       98
#define IT_SP_LIGHT          99
#define IT_SP_IDENTIFY       100
#define IT_SP_MAP            101
#define IT_SP_TELEPORT       102
#define IT_SP_UNCURSE        103
#define IT_SP_ENCHANT        104
#define IT_SP_SUMMON         105
#define IT_SP_MISSILE        106
#define IT_SP_FIRE           107
#define IT_SP_FROST          108
#define IT_SP_ACID           109
#define IT_SP_SLEEP          110
#define IT_WAND_STRIKING     111
#define IT_WAND_SLOW         112
#define IT_WAND_SLEEP        113
#define IT_WAND_FIRE         114
#define IT_WAND_FROST        115
#define IT_WAND_TELEPORT     116
#define IT_STAFF_POWER       117
#define IT_STAFF_HEALING     118
#define IT_RING_HEALTH_1     119
#define IT_RING_HEALTH_2     120
#define IT_RING_HEALTH_3     121
#define IT_RING_ARMOUR_1     122
#define IT_RING_ARMOUR_2     123
#define IT_RING_STR_1        124
#define IT_RING_DEX_1        125
#define IT_RING_CON_1        126
#define IT_RING_INT_1        127
#define IT_RING_WIS_1        128
#define IT_RING_SPEED_1      129
#define IT_RING_FIRE         130
#define IT_RING_COLD         131
#define IT_RING_VENOM        132
#define IT_RING_RENEWAL      133
#define IT_RING_WEAKNESS     134
#define IT_RING_CLUMSY       135
#define IT_RING_FRAILTY      136
#define IT_AMULET_HEALTH     137
#define IT_AMULET_GUARDING   138
#define IT_AMULET_ACID       139
#define IT_AMULET_WARDING    140
#define IT_AMULET_FOLLY      141
#define IT_TORCH             142
#define IT_LANTERN           143
#define IT_MAGIC_LANTERN     144
#define IT_KEY_IRON          145
#define IT_KEY_BRASS         146
#define IT_KEY_SILVER        147
#define IT_KEY_GOLD          148
#define IT_KEY_SKELETON      149
#define IT_LOCK_PICK         150
#define IT_FINE_LOCK_PICK    151
#define IT_GOLD_SMALL        152
#define IT_GOLD_PILE         153
#define IT_GOLD_HEAP         154
#define IT_GOLD_HOARD        155
#define IT_GEM_OPAL          156
#define IT_GEM_RUBY          157
#define IT_GEM_DIAMOND       158
#define IT_QUEST             159
#define IT_COUNT             160

/* Features (CLASS_FEATURE) */
#define FT_DOOR         0
#define FT_STAIRS_UP    1
#define FT_STAIRS_DOWN  2
#define FT_COUNT        3

/* Item subclasses, in each item's kind record */
#define ITEM_POTION     0
#define ITEM_WEAPON     1    /* melee */
#define ITEM_RING       2
#define ITEM_FOOD       3
#define ITEM_QUEST      4    /* the quest item (docs/winning.md) */
#define ITEM_RANGED     5    /* a launcher */
#define ITEM_AMMO       6    /* what a launcher shoots, or what is thrown */
#define ITEM_ARMOUR     7    /* shields too */
#define ITEM_DRINK      8
#define ITEM_PARCHMENT  9
#define ITEM_WAND       10   /* staves too */
#define ITEM_AMULET     11
#define ITEM_LIGHT      12
#define ITEM_KEY        13
#define ITEM_TOOL       14
#define ITEM_TREASURE   15
#define ITEM_COUNT      16

/* Item flags, in each item's kind record: stored now, their rules later (docs/items.md R25) */
#define ITEMF_TWO_HANDS  0x01   /* fills SLOT_SHIELD too */
#define ITEMF_CURSED     0x02   /* can't be taken off */

/* An item kind's group: what ammo works with what launcher (GROUP_*) */
#define GROUP_NONE    0
#define GROUP_ARROW   1
#define GROUP_BOLT    2
#define GROUP_STONE   3
#define GROUP_THROWN  4   /* needs no launcher */
#define GROUP_COUNT   5

/* ...or which locks a key opens (KEY_*); a lock pick's group is its bonus */
#define KEY_IRON    1
#define KEY_BRASS   2
#define KEY_SILVER  3
#define KEY_GOLD    4
#define KEY_ANY     5
#define KEY_COUNT   6

#define KIND_INVALID 0xFF /* Invalid kind */

#endif
