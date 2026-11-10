# Action: steal

`ACT_STEAL` takes an item from a neighbour's pack. It is for the bandit, whose kind says
"steals and runs" ([creatures.md](creatures.md)). The pattern every action follows is in
[actions.md](actions.md).

Status: **not built.** What follows is a proposal. Its requirements and decisions need the
user's review before it is built, and it is blocked by an AI that flees (ai.md R18).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area, with an inventory with room); `target`: the creature next to it | `item`: the item taken | `EV_STOLEN` | Yes: the roll, and which item |

## 2. Requirements (proposed)

- **R1** An actor can steal a stowed item from a creature next to it.
- **R2** No theft can take the quest item (winning.md R6): persistent items are never
  stealable, so `quest_check()` holds.
- **R3** Equipped items are out of reach.
- **R4** A failed theft is attempted: the turn is used.
- **R5** What the thief does next is its brain's, not the action's. The action sets no flee
  state: the AI reacts, as it would to any other stimulus.

## 3. Design (proposed)

1. **Context:** the actor and the target.
2. **Validate**, all refused: the actor has an inventory with room; the target is next to it,
   in the same area, and has an inventory with something stealable stowed (not persistent).
   `ACT_CHECK` stops here.
3. **Resolve:** a roll against the target; failing is `ACT_R_CAUGHT`, attempted. Then a
   random stowed item that can be stolen.
4. **Apply:** `inv_stow()` into the actor, then `EV_STOLEN`, target the item, source the
   actor, p1 the creature robbed.

| Addition | Value |
|---|---|
| Action | `ACT_STEAL` 21, a case in `act_do()`, `ACT_COUNT` 22 |
| Refused reason | `ACT_R_NOTHING_TO_STEAL` 51, with a text |
| Attempted reason | `ACT_R_CAUGHT` 52, with a text |
| Event | `EV_STOLEN`, with messages for the player robbed and for a theft seen |
| Its rows | Sections 4.1, 4.2 and 4.6 of actions.md |

Numbers are given only for ordering: each gets its number when it is built.

## 4. Callers (proposed)

- **The AI:** an option for the bandit next to the player, before attacking, then a flee
  state to run with the item.
- **The player:** none. A thief class could have it later.

## 5. Questions to settle before it is built

- **The roll:** what it is against (the target's Dexterity, its level, being asleep), and
  whether a caught thief is noticed.
- **The bandit's brain:** fleeing (ai.md R18), and what it does once it has got away.
- **Getting the item back:** clean-up drops what a dead thief owns, so killing it returns the
  item. A thief that leaves the level takes the item with it.
- **Memory:** about 0.5K in `BANK_56`, which has 2,022 bytes left in the debug build, and the
  AI's option in page 130.

## 6. Tests (proposed)

`test_act`: a theft, each refusal, a caught thief using the turn, the quest item and equipped
items never taken, a full pack refused, and check mode drawing no numbers.
