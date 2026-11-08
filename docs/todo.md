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

Topic codes: `C` Character creation, `U` UI, `V` Visual effects.

---

## C — Character creation

### C1 Create a new character at game start `[done]` (2026-11-08) [spec](character_creation.md), [plan](character_creation_plan.md)

- C1.1 Select Fighter, Mage (or Dev) class `[done]`
- C1.2 Starter item kit per class `[done]`
- C1.3 Additional classes
- C1.4 Randomised stats

## U — UI

### U1 Help screen

- U1.1 Multiple pages
- U1.2 Keyboard commands overview
- U1.3 Attributes and what each is used for (e.g. STR: melee hit and damage;
  DEX: ranged hit, lock pick)
- U1.4 Game objective and how to win

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
