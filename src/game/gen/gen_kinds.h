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
#define CR_PLAYER         0
#define CR_RAT            1
#define CR_GOBLIN         2
#define CR_BAT            3
#define CR_GREEN_SNAKE    4
#define CR_LICHEN         5
#define CR_GREEN_SLIME    6
#define CR_SHRIEKER       7
#define CR_BANDIT         8
#define CR_ORC            9
#define CR_GIANT_SPIDER   10
#define CR_SKELETON       11
#define CR_GOBLIN_SHAMAN  12
#define CR_GOLD_SNAKE     13
#define CR_FLOATING_EYE   14
#define CR_MIMIC          15
#define CR_OGRE           16
#define CR_GHOST          17
#define CR_WRAITH         18
#define CR_TROLL          19
#define CR_DRAGON         20
#define CR_FIGHTER        21   /* the player as a Fighter (game/chargen); CR_PLAYER is the Dev */
#define CR_MAGE           22   /* the player as a Mage */
#define CR_COUNT          23

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
#define IT_BOOK_BASIC        160   /* spell books (docs/spells.md 10.5.3): after IT_QUEST, so no number moves */
#define IT_BOOK_ADEPT        161
#define IT_BOOK_MASTER       162
#define IT_SP_LEVEL_TELEPORT 163   /* docs/effects_extensions.md: added at the end, so no number moves */
#define IT_POTION_LEVITATE   164
#define IT_BOOTS_LEVITATE    165
#define IT_SERRATED_SWORD    166   /* docs/items_extensions.md: added at the end, so no number moves */
#define IT_BARBED_SPEAR      167
#define IT_ACID_BLADE        168
#define IT_FIRE_BRAND        169
#define IT_BARBED_ARROWS     170
#define IT_FIRE_ARROWS       171
#define IT_POISON_DARTS      172
#define IT_SLEEP_DARTS       173
#define IT_ACID_FLASK        174
#define IT_WAYBREAD          175
#define IT_MUSHROOM_PALE     176
#define IT_MUSHROOM_VIOLET   177
#define IT_MUSHROOM_GREEN    178
#define IT_POTION_GRACE      179
#define IT_POTION_ENDURANCE  180
#define IT_POTION_INSIGHT    181
#define IT_POTION_INTUITION  182
#define IT_ANTIVENOM         183
#define IT_POTION_ACID       184
#define IT_POTION_WARDING    185
#define IT_POTION_HEROISM    186
#define IT_POTION_AID        187
#define IT_EYE_DROPS         188
#define IT_POTION_CONFUSE    189
#define IT_POTION_DREAD      190
#define IT_POTION_PARALYSE   191
#define IT_SP_DETECT         192
#define IT_SP_SHIELD         193
#define IT_SP_BLESS          194
#define IT_SP_DESCENT        195
#define IT_SP_RECALL         196
#define IT_SP_TERROR         197
#define IT_SP_CONFUSE        198
#define IT_SP_HOLD           199
#define IT_SP_HEX            200
#define IT_SP_BANE           201
#define IT_WAND_ACID         202
#define IT_WAND_WEAKNESS     203
#define IT_RING_CHA_1        204
#define IT_AMULET_FIRE       205
#define IT_AMULET_COLD       206
#define IT_AMULET_POISON     207
#define IT_MAIL_FIRE_WARD    208   /* ego armour (docs/items.md 4.11) */
#define IT_DRAGON_MAIL       209
#define IT_ELVEN_CLOAK       210
#define IT_SHIELD_WARDING    211
#define IT_SP_BLINK          212   /* aimed at a tile or an area (docs/items_aimed.md 4.10) */
#define IT_STAFF_BLINK       213
#define IT_WAND_FIREBALL     214
#define IT_WAND_FROSTBALL    215
#define IT_SP_FIREBALL       216
#define IT_SP_PANIC          217
#define IT_COUNT             218

/* Features (CLASS_FEATURE) */
#define FT_DOOR         0
#define FT_STAIRS_UP    1
#define FT_STAIRS_DOWN  2
#define FT_BOULDER      3
#define FT_STATUE       4
#define FT_CRACKED_STATUE 5
#define FT_SHRINE_HEALING 6
#define FT_SHRINE_CLEANSING 7
#define FT_ALTAR_MIGHT  8
#define FT_ALTAR_ANCIENT 9
#define FT_FOUNTAIN_RENEW 10
#define FT_FOUNTAIN_MANA 11
#define FT_FOUNTAIN_FOUL 12
#define FT_CHEST        13
#define FT_DART_TRAP    14
#define FT_PIT          15
#define FT_FIRE_RUNE    16
#define FT_WEB          17
#define FT_SLEEP_GAS    18
#define FT_TELEPORT_RUNE 19
#define FT_SUMMON_RUNE  20
#define FT_TRAPDOOR     21
#define FT_LEVEL_RUNE   22
#define FT_COUNT        23

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
