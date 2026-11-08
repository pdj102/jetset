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

Topic codes: `C` Character creation, `U` UI.

---

## C — Character creation

### C1 Create a new character at game start `[wip]` [spec](character_creation.md), [plan](character_creation_plan.md)

- C1.1 Select Fighter, Mage (or Dev) class
- C1.2 Starter item kit per class
- C1.3 Additional classes
- C1.4 Randomised stats

## U — UI

### U1 Help screen

- U1.1 Multiple pages
- U1.2 Keyboard commands overview
- U1.3 Attributes and what each is used for (e.g. STR: melee hit and damage;
  DEX: ranged hit, lock pick)
- U1.4 Game objective and how to win
