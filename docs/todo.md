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

Topic codes: `C` Character creation, `E` Effects, `G` Game rules, `U` UI, `V` Visual effects.

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

### E1 Teleport to another level

- E1.1 `OP_LEVEL_TELEPORT`, `arg` up, down or random: the player arrives
  anywhere on the new level. It asks for a level change rather than making
  it there and then (an effect can run inside an event handler), the way the
  trapdoor (`FCAN_DESCEND`) does
- E1.2 Limits: going up stops at depth 1, so it never ends the game
  (level_transitions.md R2); going down stops at the bottom depth. Going
  down while already at the bottom depth, or up while already at depth 1,
  fizzles with a message
- E1.3 On a monster: it leaves the level for good (levels aren't kept).
  Persistent entities (the boss, a quest object holder) are immune
- E1.4 Scroll of teleport level, and a level teleport trap beside the
  teleport trap
- E1.5 Delayed descent: a timed effect with one firing, whose `on_end` is a
  level teleport down by one level times the effect's power (the first
  effect to use `on_end`)
- E1.6 Delayed ascent: a timed effect with one firing, whose `on_end` is a
  level teleport up by one level times the effect's power. A recall up to
  depth 1 is one with a power high enough to reach the limit going up

### E2 Build the pending ops

Defined and used by items and traps, but they do nothing yet
(effects.md, "Pending ops").

- E2.1 Teleport (within the level; goes with E1)
- E2.2 Light
- E2.3 Map
- E2.4 Identify (see S10)
- E2.5 Enchant
- E2.6 Summon

### E3 Fill the gaps in the effect table (data only)

New rows in `effects_pdata.c` using attributes and ops that already work.

- E3.1 Timed Dex, Con, Int, Wis buffs (Grace, Endurance, Insight, …): Might
  is the only one
- E3.2 `EF_WORN_CHA`: Str to Wis have worn versions, Cha doesn't
- E3.3 Timed poison, acid and magic wards: only fire and cold have them
- E3.4 Timed max HP (Aid, Heroism): `ATTR_HP_MAX` modifier
- E3.5 Shield: timed AC without Stoneskin's speed penalty
- E3.6 Vulnerability hex (`ATTR_VULN`) and worn immunity (`ATTR_IMMUNE`):
  both attributes exist, nothing grants them
- E3.7 Ability drain: a long timed ability penalty, from a monster's hit
  or a bad potion (a permanent drain with `OP_RAISE_BASE` -1 would need a
  record of what was lost before a Restore could undo it)
- E3.8 Damage over time beyond poison: burning (fire, Dex save), bleeding
  (slash), corrosion (acid)
- E3.9 Cure blindness: `OP_CURE` with `STATUS_BLINDED`
- E3.10 Tidy names: `EF_STRENGTH` beside `EF_RAISE_DEX` and the other
  `EF_RAISE_*`

### E4 New statuses (4 of the 8 bits free)

- E4.1 Confused: `act_move()` sometimes goes a random direction
- E4.2 Frightened: no melee, the AI flees; a use for Cha (G1)
- E4.3 Paralysed: like asleep, but damage doesn't end it (needs E4.5)
- E4.4 Flying (levitation): potion or boots; fliers don't set off traps
  (features.md R15)
- E4.5 Asleep wakes when damaged (attributes.md, "Not built yet")
- E4.6 Unconscious and blinded read by the rules: an unconscious creature
  loses its turns; the main screen shows every condition, not only poisoned
- E4.7 Silenced: can't cast spells. Uses the last free bit if E4.1 to E4.4
  are all built (low)
- E4.8 Invisible and see invisible: AI, targeting and drawing all change (low)

### E5 New ops and attributes

- E5.1 `ATTR_TO_HIT`: Bless and Bane, Heroism's bonus, and the blinded
  penalty as a modifier rather than a special case in
  `attack_actor_to_hit()`
- E5.2 `OP_DETECT`: detect monsters, detect traps, beside `OP_MAP`
- E5.3 Blink: a short teleport the caster aims, within line of sight
- E5.4 Mana drain: `mana_restore()` takes a `uint8_t`, so it needs a drain
  path or a signed amount
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

### G1 Abilities in attacks

- Str to melee hit and damage, Dex to ranged hit, and a use for Cha. The
  help screen's Abilities page (`screen_guide_pdata.c`) changes with it

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

## V — Visual effects

### V1 Combat and magic VFX `[done]` (2026-11-08) [spec](vfx.md)

- V1.1 Shots fly from shooter to target (arrows, thrown items)
- V1.2 Spell and wand bolts fly, coloured by effect
- V1.3 Glow on a creature an effect takes hold on (healing, curses, buffs)
- V1.4 Damage splash, coloured by damage type, and a white hit flash
- V1.5 A creature killed stays until the blow lands, flashes and puffs
- V1.6 The VFX art (12 patterns)
