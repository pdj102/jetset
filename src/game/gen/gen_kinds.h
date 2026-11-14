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
 *
 * Kind 0 of each class (CR_NONE, IT_NONE, FT_NONE) means none: its table row
 * is blank, with rarity 0, so it is never generated, and a field that holds
 * a kind or none can leave it out of a row (docs/creature_extensions.md
 * D11). New kinds go at the end, before the class's _COUNT, so no number
 * moves. KIND_INVALID is something else: no kind at all, in kind_index[].
 */

/* Creatures (CLASS_CREATURE) */
#define CR_NONE           0   /* none: a blank row 0 in the kind table, never generated */
#define CR_PLAYER         1
#define CR_RAT            2
#define CR_GOBLIN         3
#define CR_BAT            4
#define CR_GREEN_SNAKE    5
#define CR_LICHEN         6
#define CR_GREEN_SLIME    7
#define CR_SHRIEKER       8
#define CR_BANDIT         9
#define CR_ORC            10
#define CR_GIANT_SPIDER   11
#define CR_SKELETON       12
#define CR_GOBLIN_SHAMAN  13
#define CR_GOLD_SNAKE     14
#define CR_FLOATING_EYE   15
#define CR_MIMIC          16
#define CR_OGRE           17
#define CR_GHOST          18
#define CR_WRAITH         19
#define CR_TROLL          20
#define CR_DRAGON         21
#define CR_FIGHTER        22   /* the player as a Fighter (game/chargen); CR_PLAYER is the Dev */
#define CR_MAGE           23   /* the player as a Mage */
#define CR_COUNT          24

/* Items (CLASS_ITEM): IT_QUEST is one a game, made by game/actors.c, never at random */
#define IT_NONE              0   /* none: a blank row 0 in the kind table, never generated */
#define IT_CLUB              1
#define IT_DAGGER            2
#define IT_QUARTERSTAFF      3
#define IT_SHORT_SWORD       4
#define IT_HAND_AXE          5
#define IT_SPEAR             6
#define IT_MACE              7
#define IT_SWORD             8
#define IT_BATTLE_AXE        9
#define IT_WAR_HAMMER        10
#define IT_GREATSWORD        11
#define IT_HALBERD           12
#define IT_DAGGER_1          13
#define IT_SWORD_1           14
#define IT_SWORD_2           15
#define IT_SWORD_3           16
#define IT_MACE_1            17
#define IT_BATTLE_AXE_1      18
#define IT_GREATSWORD_2      19
#define IT_RUSTY_SWORD       20
#define IT_VENOM_DAGGER      21
#define IT_FLAME_SWORD       22
#define IT_FROST_AXE         23
#define IT_SLING             24
#define IT_SHORT_BOW         25
#define IT_LONG_BOW          26
#define IT_CROSSBOW          27
#define IT_HEAVY_CROSSBOW    28
#define IT_LONG_BOW_2        29
#define IT_STONES            30
#define IT_IRON_SHOT         31
#define IT_ARROWS            32
#define IT_ARROWS_1          33
#define IT_BOLTS             34
#define IT_BOLTS_1           35
#define IT_DARTS             36
#define IT_JAVELINS          37
#define IT_OIL               38
#define IT_ROBE              39
#define IT_LEATHER_ARMOUR    40
#define IT_RING_MAIL         41
#define IT_SCALE_MAIL        42
#define IT_CHAIN_MAIL        43
#define IT_PLATE_ARMOUR      44
#define IT_LEATHER_ARMOUR_1  45
#define IT_CHAIN_MAIL_1      46
#define IT_PLATE_ARMOUR_2    47
#define IT_CAP               48
#define IT_HELMET            49
#define IT_BRACERS           50
#define IT_VAMBRACES         51
#define IT_GLOVES            52
#define IT_GAUNTLETS         53
#define IT_GAUNTLETS_MIGHT   54
#define IT_LEGGINGS          55
#define IT_GREAVES           56
#define IT_BOOTS             57
#define IT_IRON_BOOTS        58
#define IT_BOOTS_SPEED       59
#define IT_CLOAK             60
#define IT_CLOAK_PROTECTION  61
#define IT_CLOAK_FIRE        62
#define IT_BUCKLER           63
#define IT_SHIELD            64
#define IT_TOWER_SHIELD      65
#define IT_APPLE             66
#define IT_BREAD             67
#define IT_WATERSKIN         68
#define IT_ALE               69
#define IT_PIE               70
#define IT_RATION            71
#define IT_ROAST             72
#define IT_DEAD_RAT          73
#define IT_HEAL_DRAUGHT      74
#define IT_HEAL_POTION       75
#define IT_HEAL_PHILTRE      76
#define IT_POTION_LIFE       77
#define IT_ANTIDOTE          78
#define IT_PANACEA           79
#define IT_MANA_DRAUGHT      80
#define IT_MANA_POTION       81
#define IT_ELIXIR            82
#define IT_POTION_STR        83
#define IT_POTION_DEX        84
#define IT_POTION_CON        85
#define IT_POTION_INT        86
#define IT_POTION_WIS        87
#define IT_POTION_CHA        88
#define IT_POTION_HASTE      89
#define IT_POTION_MIGHT      90
#define IT_POTION_STONE      91
#define IT_POTION_REGEN      92
#define IT_POTION_FIRE       93
#define IT_POTION_COLD       94
#define IT_POTION_POISON     95
#define IT_POTION_SLEEP      96
#define IT_POTION_BLIND      97
#define IT_POTION_SLOW       98
#define IT_POTION_WEAK       99
#define IT_SP_LIGHT          100
#define IT_SP_IDENTIFY       101
#define IT_SP_MAP            102
#define IT_SP_TELEPORT       103
#define IT_SP_UNCURSE        104
#define IT_SP_ENCHANT        105
#define IT_SP_SUMMON         106
#define IT_SP_MISSILE        107
#define IT_SP_FIRE           108
#define IT_SP_FROST          109
#define IT_SP_ACID           110
#define IT_SP_SLEEP          111
#define IT_WAND_STRIKING     112
#define IT_WAND_SLOW         113
#define IT_WAND_SLEEP        114
#define IT_WAND_FIRE         115
#define IT_WAND_FROST        116
#define IT_WAND_TELEPORT     117
#define IT_STAFF_POWER       118
#define IT_STAFF_HEALING     119
#define IT_RING_HEALTH_1     120
#define IT_RING_HEALTH_2     121
#define IT_RING_HEALTH_3     122
#define IT_RING_ARMOUR_1     123
#define IT_RING_ARMOUR_2     124
#define IT_RING_STR_1        125
#define IT_RING_DEX_1        126
#define IT_RING_CON_1        127
#define IT_RING_INT_1        128
#define IT_RING_WIS_1        129
#define IT_RING_SPEED_1      130
#define IT_RING_FIRE         131
#define IT_RING_COLD         132
#define IT_RING_VENOM        133
#define IT_RING_RENEWAL      134
#define IT_RING_WEAKNESS     135
#define IT_RING_CLUMSY       136
#define IT_RING_FRAILTY      137
#define IT_AMULET_HEALTH     138
#define IT_AMULET_GUARDING   139
#define IT_AMULET_ACID       140
#define IT_AMULET_WARDING    141
#define IT_AMULET_FOLLY      142
#define IT_TORCH             143
#define IT_LANTERN           144
#define IT_MAGIC_LANTERN     145
#define IT_KEY_IRON          146
#define IT_KEY_BRASS         147
#define IT_KEY_SILVER        148
#define IT_KEY_GOLD          149
#define IT_KEY_SKELETON      150
#define IT_LOCK_PICK         151
#define IT_FINE_LOCK_PICK    152
#define IT_GOLD_SMALL        153
#define IT_GOLD_PILE         154
#define IT_GOLD_HEAP         155
#define IT_GOLD_HOARD        156
#define IT_GEM_OPAL          157
#define IT_GEM_RUBY          158
#define IT_GEM_DIAMOND       159
#define IT_QUEST             160
#define IT_BOOK_BASIC        161   /* spell books (docs/spells.md 10.5.3): added after IT_QUEST */
#define IT_BOOK_ADEPT        162
#define IT_BOOK_MASTER       163
#define IT_SP_LEVEL_TELEPORT 164   /* docs/effects_extensions.md: added at the end */
#define IT_POTION_LEVITATE   165
#define IT_BOOTS_LEVITATE    166
#define IT_SERRATED_SWORD    167   /* docs/items_extensions.md: added at the end */
#define IT_BARBED_SPEAR      168
#define IT_ACID_BLADE        169
#define IT_FIRE_BRAND        170
#define IT_BARBED_ARROWS     171
#define IT_FIRE_ARROWS       172
#define IT_POISON_DARTS      173
#define IT_SLEEP_DARTS       174
#define IT_ACID_FLASK        175
#define IT_WAYBREAD          176
#define IT_MUSHROOM_PALE     177
#define IT_MUSHROOM_VIOLET   178
#define IT_MUSHROOM_GREEN    179
#define IT_POTION_GRACE      180
#define IT_POTION_ENDURANCE  181
#define IT_POTION_INSIGHT    182
#define IT_POTION_INTUITION  183
#define IT_ANTIVENOM         184
#define IT_POTION_ACID       185
#define IT_POTION_WARDING    186
#define IT_POTION_HEROISM    187
#define IT_POTION_AID        188
#define IT_EYE_DROPS         189
#define IT_POTION_CONFUSE    190
#define IT_POTION_DREAD      191
#define IT_POTION_PARALYSE   192
#define IT_SP_DETECT         193
#define IT_SP_SHIELD         194
#define IT_SP_BLESS          195
#define IT_SP_DESCENT        196
#define IT_SP_RECALL         197
#define IT_SP_TERROR         198
#define IT_SP_CONFUSE        199
#define IT_SP_HOLD           200
#define IT_SP_HEX            201
#define IT_SP_BANE           202
#define IT_WAND_ACID         203
#define IT_WAND_WEAKNESS     204
#define IT_RING_CHA_1        205
#define IT_AMULET_FIRE       206
#define IT_AMULET_COLD       207
#define IT_AMULET_POISON     208
#define IT_MAIL_FIRE_WARD    209   /* ego armour (docs/items.md 4.11) */
#define IT_DRAGON_MAIL       210
#define IT_ELVEN_CLOAK       211
#define IT_SHIELD_WARDING    212
#define IT_SP_BLINK          213   /* aimed at a tile or an area (docs/items_aimed.md 4.10) */
#define IT_STAFF_BLINK       214
#define IT_WAND_FIREBALL     215
#define IT_WAND_FROSTBALL    216
#define IT_SP_FIREBALL       217
#define IT_SP_PANIC          218
#define IT_COUNT             219

/* Features (CLASS_FEATURE) */
#define FT_NONE         0   /* none: a blank row 0 in the kind table, never generated */
#define FT_DOOR         1
#define FT_STAIRS_UP    2
#define FT_STAIRS_DOWN  3
#define FT_BOULDER      4
#define FT_STATUE       5
#define FT_CRACKED_STATUE 6
#define FT_SHRINE_HEALING 7
#define FT_SHRINE_CLEANSING 8
#define FT_ALTAR_MIGHT  9
#define FT_ALTAR_ANCIENT 10
#define FT_FOUNTAIN_RENEW 11
#define FT_FOUNTAIN_MANA 12
#define FT_FOUNTAIN_FOUL 13
#define FT_CHEST        14
#define FT_DART_TRAP    15
#define FT_PIT          16
#define FT_FIRE_RUNE    17
#define FT_WEB          18
#define FT_SLEEP_GAS    19
#define FT_TELEPORT_RUNE 20
#define FT_SUMMON_RUNE  21
#define FT_TRAPDOOR     22
#define FT_LEVEL_RUNE   23
#define FT_COUNT        24

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
#define ITEM_BOOK       16   /* a spell book: its data is a SPELL_BOOK_* (docs/spells.md 10.5.3) */
#define ITEM_COUNT      17

/* Item flags, in each item's kind record: their rules are in the inventory component */
#define ITEMF_TWO_HANDS  0x01   /* fills SLOT_SHIELD too */
#define ITEMF_CURSED     0x02   /* can't be taken off, until uncursed (ITEM_UNCURSED) */

/*
 * An aimed item's shape (SPELL_*): a field of three bits in the flags, not
 * flags, so a kind has one. A row sets one value; none is SPELL_BOLT, and
 * 0x14 to 0x1C are invalid. Read it with gen_item_shape()
 */
#define ITEMF_SHAPE      0x1C   /* the field's mask */
#define ITEMF_DIRECT     0x04   /* SPELL_DIRECT: past creatures to the chosen tile (ACT_LINE_DIRECT) */
#define ITEMF_TILE       0x08   /* SPELL_TILE: its user, to a free tile */
#define ITEMF_BALL       0x0C   /* SPELL_BALL: every creature around the chosen tile, the user too */
#define ITEMF_BURST      0x10   /* SPELL_BURST: every creature around the user, not the user */

/* An ammo group: what ammo works with what launcher (GROUP_*) */
#define GROUP_NONE    0
#define GROUP_ARROW   1
#define GROUP_BOLT    2
#define GROUP_STONE   3
#define GROUP_THROWN  4   /* needs no launcher */
#define GROUP_COUNT   5

/* Which locks a key opens (KEY_*) */
#define KEY_NONE    0   /* not a key: opens no lock */
#define KEY_IRON    1
#define KEY_BRASS   2
#define KEY_SILVER  3
#define KEY_GOLD    4
#define KEY_ANY     5
#define KEY_COUNT   6

/* A key's kind from the KEY_* it opens: the level generator makes the key to a lock */
typedef char gen_key_kinds_check[(IT_KEY_GOLD - IT_KEY_IRON == KEY_GOLD - KEY_IRON) ? 1 : -1];

#define KIND_INVALID 0xFF /* Invalid kind */

#endif
