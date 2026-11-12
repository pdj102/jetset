# To do

Aid-memoire for features still to build. Points are short on purpose: the
detail goes in a spec when an item is picked up.

## How it is organised

| Level | Number | What it is |
|---|---|---|
| Topic | `U` | An area of the game (letter code) |
| Objective | `U1` | Something the player can do or see when it's done |
| Feature | `U1.2` | One piece of an objective, small enough to spec and build on its own |

- Numbers never change or get reused, so specs and commits can name them.
- Status after each line: blank = idea, `[spec]` spec written,
  `[plan]` plan written, `[wip]` building, `[done]` built (date).
- Link the spec beside the status once it exists, e.g. `[spec](help_screen.md)`.
- `(low)` after a line marks a lower priority: it needs a significant change
  (a wider mask, a new system) for what it adds.
- To pick one up, ask: *"Write a spec for U1"* (whole objective) or
  *"Write a spec and plan for U1.3"* (one feature).

Topic codes: `C` Character creation, `E` Effects, `G` Game rules, `I` Items, `U` UI, `V` Visual effects.

---

## C — Character creation

### C1 Create a new character at game start `[done]` (2026-11-08) [spec](character_creation.md), [plan](character_creation_plan.md)

- C1.1 Select Fighter, Mage (or Dev) class `[done]`
- C1.2 Starter item kit per class `[done]`
- C1.3 Additional classes
- C1.4 Randomised stats

## D - Difficulty scaling
D1 - Create a roadmap guide for game mechanic scaling `[spec]` [spec](difficulty.md)
- D1.1 Define key parameters that are key inputs to  scaling game mechanics e.g. maximum dungeon depth, maximum character level
- D1.2 Define key principles e.g. player character should on average gain 1 level per dungeon depth fully cleared - when going down

## E — Effects

Effects missing against classic D&D and roguelikes, from a review of
`effects_pdata.c` ([effects.md](effects.md), [attributes.md](attributes.md)).

### E1 Teleport to another level `[done]` (2026-11-11) [spec](effects_extensions.md), [plan](effects_extensions_plan.md)

- E1.1 `OP_LEVEL_TELEPORT`, `arg` up, down or random: the player arrives
  anywhere on the new level. It asks for a level change rather than making
  it there and then (an effect can run inside an event handler), the way the
  trapdoor (`FCAN_DESCEND`) does `[done]`
- E1.2 Limits: going up stops at depth 1, so it never ends the game
  (level_transitions.md R2); going down stops at the bottom depth. Going
  down while already at the bottom depth, or up while already at depth 1,
  fizzles with a message `[done]`
- E1.3 On a monster: it leaves the level for good (levels aren't kept).
  Persistent entities (the boss, a quest object holder) are immune `[done]`
- E1.4 Scroll of teleport level, and a level teleport trap beside the
  teleport trap `[done]`
- E1.5 Delayed descent: a timed effect with one firing, whose `on_end` is a
  level teleport down by one level times the effect's power (the first
  effect to use `on_end`) `[done]`
- E1.6 Delayed ascent: a timed effect with one firing, whose `on_end` is a
  level teleport up by one level times the effect's power. A recall up to
  depth 1 is one with a power high enough to reach the limit going up `[done]`

### E2 Build the pending ops [spec](effects_extensions.md), [plan](effects_extensions_plan.md) (E2.1, E2.6 done; the rest later)

Defined and used by items and traps, but they do nothing yet
(effects.md, "Pending ops").

- E2.1 Teleport (within the level; goes with E1) `[done]`
- E2.2 Light
- E2.3 Map
- E2.4 Identify (see S10)
- E2.5 Enchant
- E2.6 Summon `[done]`

### E3 Fill the gaps in the effect table (data only) `[done]` (2026-11-11) [spec](effects_extensions.md), [plan](effects_extensions_plan.md)

New rows in `effects_pdata.c` using attributes and ops that already work.

- E3.1 Timed Dex, Con, Int, Wis buffs (Grace, Endurance, Insight, …): Might
  is the only one `[done]`
- E3.2 `EF_WORN_CHA`: Str to Wis have worn versions, Cha doesn't `[done]`
- E3.3 Timed poison, acid and magic wards: only fire and cold have them `[done]`
- E3.4 Timed max HP (Aid, Heroism): `ATTR_HP_MAX` modifier `[done]`
- E3.5 Shield: timed AC without Stoneskin's speed penalty `[done]`
- E3.6 Vulnerability hex (`ATTR_VULN`) and worn immunity (`ATTR_IMMUNE`):
  both attributes exist, nothing grants them `[done]`
- E3.7 Ability drain: a long timed ability penalty, from a monster's hit
  or a bad potion (a permanent drain with `OP_RAISE_BASE` -1 would need a
  record of what was lost before a Restore could undo it) `[done]`
- E3.8 Damage over time beyond poison: burning (fire, Dex save), bleeding
  (slash), corrosion (acid) `[done]`
- E3.9 Cure blindness: `OP_CURE` with `STATUS_BLINDED` `[done]`
- E3.10 Tidy names: `EF_STRENGTH` beside `EF_RAISE_DEX` and the other
  `EF_RAISE_*` `[done]`

### E4 New statuses (4 of the 8 bits free) [spec](effects_extensions.md), [plan](effects_extensions_plan.md) (E4.1 to E4.6 done)

- E4.1 Confused: `act_move()` sometimes goes a random direction `[done]`
- E4.2 Frightened: no melee, the AI flees; a use for Cha (G1) `[done]`
- E4.3 Paralysed: like asleep, but damage doesn't end it (needs E4.5) `[done]`
- E4.4 Flying (levitation): potion or boots; fliers don't set off traps
  (features.md R15) `[done]`
- E4.5 Asleep wakes when damaged (attributes.md, "Not built yet") `[done]`
- E4.6 Unconscious and blinded read by the rules: an unconscious creature
  loses its turns; the main screen shows every condition, not only poisoned `[done]`
- E4.7 Silenced: can't cast spells. Uses the last free bit if E4.1 to E4.4
  are all built (low)
- E4.8 Invisible and see invisible: AI, targeting and drawing all change (low)

### E5 New ops and attributes [spec](effects_extensions.md), [plan](effects_extensions_plan.md) (E5.1 to E5.4 done)

- E5.1 `ATTR_TO_HIT`: Bless and Bane, Heroism's bonus, and the blinded
  penalty as a modifier rather than a special case in
  `attack_actor_to_hit()` `[done]`
- E5.2 `OP_DETECT`: detect monsters, detect traps, beside `OP_MAP` `[done]` for traps; monsters later (needs fog of war)
- E5.3 Blink: a short teleport the caster aims, within line of sight `[done]`
- E5.4 Mana drain: `mana_restore()` takes a `uint8_t`, so it needs a drain
  path or a signed amount `[done]`
- E5.5 Life drain: damage that heals the caster; ops only know the target,
  so the source has to be passed in (low)
- E5.6 Life saving: a worn reaction to lethal damage, a hook in
  `health_damage()` (low)
- E5.7 Lightning, necrotic and radiant damage: all 8 damage-type bits are
  used, so the immune, resist and vulnerable masks become `uint16_t` (low)
- E5.8 More than 8 statuses: the status mask and the definitions' `status`
  field become `uint16_t` (low)
- E5.9 Condition immunities (free action, poison immunity): a status mask
  attribute that blocks conditions (low)
## G — Game rules

### G1 Abilities in attacks [done] 

- Str to melee hit and damage, Dex to ranged hit, and a use for Cha. The
  help screen's Abilities page (`screen_guide_pdata.c`) changes with it

### G2 Hunger

Food heals today (`EF_NOURISH`) and nothing makes the player eat. A hunger
clock gives food its purpose and a cost to farming new levels on the stairs
(level_transitions.md D7).

- G2.1 A satiety count in the player character, lowered every few turns
  (a timer, as regeneration is)
- G2.2 Stages: fed, hungry, weak (an ability penalty), fainting (turns
  lost), starving (damage). Shown on the main screen; a message as each
  stage starts. A pc variable, not a status: the status bits are nearly
  full (E5.8)
- G2.3 Eating restores satiety by the food's power; whether food still
  heals is for the spec
- G2.4 Food rarity and depth bands checked against the clock, so a careful
  player doesn't starve (D1 difficulty)

## H — 
## I — Items

From a review (2026-11-12) of the 166 kinds in `gen_item_pdata.c` against
`effects_pdata.c`. The item table has about 1.8K left in its page
(memory_map.md: 6.2K of 8K), room for about 50 more kinds. New kinds reuse
an existing sprite with another palette bank where they can.

Effects no item uses but that belong elsewhere: the drains (`EF_DRAIN_*`)
suit monster hit hooks (a shadow, a wraith); `EF_REGEN`, `EF_ROOT`,
`EF_MANA_REGEN` and `EF_HOLD` are innate or monster effects already.

### I1 Items for the unused effects (data only) `[done]` (2026-11-12) [spec](items_extensions.md)

New rows using effects that work today; no new code. Before I3, every item
can only aim at a creature.

- I1.1 Potions of the timed ability buffs: grace (`EF_GRACE`), endurance
  (`EF_ENDURANCE`), insight (`EF_INSIGHT`), intuition (`EF_INTUITION`).
  Names must not clash with the permanent ones (potion of vigour, wisdom, …)
- I1.2 Ward potions for poison, acid and magic (`EF_RESIST_POISON`,
  `_ACID`, `_MAGIC`): only fire and cold have them
- I1.3 Potions of heroism (`EF_HEROISM`) and aid (`EF_AID`); eye drops
  (`EF_CURE_BLIND`)
- I1.4 Bad potions: confusion (`EF_CONFUSE`), dread (`EF_FEAR`), paralysis
  (`EF_PARALYSE`), beside poison, sleep and blindness. Not waiting for S10
  (answered 2026-11-12)
- I1.5 Parchments read on oneself: detect traps (`EF_DETECT_TRAPS`),
  shield (`EF_SHIELD`), blessing (`EF_BLESS`), deep descent
  (`EF_DESCENT`), recall (`EF_RECALL`, power enough to reach depth 1)
- I1.6 Aimed parchments and wands: terror (`EF_FEAR`), confusion
  (`EF_CONFUSE`), hold monster (`EF_PARALYSE`), hex (`EF_HEX`), bane
  (`EF_BANE`); a wand of acid (`EF_BOLT_ACID`: only a parchment today) and
  a wand of weakness (`EF_WEAKNESS`)
- I1.7 Weapons with a hit hook: serrated sword or barbed spear
  (`EF_BLEEDING`), acid blade (`EF_CORROSION`), fire brand (`EF_BURNING`).
  A +1 weapon with one of these is already possible: its bonus is in its
  attack (`to_hit`, `to_damage`), the effect in its hook
- I1.11 The flask of oil sets the creature hit burning (`EF_BURNING`)
  instead of one fire bolt, at the same damage a turn (power 4) (answered
  2026-11-12)
- I1.8 Ammo with a hit hook: barbed arrows (`EF_BLEEDING`), fire arrows
  (`EF_BURNING`), poisoned darts (`EF_POISON`), sleep darts (`EF_SLEEP`),
  flask of acid (`EF_CORROSION`)
- I1.9 Worn: ring of presence (`EF_WORN_CHA`); rare deep amulets of fire,
  cold and poison immunity (`EF_WORN_IMMUNE_*`)
- I1.10 Food with effects: mushrooms, one kind per effect (healing,
  confusion, poison); elven waybread (`EF_HEROISM`)

### I2 Armour with a property [spec](items.md) (R21 to R30, sections 4.7, 4.11), [plan](items_armour_plan.md) (I2.1, I2.2 done 2026-11-12)

Each kind has one hook, and armour spends it on its AC (`EF_WORN_AC`), so
armour can't give AC and a resistance. Weapons don't have the problem:
their bonus is in their attack, their effect in the hook.

- I2.1 Armour's AC in a field of its own in the kind record, as a weapon's
  attack is, counted while it is worn; the hook is then free for a
  property. Proposed over a second hook: 1 byte a kind (166 bytes) against
  3 (about 500), and it follows D&D, where armour has a base AC and magic on
  top. `screen_inv_bank.c`'s special case for `EF_WORN_AC` reads the field
  instead `[done]` (2026-11-12)
- I2.2 Ego armour with it: armour of fire resistance, dragon scale mail (AC
  and fire immunity), elven cloak, shield of warding `[done]` (2026-11-12)
- I2.3 A second hook, only if a kind needs two properties (a ring of fire
  and frost ward) (low)
- I2.4 Enchantment rolled when generated, not fixed +1 and +2 rows: fewer
  kinds for more variety (low)

### I3 Items aimed at a tile or an area

`act_aim()` needs a creature in line (action_aim.md); spells have shapes
(`SPELL_SHAPE_*`), items don't.

- I3.1 Aim at a tile: blink parchment and staff of blinking (`EF_BLINK`,
  only a spell today)
- I3.2 Area items with the spells' shapes: wand of fireballs, parchment of
  scare monsters (`EF_FEAR` on every monster around)

### I4 Thrown potions

- I4.1 A thrown potion breaks and gives the creature hit its use effect:
  bad potions become weapons, and a potion can't be thrown by mistake as a
  drink

### I5 Charges

- I5.1 Recharging parchment: a new op adding charges to a wand (action_aim.md
  D2 keeps empty wands for this), with a chance to destroy it
- I5.2 Rods: charges come back over time, a timer on the item (low)

### I6 Restoring and gaining

- I6.1 Potion of restoration: ends drain effects (`EF_DRAIN_*`). Needs an op
  that ends effects by kind, or the drains to share a status (the status
  bits are nearly full, E5.8)
- I6.2 Potion of experience: an op that adds XP or a level

### I7 Light and fuel (with E2.2)

- I7.1 Torches burn down (an item's count can be burn time, items.md); a
  flask of oil refills a lantern
- I7.2 Light radius matters: needs fog of war, as E5.2 does for monsters

### I8 Weapons against a kind of creature (low)

- I8.1 Creature families (undead, demon, beast), and weapons and ammo that
  do more to one: a mace of disruption, silver arrows, holy water thrown at
  undead

### I9 Digging (low)

- I9.1 Pick-axe and wand of digging: rock or wall becomes corridor and is
  drawn again (`level_decorate_around()`)

### I10 Uses for gold (low)

- I10.1 Gold and gems are only picked up today: shops, or shrines that take
  an offering

Items in other objectives: amulet of life saving (E5.6), ring of free action
(E5.9), invisibility and see invisible (E4.8), a vampiric weapon (E5.5),
lightning wands and weapons (E5.7), telepathy (E5.2), identify (S10), light,
mapping and enchanting (E2).

## J — 
## K — 
## L — Look around
### L1 Look around line of sight
- L1.1 Look checks for line of sight of sight
- L1.2 Target cursor is green if there is  LoS and red if no LoS
## M
## N
## O
## P — Performance optimisations

### P1 Entity loops scale with entities in use, not NGE_ENTITY_MAX ([slot0_tables.md](slot0_tables.md) R9)

- P1.1 Time a monster turn and an idle frame at NGE_ENTITY_MAX 150 against
  100, in CSpect and on hardware
- P1.2 Timer system's three loops over every ID (`nge_timer.c`) walk the
  used list instead
- P1.3 The other loops over every ID, if timing still shows them:
  `nge_location_find()`, the inventory walks (`inventory.c`), `act_pickup.c`,
  `player_bank.c`, the owner searches (`nge_owner_set.c`)

## S - Spells
### S1 - Magical Player Characters have Spell Slots `[done]` (2026-11-10) [spec](spells.md) (section 10), [plan](spells_plan.md) (part 2)
- S1.1 A Spell Slots table defines the number of spells a Player Character can learn by Spell Level `[done]`
- S1.2 The Spell Slot table is indexed by Player Character Class and Player Character Level `[done]`
- S1.3 The  higher the Player Character Level the more spells they can learn `[done]`
- S1.4 Not all Player Character Classes can learn spells (only characters with the Mana Component) `[done]`
- S1.4 Non Player Characters do not learn spells and have a fixed set number of learnt spells defined by their Kind `[done]`
- S1.5 When a Player Character levels up, a message notifies the Player if they can learn new spells `[done]`
### S2 - The game has Spell Books items `[done]` (2026-11-10) [spec](spells.md) (section 10), [plan](spells_plan.md) (part 2)
- S2.1 Implement a new item type - spell book. `[done]`
- S2.1b Implement a sprite for the spell book `[done]`
- S2.2 A spell book contains spells that a spell caster can learn `[done]`
- S2.3 Implement a basic spell book - contains spells levels 1 and 2 `[done]`
- S2.4 Implement a intermediate spell book - contains spells levels 3 and 4 `[done]`
- S2.5 Implement an advanced spell book - contains spells level 5 `[done]`
- S2.6 The Mage and DEV character class start with a basic spell book `[done]`
 ### S3 - Player Character can learn spells from Spell Books `[done]` (2026-11-10) [spec](spells.md) (section 10), [plan](spells_plan.md) (part 2)
- S3.1 The Player can read a Spell Book `[done]`
- S3.2 The Spell Book UI shows all the spells the book contains and highlights which spells the PlayerCharacter  already knows, which spells they could learn (based on available spell slots) and which spells they cannot learn (no spell slot) `[done]`
- S3.3 The Player can select a spell to learn `[done]`
- S3.4 The Player Character can learn a spell if they have an free Spell Slot `[done]`
 ### S10 - Implement identify spell
- S10.1 Each item has an identified flag, default unidentified
- S10.2 An unidentified item's name does not reveal its effects in the UI (e.g. in the inventory screen, messages)
- S10.3 An identified item's name reveals its effects 
- S10.4 The identify spell identifies a single item in the inventory, setting its identified flag
## T - Targetting action
T1 - Selecting previous target
- T1.1 the player character targetting action should select the previous target, if one exists and it is still alive

## U — UI

### U1 Help screen `[done]` (2026-11-09) [spec](help_screen.md), [plan](help_screen_plan.md)

- U1.1 Multiple pages `[done]`
- U1.2 Keyboard commands overview `[done]`
- U1.3 Attributes and what each is used for, as the rules are now
  (help_screen.md R10) `[done]`
- U1.4 Game objective and how to win `[done]`
- U1.5 Context help: `?` on each screen and prompt opens the page for it
  (ui_screens.md R13)
- U1.6 Tell the player about `?` on the title or creation screen

### U2 Main stats panel
- Stats are coloured to show delta from base - cyan for better, red for worse
- 
 ### U3 Update keyboard controls
- sU3.1 Remove **hjkl** as diirection keys
- U3.2 Change key r **r**emove to t **t**akeoff
- U3.3 Implement key **r**ead scrolls 
- U3.4 Implement key q **q**uaff potions
- U3.5 Implement key l **l**ook around
- U3.6 Change key o  **o**perate to A **A**ctivate feature  e.g. shrine, trap 
- U3.7 Implement o **o**pen
- U3.8 Implement key E **E**at  
- U3.9 Change key e **e**xchange to x e**x**change


## V — Visual effects

### V1 Combat and magic VFX `[done]` (2026-11-08) [spec](vfx.md)

- V1.1 Shots fly from shooter to target (arrows, thrown items)
- V1.2 Spell and wand bolts fly, coloured by effect
- V1.3 Glow on a creature an effect takes hold on (healing, curses, buffs)
- V1.4 Damage splash, coloured by damage type, and a white hit flash
- V1.5 A creature killed stays until the blow lands, flashes and puffs
- V1.6 The VFX art (12 patterns)
