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
- To pick one up, ask: *"Write a spec for U1"* (whole objective) or
  *"Write a spec and plan for U1.3"* (one feature).

Topic codes: `C` Character creation, `G` Game rules, `U` UI, `V` Visual effects.

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
## P
## S - Spells
### S1 - Magical Player Characters have Spell Slots `[plan]` [spec](spells.md) (section 10), [plan](spells_plan.md) (part 2)
- S1.1 A Spell Slots table defines the number of spells a Player Character can learn by Spell Level 
- S1.2 The Spell Slot table is indexed by Player Character Class and Player Character Level
- S1.3 The  higher the Player Character Level the more spells they can learn 
- S1.4 Not all Player Character Classes can learn spells (only characters with the Mana Component) 
- S1.4 Non Player Characters do not learn spells and have a fixed set number of learnt spells defined by their Kind
- S1.5 When a Player Character levels up, a message notifies the Player if they can learn new spells 
### S2 - The game has Spell Books items `[plan]` [spec](spells.md) (section 10), [plan](spells_plan.md) (part 2)
- S2.1 Implement a new item type - spell book.
- S2.1b Implement a sprite for the spell book
- S2.2 A spell book contains spells that a spell caster can learn
- S2.3 Implement a basic spell book - contains spells levels 1 and 2
- S2.4 Implement a intermediate spell book - contains spells levels 3 and 4
- S2.5 Implement an advanced spell book - contains spells level 5
- S2.6 The Mage and DEV character class start with a basic spell book
 ### S3 - Player Character can learn spells from Spell Books `[plan]` [spec](spells.md) (section 10), [plan](spells_plan.md) (part 2)
- S3.1 The Player can read a Spell Book
- S3.2 The Spell Book UI shows all the spells the book contains and highlights which spells the PlayerCharacter  already knows, which spells they could learn (based on available spell slots) and which spells they cannot learn (no spell slot)
- S3.3 The Player can select a spell to learn 
- S3.4 The Player Character can learn a spell if they have an free Spell Slot
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
