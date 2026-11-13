# Creature extensions: more classic monsters, and what makes them work

Status: agreed (2026-11-13). D1 to D6 and D8 to D10 agreed as proposed. D7 was
revised on review: `ai_can_t` widens to 16 bits. Q1 to Q7 were answered
(section 12). The plan is [creature_extensions_plan.md](creature_extensions_plan.md).
Nothing is built yet, except the `balance.py` fix from Q7 (section 10).

Builds on [creatures.md](creatures.md) (the roster of 21 random kinds and rooted
creatures), [ai.md](ai.md) and [ai_default_brain.md](ai_default_brain.md),
[spells.md](spells.md), [effects.md](effects.md) and
[effects_extensions.md](effects_extensions.md), [difficulty.md](difficulty.md)
(the pacing and power checks, `make balance`) and
[sprite_style.md](sprite_style.md) (the new shapes). The to-do items are topic
**M** in [todo.md](todo.md).

## 1. Goal

- More of the classic D&D and roguelike monsters that the roster lacks: kobolds,
  zombies, ghouls, wolves, hill giants, basilisks, liches and others.
- A challenge at every depth, above all depths 8 to 10, which have only 7, 5
  and 3 kinds today.
- Monsters that use the game's rules: effects, statuses, spells, ranged attacks,
  equipment, resistances. A new kind should not be just another set of bigger
  numbers.
- Monsters that work together. A goblin war band (fighters, an archer and a
  shaman) or a necromancer with its undead should be harder together than each
  one is alone.
- Know how many kinds the game can hold, and what limits that.

## 2. Terms

| Term | Meaning |
|---|---|
| Kind | One row of `gen_creatures[]`, indexed by a `CR_*` number |
| Shape | One drawing (2 rest patterns), drawn in any of the 16 palette banks |
| Stand-in | A kind drawn with another kind's shape in a different bank |
| Kit | Items a kind is generated with, already equipped (M1.1) |
| Natural ranged attack | A ranged attack that is part of the creature (spit, gaze, a thrown boulder), not a launcher it carries (M1.2) |
| Skirmisher | A monster that steps back from melee to shoot or cast (M1.3) |
| Trait | A bit in the kind's AI part that says how it likes to fight, as opposed to what it can do (M1.3) |
| Ability spell | A spell that only monsters know: a breath, a wail, a summons. Its MP cost works as a cooldown (D1) |
| Band | Several kinds placed together as one draw by the level generator (M3.1) |
| Family | A class of creature (undead, beast...) kept in the kind's flags (M4.3) |
| Mindless | A family trait: immune to sleep, confusion and fear (M4.3) |

## 3. Today: the roster and its gaps

There are 20 random kinds, plus 3 player kinds. Pool size by depth, from
`make balance`:

| Depth | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Kinds | 5 | 8 | 12 | 15 | 15 | 14 | 11 | 7 | 5 | 3 |

What the roster lacks:

- **Variety at depth:** depth 10 holds only the wraith, troll and dragon.
- **Ranged threats:** no monster spawns with a ranged attack. The AI can shoot
  (`opt_shoot()`), but only the dev kit's bandit is ever given a bow. The
  gold snake's "spits at range" is not built.
- **Groups:** every monster is placed alone at a random spot. The shaman is
  never near the goblins it could support.
- **Mechanics already built but used by no monster:** paralyse, fear,
  confuse, burning, bleeding, corrosion, ability drain (only the wraith
  uses Str), bane, hex, the area spell shapes (`SPELL_BALL`, `SPELL_BURST`),
  blink, summon, and monsters drinking potions.
- **Classic families with no members:** kobolds, wolves, zombies, ghouls and
  other undead (only the skeleton, ghost and wraith exist), giants, and
  dragon-kin.
- **Unbuilt "Later"s in creatures.md:** erratic flight, splitting, the
  shriek, stealing, webs, spitting, the gaze, hiding, passing through walls,
  and fire breath.

## 4. Limits: how many kinds fit

### 4.1 Hard limits

| Limit | Where | Allows |
|---|---|---|
| A kind is a `uint8_t` (`kind_index[]`), and `KIND_INVALID` is `0xFF` | game/components/kind.h, gen_kinds.h | 255 kinds, 0 to 254 |
| `GEN_PICK` counts with a `uint8_t` while `gen_k_ < count` | gen_internal.h | `CR_COUNT` at most 255: the same limit |
| `GEN_PICK` sums rarity into a `uint16_t` | gen_internal.h | 255 kinds × rarity 255 = 65,025: never overflows |
| `xp` is a `uint16_t`, `hp_max` a `uint8_t` | the kind record | 65,535 XP, 255 HP a kind: enough for a lich |
| The spell set is `SPELL_SET_BYTES` (4) bits | spells.h, compile check | 32 spells: 12 in use, 20 with this spec |
| `opt_cast()` picks a start spell with `rand() & 15` | ai_think.c | **Breaks above 16 spells.** It must change to `& 31` (R11) |
| The item kinds are `uint8_t` too | gen_kinds.h | 218 of 255 in use: **37 left**. No kit needs a new item |
| Statuses: 8 bits, all used | stats.h | No new status without E5.8 |
| Damage types: 8 bits, all used | health.h | No lightning, necrotic or holy without E5.7. Use `DMG_MAGIC` |
| AI capabilities (`ai_can_t`): 8 bits, all used | ai.h | Widened to 16 bits (D7): 2 used here, 2 kept for M5, 4 spare |
| Creature flags: 1 of 8 bits used | gen_internal.h | 5 used for families and mindless (M4.3), 2 spare |

### 4.2 Memory

| Store | Per kind | Today | With this spec |
|---|---|---|---|
| Page 107: the kind records and names | 59 bytes and the name (about 66) | $5F4, 1,524 of 8,192 bytes, 23 kinds | about 4.5K: 57 kinds × 68 bytes (kit 2, ranged 6, traits 1), the names and the band table |
| Page 127: the kinds' animation sequences (`anim_seqs[CR_COUNT]`) | 6 bytes | $90, 144 bytes | 348 bytes |
| Page 106: the creature generator's code | none | $62A | plus the kit and the natural ranged attack: about 0.3K |
| Page 102: the level generator | none | $1F51, 175 bytes free | **bands don't fit here**: their placing goes in page 103's code or a page of its own (plan, step 8) |
| Page 130: the AI's code | none | $181D | the stand-off and ally options: about 0.4K |
| Page 131: the AI's per-entity tables | none | $384 | `ai_traits[]`, 150 bytes |
| Page 135: spell definitions | none | $3CF | a `flags` byte a row, and 7 more rows (M2.1): about 0.1K |
| Page 121: effect definitions | none | $15DD | 3 more rows: `EF_ERRATIC`, `EF_SUMMON_UNDEAD`, `EF_WAKE` |
| Slot 0 and common memory | none | | nothing: tables are per kind, or per entity in banked pages |
| Sprite patterns (4-bit) | per shape, not per kind | 86 of 128 | 98: 6 new shapes (D9) |

At about 77 bytes a kind with its name, **page 107 holds about 105 kinds**.
That is the practical limit, well under the 255 allowed by `uint8_t`. Beyond
it, the names would move to their own page.

### 4.3 Per level

- **Entities:** `LEVEL_ENTITIES_MAX` budgets `LEVEL_MONSTERS_MAX` (10) monsters
  × (1 + `LEVEL_CREATURE_OWNED` 3), against `NGE_ENTITY_MAX` 150. A kit uses
  2 of a monster's 3 owned entities. An innate effect, or the mana
  regeneration that every caster has, uses the third. So an armed caster
  (the necromancer: a kit and mana) picks nothing up (R12). A band counts
  as its size against `LEVEL_MONSTERS_MAX` (R8), so the compile-time check
  still holds.
- **Summons:** `EF_SUMMON` makes at most 3 and is limited by the 15 entities
  reserved for play. A necromancer and a lich must not be able to empty the
  pool (R13).
- **Events while generating:** equipping a kit posts events, as generating a
  rooted creature does (creatures.md section 8: 2 each). The 16-event queue
  must not fill while a level is made (R15).

### 4.4 Recommendation

Aim for about **60 random kinds** (this spec brings the total to 54 random
and 57 in all). That leaves page 107 half free, and spare item numbers and
sprite patterns. The older kinds' depths are narrowed (6.3, Q5), so no depth
holds more than 22 kinds.

## 5. New mechanics

Grouped as to-do objectives. Each says what it costs and which kinds need it.
Phase 1 is data and small code changes. Phase 2 adds new rules. Phase 3 is
`(low)`.

### M1 Ranged and armed monsters (phase 1)

- **M1.1 Kit.** `uint8_t kit[2]` in the kind record: item kinds,
  stored as the item kind plus 1 (`GEN_KIT(IT_SHORT_BOW)`), with 0 for none:
  an entry a row leaves out is 0, and item kind 0 is the club. Each is made
  in the creature's inventory when it is generated, and equipped there
  with `act_equip(..., ACT_DO)`, as chargen equips the player's starting
  items. A sleeping archer then wakes with its bow in hand, and the player gets the kit as loot. Costs 2 bytes a kind.
  Used by: the kobold slinger, the archers, the hobgoblin, the orc warlord,
  the wight, the necromancer and the minotaur. The bandit keeps the dev kit's
  bow, which is placed beside it.
- **M1.2 Natural ranged attack.** An `attack_kind_t ranged` in the record
  (`dice` 0 for none). It gives the existing `ATTACK_RANGED` component, so
  `ai_can()` sets `AI_CAN_RANGED` and `opt_shoot()` uses it. `act_fire()`
  already falls back to the actor's own ranged attack when it has no launcher,
  and uses up no ammo then. The one change is in `act_strike()`: a creature's
  own ranged attack takes its hit effect from the **`GEN_HOOK_AIM`** slot,
  which creatures don't use (D3), not from its bite's `GEN_HOOK_HIT`. Costs
  6 bytes a kind (Q6). Used by: the gold snake's spit (blinds), the
  bombardier beetle's spray (corrodes), the basilisk's gaze (paralyses) and
  the hill giant's boulder.
- **M1.3 Skirmishers (D7).** A trait, `AI_TRAIT_SKIRMISH`, in a new
  `traits` byte of `ai_kind_t`. `ai_add()` copies it into `ai_traits[]`
  (page 131). `ai_can()` sets the new capability **`AI_CAN_SKIRMISH`**
  (`0x0100`) when the trait is set, the creature can move, and it can shoot
  or cast. In hunt, a new first option, `opt_stand_off()`, steps away from
  a target next to it (`opt_flee()`'s scoring), if the new tile still sees
  the target. Otherwise the turn goes on as now. An archer out of arrows
  loses the capability and closes to melee, as the capabilities are meant to
  work (ai.md D3). Used by: archers, slingers and casters (the shaman too).
- **M1.4 Erratic movers.** `EF_ERRATIC`: timed, lasts for ever, innate,
  grants `STATUS_CONFUSED`, so `act_move()` sometimes goes a random way. Data
  only. Used by: the bat ("erratic flight", creatures.md) and the
  will-o'-wisp.

### M2 Monster abilities as spells (phases 1 and 2)

- **M2.1 Ability spells (phase 1).** Spell rows that no book lists. A new
  `flags` byte in `spell_def_t` marks them `SPELL_F_MONSTER`, so
  `spell_learn()` refuses them for the player. Their level is 1 or more, so
  they cost MP: a level-0 spell (an ability, spells.h) costs nothing and
  would give no cooldown.

  | Spell | Shape | Effect | AI use | Kinds |
  |---|---|---|---|---|
  | `SP_FIRE_BREATH` | ball | `EF_BOLT_FIRE` | target | hell hound, dragon |
  | `SP_FROST_BREATH` | ball | `EF_BOLT_COLD` | target | frost drake |
  | `SP_CONFUSE` | direct | `EF_CONFUSE` | target | harpy (its song) |
  | `SP_WAIL` | burst | `EF_FEAR` | seen | banshee |
  | `SP_SHRIEK` | self | `EF_WAKE`: a new op, `OP_WAKE`, wakes every sleeping monster within power (8) tiles of the target, through a new `ai_alert()` that sends `AI_STIM_NOISE` | seen | shrieker |
  | `SP_SUMMON_UNDEAD` | self | `EF_SUMMON_UNDEAD`: as `EF_SUMMON`, undead kinds only (M4.3) | seen | necromancer, lich |
  | `SP_WAR_CRY` | burst, allies (M2.2) | `EF_BLESS` | seen | orc warlord |

  `opt_cast()` casts a target spell only when the cast reaches the creature
  it hunts (`a.target == target`). A self spell's target is the caster, and
  a burst has no single target, so neither would ever be cast. A new AI use,
  **`SPELL_AI_SEEN`**, casts on itself or around itself when it sees its
  target. A burst is cast only when the target is within
  `ACT_BURST_RADIUS`. A burst reaches 2 tiles, too few for a shriek that
  wakes a level, so the shriek is a self spell whose op has its own radius.

  Breath is a ball and catches the breather too, so breathers are immune to
  their element. Hell hounds then don't burn each other. With `SP_MEND`
  (M2.2), `SPELL_COUNT` goes from 12 to 20 (R11). `SP_SUMMON_UNDEAD` needs
  the undead family (M4.3), and `SP_WAR_CRY` ally targeting (M2.2), so both
  rows come in phase 2.
- **M2.2 Spells on allies (phase 2).** `SPELL_AI_ALLY`: cast on the most hurt
  monster in sight. Cure Wounds is a self spell, so the shaman also learns
  `SP_MEND`: monster-only, direct, `EF_HEALING`, `SPELL_AI_ALLY`. `ai_can()` sets a
  second new capability, **`AI_CAN_SUPPORT`** (`0x0200`), when the creature
  knows such a spell, and `opt_cast()` tries it before its target spells.
  A spell-definition flag, `SPELL_F_ALLIES`, makes area spells pass over
  creatures with no AI component, so a war cry never blesses the player.
  All monsters are allies of each other: the only other side is the player
  (D6).

### M3 Bands (phase 2)

- **M3.1 The band table**, in page 107 next to the kinds:
  `{ leader, member[3], min[3], max[3], where }`, about 13 bytes a band.
- **M3.2 Placing bands.** A monster draw picks a band 1 time in 4, where one
  is in the depth's band pool. The leader is placed on free floor and the
  members on free floor in the same room. The members' states follow the
  leader's: asleep together, or awake together. **Members are placed whatever
  their own depths:** a hobgoblin troop at depth 6 brings goblins, though
  goblins alone stop at depth 3.
- **M3.3 Counting.** A band uses as many of the level's monster draws as it
  has creatures, and is cut short when the draws run out. `balance.py`'s
  `per_level()` models bands (difficulty.md D10).

| Band | Depths | Rarity | Leader | Members | Why it is harder |
|---|---|---|---|---|---|
| Kobold pack | 1-3 | 6 | kobold | kobold 1-2, kobold slinger 0-1 | Numbers, and a stone from the back |
| Jackal pack | 1-3 | 5 | jackal | jackal 2 | Fast: you can't outrun them |
| Goblin war band | 2-5 | 5 | goblin | goblin 1-2, goblin archer 1, goblin shaman 0-1 (from depth 3) | Front line, arrows, then bolts, Enfeeble and healing |
| Hobgoblin troop | 4-6 | 3 | hobgoblin | goblin 2, goblin archer 1, goblin shaman 1 | As above, behind a shielded leader who drinks potions |
| Wolf pack | 3-5 | 5 | wolf | wolf 1-2 | Surrounds you |
| Bone patrol | 3-6 | 4 | skeleton | skeleton 1, skeleton archer 1 | Arrows from a patrol that never runs (mindless) |
| Ghoul pack | 3-5 | 3 | ghoul | zombie 1-2 | A paralysing touch, then the zombies hit a helpless target |
| Orc war party | 6-8 | 4 | orc warlord | orc 2, orc archer 1 | War cry: +1 to hit for the whole party |
| Warg pack | 6-8 | 4 | warg | warg 1-2 | Fast and hard-hitting |
| Necromancer's court | 7-10 | 3 | necromancer | zombie 1, skeleton 1, wight 0-1 | Holds you, and its losses come back as summons |
| Hell hound pack | 7-10 | 3 | hell hound | hell hound 1 | Two breaths. They don't burn each other |
| The lich's crypt | 10 | 1 | lich | mummy 1, wight 1 | The last fight before the quest's end |

### M4 Reactive monsters (phase 2)

- **M4.1 Death hook.** For creatures, the unused **`GEN_HOOK_USE`** slot means
  *on death*. On `EV_DIED`, its effect is applied as a burst around the corpse,
  to every creature next to it, monsters included. Used by: the gas spore
  (`EF_BOLT_FIRE`, power 8). Kill it from a distance, or lure monsters next to
  it.
- **M4.2 Struck hook.** The unused **`GEN_HOOK_WORN`** slot means *when hit in
  melee*. Its effect goes to the attacker, in `act_strike()` after the damage.
  Used by: the floating eye (`EF_PARALYSE`, the classic "don't hit it": this
  is its "paralysing gaze"), the salamander (`EF_BURNING`) and the black
  pudding (`EF_CORROSION`).
- **M4.3 Families and mindless.** Flag bits: `CREATURE_UNDEAD`,
  `CREATURE_BEAST`, `CREATURE_GIANT`, `CREATURE_DRAGON` and
  `CREATURE_MINDLESS`. The first four are I8.1's families: summon filters,
  bane weapons and a later "turn undead". Mindless is a monster-only part of
  E5.9: `effect_apply()` refuses `EF_SLEEP`, `EF_CONFUSE` and `EF_FEAR` on a
  mindless creature (Q4). It reads the flags through a banked accessor,
  `gen_creature_flags(kind)`, beside `gen_creature_hook()`. Watch the bank
  depth (effects_extensions.md 3.9). Used by: zombies, skeletons, mummies,
  the shadow, and the existing skeleton ("never flees").

### M5 Later (phase 3, low)

- **M5.1 Life drain** (= E5.5): melee damage heals the attacker. Used by: the
  vampire. It comes without this until then.
- **M5.2 Passing through walls:** `NGE_BLOCK_SOLID` in the ghost's `pass`
  part. That is data, but closed doors are solid too, and sight, pathing and
  drawing over walls need a spec. Capability `AI_CAN_PHASE` (`0x0800`) is
  kept for it. Used by: the ghost, the wraith and the banshee.
- **M5.3 Splitting:** hit by slashing or blunt damage while it has 10 HP or
  more, it makes a copy with half its HP. Needs an entity, within the
  reserve (R13). Used by: the green slime and the black pudding.
- **M5.4 Steal and teleport:** [action_steal.md](action_steal.md), then
  `EF_TELEPORT` on itself. Capability `AI_CAN_STEAL` (`0x0400`) is kept for
  it. Used by: the bandit.
- **M5.5 Disguise** (= K4.2): drawn as an item or a feature until touched.
  Used by: the mimic, and a later gargoyle.
- **M5.6 Webs:** the giant spider places `FT_WEB` near itself. That needs a
  feature-making effect.

## 6. The new kinds

Numbers follow creatures.md: about 4.5 HP a hit die, to-hit about the hit dice,
XP from the B/X table with its bonus for a special ability. As before, none
of it has been balanced by play. "Ph" is the phase in which a kind works fully.
A phase-2 kind goes in during phase 1 without its phase-2 part, except where
the plan says otherwise.

### 6.1 Depths 1 to 6

| Depth | Rarity | Kind | Shape / bank | HD | HP | AC | Melee | What makes it work | XP | Ph |
|---|---|---|---|---|---|---|---|---|---|---|
| 1-3 | 8 | kobold | kobold (new) / red | <1 | 3 | 12 | d4 pierce | Packs (band). Weak alone, dangerous in threes | 5 | 1 |
| 1-3 | 5 | kobold slinger | kobold / earth | <1 | 3 | 12 | d2 blunt | Kit: sling, stones. Skirmisher | 6 | 1 |
| 1-3 | 6 | jackal | dog (new) / gold | <1 | 3 | 12 | d2 pierce | Beast. Fast (`MONSTER_FAST`). Packs | 5 | 1 |
| 1-3 | 5 | stirge | bat / red | 1 | 4 | 13 | d3 pierce | Flies. Bite sets `EF_BLEEDING` 1 | 13 | 1 |
| 1-3 | 5 | fire beetle | bug (new) / orange | 1 | 5 | 14 | d4+1 slash | Beast. A hard shell (AC 14). Immune to fire | 10 | 1 |
| 2-4 | 5 | goblin archer | goblin / orange | 1-1 | 5 | 13 | d3 slash | Kit: short bow, arrows. Skirmisher. War band | 10 | 1 |
| 2-4 | 6 | zombie | zombie (new) / green | 2 | 11 | 8 | d8 blunt | Slow, undead, mindless, immune to poison | 20 | 1 |
| 2-5 | 3 | gas spore | eye / iron | 1 | 4 | 10 | none | Slow, flies. Death: a fire burst (power 8) | 25 | 2 |
| 3-5 | 5 | ghoul | zombie / iron | 2 | 11 | 13 | d3 slash | Undead. Claws set `EF_PARALYSE` 1 | 25 | 1 |
| 3-5 | 6 | wolf | dog / iron | 2 | 11 | 13 | d6 pierce | Beast. Packs | 20 | 1 |
| 3-5 | 4 | bombardier beetle | bug / gold | 2 | 9 | 15 | d4 slash | Beast. Spray: range 3, d4 acid and `EF_CORROSION` 1. Immune to acid | 25 | 1 |
| 4-6 | 3 | hobgoblin | goblin / red | 2 | 11 | 11 | d6 slash | Kit: short sword, shield. Fighter's slots. Drinks potions. Leads a troop | 25 | 1 |
| 4-6 | 4 | harpy | bat / flesh | 3 | 13 | 12 | d4 slash | Flies. Song: `SP_CONFUSE` (6 MP) | 50 | 1 |
| 4-6 | 4 | shadow | ghost / shadow | 3 | 13 | 13 | d4 cold | Undead, mindless, resists weapons. Touch sets `EF_DRAIN_STR` 1 | 50 | 1 |
| 4-6 | 5 | orc archer | goblin / earth | 2 | 9 | 13 | d4 slash | Kit: crossbow, bolts. Skirmisher. War party | 25 | 1 |
| 4-6 | 4 | skeleton archer | skeleton / iron | 2 | 9 | 13 | d4 slash | Kit: short bow, arrows. Skirmisher. Undead, mindless. Weak to blunt | 25 | 1 |

### 6.2 Depths 5 to 10

| Depth | Rarity | Kind | Shape / bank | HD | HP | AC | Melee | What makes it work | XP | Ph |
|---|---|---|---|---|---|---|---|---|---|---|
| 5-7 | 5 | owlbear | ogre / flesh | 5 | 23 | 13 | 2d6 slash | Beast. A hug: `EF_HOLD` 1 | 225 | 1 |
| 5-8 | 3 | will-o'-wisp | fx orb / blue | 5 | 22 | 19 | 2d4 magic | Fast, flies, erratic. `SP_MAGIC_BOLT`, `SP_BLINK` (8 MP). Hard to hit | 225 | 1 |
| 6-8 | 5 | warg | dog / dark | 4 | 18 | 14 | 2d4+1 pierce | Speed 8. Beast. Packs | 75 | 1 |
| 6-8 | 2 | orc warlord | goblin / dark | 5 | 23 | 12 | d8 slash | Kit: battle axe, chain mail. `SP_WAR_CRY` (6 MP). Leads a war party | 225 | 2 |
| 6-8 | 4 | wight | zombie / dark | 4 | 18 | 14 | d6 cold | Kit: sword. Undead, resists weapons. Touch sets `EF_DRAIN_CON` 1 | 125 | 1 |
| 6-8 | 3 | basilisk | lizard (new) / green | 6 | 27 | 15 | d10 pierce | Beast, slow. Gaze: range 4, 1 magic and `EF_PARALYSE` 1 | 500 | 1 |
| 6-9 | 4 | wyvern | dragon / green | 6 | 27 | 14 | d6 pierce | Dragon, flies. Sting sets `EF_POISON` 3 | 500 | 1 |
| 7-10 | 3 | necromancer | robe (new) / dark | 6 | 22 | 11 | d6 blunt | Kit: quarterstaff, robe. `SP_FROST_BOLT`, `SP_HOLD`, `SP_SUMMON_UNDEAD`, `SP_BLINK` (14 MP). Skirmisher. Drinks potions | 500 | 2 |
| 7-10 | 4 | mummy | zombie / bone | 7 | 32 | 14 | 2d6 blunt | Slow, undead, mindless. Resists blunt and pierce, weak to fire. Touch sets `EF_DRAIN_CON` 2 | 850 | 1 |
| 7-10 | 5 | hill giant | ogre / iron | 8 | 36 | 13 | 2d8+2 blunt | Giant. Boulder: range 5, 2d6 blunt. Wields | 650 | 1 |
| 7-10 | 4 | hell hound | dog / orange | 6 | 27 | 15 | d6+1 pierce | Beast. Immune to fire, weak to cold. `SP_FIRE_BREATH` (6 MP). Packs | 500 | 1 |
| 8-10 | 4 | salamander | lizard / orange | 7 | 32 | 15 | d8 pierce | Immune to fire, weak to cold. Hit sets `EF_BURNING` 2. Struck: `EF_BURNING` 1 | 850 | 2 |
| 8-10 | 3 | banshee | ghost / ice | 7 | 32 | 15 | d6 cold | Undead, floats, resists weapons. `SP_WAIL` (8 MP): a fear burst | 850 | 1 |
| 8-10 | 4 | minotaur | ogre / red | 9 | 40 | 14 | 2d8+2 pierce | Giant. Speed 8. Kit: battle axe | 900 | 1 |
| 8-10 | 3 | black pudding | slime / dark | 10 | 45 | 6 | 2d8 acid | Slow, mindless. Immune to acid, cold and poison. Hit sets `EF_CORROSION` 2. Struck: `EF_CORROSION` 1. Later: splits | 1750 | 2 |
| 9-10 | 3 | frost drake | dragon / ice | 9 | 40 | 16 | 2d6 pierce | Dragon, flies. Immune to cold, weak to fire. `SP_FROST_BREATH` (10 MP) | 1600 | 1 |
| 9-10 | 2 | vampire | robe / red | 9 | 41 | 16 | d8 pierce | Undead. Regenerates (`EF_REGEN` 2), resists weapons, weak to fire. `SP_HOLD`, `SP_BLINK` (8 MP). Later: life drain | 1600 | 1 |
| 10 | 1 | lich | robe / bone | 11 | 50 | 17 | d10 cold | Undead. Touch sets `EF_PARALYSE` 1. `SP_POWER_BOLT`, `SP_FROST_BOLT`, `SP_HOLD`, `SP_SUMMON_UNDEAD` (30 MP). Skirmisher | 2300 | 2 |

### 6.3 Changes to existing kinds

The depths of the older kinds narrow (Q5), so each depth keeps its character
and no depth holds more than 22 kinds:

| Kind | Depths now | Depths after |
|---|---|---|
| rat | 1-4 | 1-2 |
| goblin | 1-6 | 1-3 |
| bat | 1-4 | 1-3 |
| green snake, lichen | 1-5 | 1-3 |
| green slime | 2-6 | 2-4 |
| shrieker | 2-6 | 2-5 |
| bandit | 2-6 | 2-4 |
| orc, giant spider, skeleton, goblin shaman | 3-7 | 3-5 |
| gold snake, floating eye | 4-8 | 4-6 |
| mimic | 4-9 | 4-7 |
| ogre | 5-8 | 5-7 |
| ghost | 5-9 | 5-7 |
| wraith | 6-10 | 6-8 |
| troll, dragon | 7-10, 9-10 | unchanged |

Other changes:

| Kind | Change | Mechanic | Ph |
|---|---|---|---|
| bat | Innate `EF_ERRATIC`: "erratic flight" | M1.4 | 1 |
| gold snake | Spit: range 3, d3 poison, `EF_BLIND` 1. Its blinding moves from the bite to the spit | M1.2 | 1 |
| goblin shaman | Skirmisher. Cure Wounds on hurt allies | M1.3, M2.2 | 1, 2 |
| dragon | 12 MP, `SP_FIRE_BREATH`: "fire breath" | M2.1 | 1 |
| shrieker | 4 MP, `SP_SHRIEK`: "a shriek that wakes the level" | M2.1 | 1 |
| floating eye | Struck: `EF_PARALYSE` 2: "a paralysing gaze" | M4.2 | 2 |
| skeleton, ghost, wraith | Undead. The skeleton is mindless: "never flees" | M4.3 | 2 |
| rat | Beast | M4.3 | 2 |
| ogre, troll | Giant | M4.3 | 2 |
| dragon | Dragon | M4.3 | 2 |
| green slime, mimic, bandit, giant spider, ghost | Their "Later"s: M5.3, M5.5, M5.4, M5.6, M5.2 | M5 | 3 |

### 6.4 Coverage after

| Depth | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Kinds today | 5 | 8 | 12 | 15 | 15 | 14 | 11 | 7 | 5 | 3 |
| Kinds after | 10 | 16 | 22 | 21 | 21 | 18 | 16 | 16 | 13 | 13 |
| Mean XP a monster today | 8 | 11 | 17 | 24 | 56 | 92 | 195 | 314 | 600 | 854 |
| Mean XP a monster after | 8 | 11 | 16 | 31 | 71 | 176 | 395 | 614 | 927 | 997 |

The bands are not counted here. Narrowing the older kinds takes the weak ones
out of the deep pools, so the mean XP at depths 6 to 8 roughly doubles
(section 10).

### 6.5 Shapes

There are 6 new shapes, 12 patterns, which Claude draws by
[sprite_style.md](sprite_style.md) section 7 (grids, a preview next to the
masters, then the file). Each is rest art only (`<shape>.idle`, 2 frames), as
the other monsters are:

| Shape | Size class (sprite_style.md 6.2) | Default bank | Master to match | Kinds |
|---|---|---|---|---|
| kobold | small humanoid, about 9 x 10 | red | `player.idle`, smaller | kobold, kobold slinger |
| dog | small, about 12 x 9 | iron | `rat.idle` | jackal, wolf, warg, hell hound |
| bug | small, about 12 x 8 | orange | `rat.idle` | fire beetle, bombardier beetle |
| zombie | medium | green | `player.idle` | zombie, ghoul, wight, mummy |
| robe | medium | dark | `player.idle` | necromancer, vampire, lich |
| lizard | small, about 12 x 9 | green | `rat.idle` | basilisk, salamander |

The will-o'-wisp uses the `fx.orb` pattern (one frame, still). Every other
kind is a stand-in in a bank that no other kind of its shape uses. Monster
patterns rise from 27 to 39, within sprite_style.md 6.8's share of 50, and
98 of the 128 patterns are used. The goblin shape carries 7 kinds and the ogre
shape 5. With the narrower depths, no more than 4 of a shape share a depth.

## 7. Requirements

- **R1** Every depth has at least 6 random kinds (difficulty.md D7 asks for 3),
  and no more than 22.
- **R2** From depth 2, every depth's pool holds at least one kind with a ranged
  attack or offensive spells.
- **R3** Every new kind uses at least one rule beyond a plain melee attack: an
  effect, a resistance pattern, a spell, a ranged attack, equipment, a family
  trait or a band role.
- **R4** The numbers follow creatures.md's rule (HP, to-hit and XP by hit
  dice), so `make balance` measures them like the rest.
- **R5 (Changed)** No new status bit, damage type, or table in slot 0 or common
  memory. `ai_can_t` widens to 16 bits (D7). Reason: the review of D7.
- **R6** A kit item fits a slot the kind has, or it is ammo for its launcher.
  A kind with a kit has an inventory large enough for it, and its kit is
  equipped when it is generated.
- **R7** A natural ranged attack has `range` 2 or more. It uses no ammo, and the
  AI holds its fire when another creature is in the line (`opt_shoot()` as now).
- **R8** A band counts as its size against the level's monster draws, so
  `LEVEL_ENTITIES_MAX` and its compile-time check hold.
- **R9** Area spells marked `SPELL_F_ALLIES` never affect the player. Breathers are
  immune to their breath's damage type.
- **R10** An ability spell is not in any book, and the player can't learn it.
- **R11** `opt_cast()`'s start-spell pick works for any `SPELL_COUNT` up to 32.
- **R12** An armed monster's kit, innate effects and mana regeneration fit in
  `LEVEL_CREATURE_OWNED`. A kind that would need more raises the constant,
  and the entity budget is checked again.
- **R13** Summons stop when fewer than 5 of the 15 reserved entities are free.
- **R14** The spec that builds each phase says what `make balance` showed
  before and after (difficulty.md R13).
- **R15 (New)** Generating a level never fills the event queue. Reason:
  equipping a kit posts events, as rooted creatures do.
- **R16 (New)** A skirmisher steps back only to a tile from which it still sees
  its target, and never while rooted. Reason: the review of D7.

## 8. Decisions

- **D1 Abilities are spells (agreed).** Breath, wail, song, summons and shriek
  are spell rows with monster-only marks. The AI already casts
  (`opt_cast()`), MP and `EF_MANA_REGEN` already act as a cooldown, and the
  spell shapes (ball, burst) already do the area work. **Rejected:** an
  ability field per kind with a cooldown timer: it needs a new record part,
  a new timer per monster, and a new AI option for what casting already does.
- **D2 A two-item kit, not a loot table (agreed).** Two item kinds (2 bytes)
  cover a launcher and its ammo, or a weapon and its armour. It is equipped
  when generated, by the same action call chargen uses. The player finds it
  on the body. **Rejected:** loot tables by depth: more data for little gain
  now. **Rejected:** placing items next to the monster as the dev kit does:
  the player could pick them up first.
- **D3 Reuse the creature's empty hook slots (agreed).** `gen_hook_t hooks[5]`
  uses 2 of 5 slots for creatures. `USE` becomes *on death*, `WORN` *when
  struck*, and `AIM` the natural ranged attack's hit. That costs no bytes,
  and the per-class reader (`gen_creature_hook()`) is the only place that
  names them. **Rejected:** new hook numbers: `GEN_HOOK_COUNT` would grow for
  every class.
- **D4 The natural ranged attack is a full `attack_kind_t` (agreed, Q6).** 6
  bytes, the type that the component and `act_fire()` already take.
  **Rejected:** packing it into 4 bytes: it saves 114 bytes over 57 kinds,
  but costs unpacking code.
- **D5 Bands in their own table, placed as one draw (agreed).** A kind doesn't
  know its band, so the goblin can be both alone and in a band.
  **Rejected:** a "companions" field per kind: 3 or more bytes on every kind
  for a dozen bands.
- **D6 Monsters are all allies (agreed).** Only the player is on the other
  side. Ally spells pass over entities with no AI component. **Rejected:**
  factions: no kind needs to fight another kind yet.
- **D7 (revised) Widen `ai_can_t` to 16 bits, and add how-it-fights traits.**
  The first proposal was a second brain, `AI_BRAIN_RANGED`. But a brain is only
  a transition table (`ai_brains.c`), and stepping back is an option inside
  the hunt state, so a brain could do it only as `if (brain == ...)` in
  `think_hunt()`, which ai.md section 2 rules out. Widening is cheap:
  `ai_can_t` appears 7 times (the type, `ai_can()`, the context's field, two
  locals) plus the tests' exact masks, and `ai_can()` then returns in HL
  rather than L. New bits are hex constants (`0x0100`), so the zsdcc
  signed-shift trap doesn't apply. The bits: `AI_CAN_SKIRMISH` (`0x0100`)
  and `AI_CAN_SUPPORT` (`0x0200`) now, `AI_CAN_STEAL` (`0x0400`) and
  `AI_CAN_PHASE` (`0x0800`) kept for M5.4 and M5.2, 4 spare. A capability
  says what a creature can do now, but stepping back is also a choice that
  some kinds make and others don't. So a trait byte in `ai_kind_t`, copied
  to `ai_traits[]` in page 131, says which kinds want to stand off.
  `ai_can()` combines it with what the creature can do now.
  **Rejected:** `AI_BRAIN_RANGED`, as above. **Rejected:** deriving it from
  `AI_CAN_RANGED` alone: the hill giant and the gold snake would back away
  from melee. **Rejected:** reading the trait from the creature's flags each
  turn: a banked call into page 107 for every monster turn.
- **D8 Families and mindless in the flags byte (agreed).** 7 bits are free,
  and they are read rarely (on summoning, and when applying an effect), so a
  banked accessor is enough. **Rejected:** a per-entity copy in slot 0:
  150 bytes for a rare read.
- **D9 Stand-ins first, 6 new shapes (agreed).** 42 patterns are free, and
  attack and dead frames are still wanted for every shape (V1.5). New shapes
  are for body plans with no match. **Rejected:** a shape per kind: 34 shapes
  would need 68 patterns, and there are 42.
- **D10 Classic, generic monsters only (agreed).** Names and ideas from the
  open D&D reference and common roguelike folklore (kobolds, wights,
  basilisks). No trademarked creature from any game, and no copied art.

## 9. Scope and phases

- **Phase 1** (M1, M2.1, R11): the kit, natural ranged attacks,
  skirmishers with the wider `ai_can_t`, erratic movers and ability spells.
  With them, the 28 phase-1 kinds in section 6, the narrower depths and the
  phase-1 changes to existing kinds. **To-do M1, M2.1, M6.1, M6.3.**
- **Phase 2** (M2.2, M3, M4): ally spells, bands, the death and struck
  hooks, families and mindless. With them, the gas spore, orc warlord,
  necromancer, salamander, black pudding, lich and the floating eye's
  gaze. **To-do M2.2, M3, M4, M6.2.**
- **Phase 3** (M5, low): life drain, passing walls, splitting, stealing,
  disguise and webs. Each needs its own spec.
- **Not in scope:** rebalancing XP pacing (section 10), new statuses or
  damage types (E5.7, E5.8), and the shapes' attack and dead frames.

Kinds are added at the end of `gen_kinds.h` before `CR_COUNT`. The player
kinds (`CR_FIGHTER`, `CR_MAGE`) keep their numbers, because nothing may
renumber.

## 10. Balance (difficulty.md R13)

**Fixed first (Q7, 2026-11-13).** `balance.py` read monster damage as 0.0
because the attack part's `.bonus` became `.to_damage`. The pattern matched
nothing and the script fell back to 0, against difficulty.md R12. It now
reads `.to_damage`, and stops with an error if a kind has a `.melee` part
it can't read.

**Before** (`make balance`, 2026-11-13, 6 to 10 monsters a level):

- C1 pacing **fails** at depths 2 to 8, where the player arrives 1 to 2
  levels ahead. The cap is reached at depth 7.
- C5 power: 4.4 hits to kill the player at depth 1 (warns), 5.8 to 8.9
  elsewhere (ok).
- C3 variety: ok.

**Expected after:** the mean XP per monster roughly doubles at depths 6 to
8 (6.4), so C1 gets worse. Bands don't change XP per draw. XP pacing is
already off before this spec, so retuning it (the monster count, the level
table or XP values) is a separate task, and this spec keeps the B/X XP. C5
should stay in its band, since the deep kinds' damage follows their hit
dice. Each step that adds kinds reports the before and after numbers (R14).

## 11. Tests and checks

PC tests:

- **`test_gen`:** every kit item exists and fits a slot the kind has, or is
  ammo for its launcher (R6). A generated archer has its bow and arrows
  equipped. A natural ranged attack has range 2 or more and gives
  `ATTACK_RANGED`. A creature's `GEN_HOOK_AIM` names an effect only when the
  kind has a ranged attack. `GEN_HOOK_USE` and `GEN_HOOK_WORN` on creatures
  name instant or timed effects. Bands: sizes fit in `LEVEL_MONSTERS_MAX`.
  Every depth has 6 to 22 kinds (R1).
- **`test_spell`:** ability spells are in no book, and `spell_learn()` refuses
  them (R10). The start pick reaches every spell with 20 (R11).
- **`test_ai`:** `ai_can()`'s masks are 16-bit. A skirmisher next to the
  player steps back, then shoots. One with no tile from which it still sees
  the target, or rooted, attacks. An archer with no ammo loses
  `AI_CAN_SKIRMISH`. A shaman heals a hurt goblin in sight. War cry blesses
  monsters and not the player.
- **`test_effects`:** `EF_ERRATIC` is innate. Mindless creatures refuse sleep,
  confusion and fear. `EF_WAKE` wakes sleepers in range only.
  `EF_SUMMON_UNDEAD` makes only undead kinds, and stops at R13's reserve.
- **`test_act`:** firing a natural ranged attack uses no ammo, and applies the
  `GEN_HOOK_AIM` effect, not the bite's. A struck hook reaches the attacker,
  a death burst reaches neighbours.
- **`test_level`:** bands are placed in one room. A level with bands stays
  within the entity budget, and doesn't fill the event queue, at every
  depth over many seeds (R15).

CSpect checks (the user runs them):

1. A goblin archer steps back from the player, then shoots. The shaman does
   the same and casts.
2. A hell hound breathes: the player and its packmate are caught, and the
   packmate takes no damage.
3. Hitting a floating eye in melee paralyses the player. Hitting a gas spore
   from range burns the monsters next to it.
4. A shrieker's shriek wakes a sleeping band in the next room.
5. Depth 10 with the dev seed: about 13 kinds turn up over several levels,
   and the frame time with a full band holds.

## 12. Questions (answered 2026-11-13)

- **Q1** The roster: all 34 kinds.
- **Q2** Bands don't come before phase 1's kinds.
- **Q3** Claude draws the 6 new shapes, following sprite_style.md (6.5).
- **Q4** Mindless: immune to sleep, confusion and fear, as proposed. Hold and
  paralysis still work.
- **Q5** Yes: the older kinds' depths narrow (6.3).
- **Q6** The natural ranged attack takes 6 bytes (D4).
- **Q7** Yes: `balance.py`'s melee damage is fixed (section 10).
