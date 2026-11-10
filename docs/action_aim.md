# Action: aim

`ACT_AIM` sends an item's effect along a line towards a chosen tile: a wand, a staff or an
aimed parchment. The pattern every action follows is in [actions.md](actions.md), and the
line of fire and its modes are in its section 4.11.

Status: built. Code: [act_aim.c](../src/game/actions/act_aim.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `item`; `x`, `y`: the tile aimed at | `target`: the first creature in line, the one on the tile if the item is direct, or the actor; `effect` | `EV_AIMED`, then the effect's | No |

## 2. Requirements

- **R1** Wands, staves and aimed parchments are aimed at a chosen tile, within
  `ACT_AIM_RANGE`, 8, the same as `AI_SIGHT`. A kind has no range of its own, and one number
  keeps wands simple.
- **R2** The line mode comes from the item kind: `ITEMF_DIRECT` makes it a direct line, and
  otherwise it is a bolt (actions.md R26).
- **R3** Aiming at the actor's own tile picks the actor, so a staff of healing can heal its
  holder. Only aim does this: fire and cast refuse it.
- **R4** A wand or staff uses a charge, and is kept at 0. A parchment is destroyed.
- **R5** The effect names the actor as its source, so a kill earns its XP (actions.md D13).

## 3. Design

1. **Context:** the actor and the item.
2. **Validate**, all refused, in this order:
   - the actor has the item, and it is stowed;
   - it has an aim hook (`GEN_HOOK_AIM`) with an effect;
   - a wand (subclass `ITEM_WAND`, which includes staves) has a charge;
   - `effect` is set;
   - on the actor's own tile, the target is the actor (R3). Otherwise `act_line()` with
     `ACT_AIM_RANGE` and the item's mode (R2), read with `gen_item_flags()` only when the
     line is walked.

   `ACT_CHECK` stops here. A check on the actor's own tile asks about the item alone.
3. **Resolve:** nothing to roll. An effect's own saving throw decides what it does.
4. **Apply:** `EV_AIMED` (target the target, source the actor, p1 the item), the charge
   used or the parchment destroyed (D2), then `effect_apply()` at the hook's power, with
   the actor as the source (R5).

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOT_CARRIED` | Refused | The actor doesn't have the item | None |
| `ACT_R_EQUIPPED` | Refused | It is in a slot | "You must take that off first." |
| `ACT_R_CANT_AIM` | Refused | It has nothing to aim | "You can't aim that." |
| `ACT_R_NO_CHARGES` | Refused | A wand or staff with none left | "It has no charges left." |
| `ACT_R_OUT_OF_RANGE` | Refused | Beyond 8 | "That is out of reach." |
| `ACT_R_NO_LINE` | Refused | A wall, a closed door or the map's edge first | "Something is in the way." |
| `ACT_R_NO_TARGET` | Refused | No creature on the line, or on the tile for a direct item | "There is nothing there to attack." |

**Which items are direct** (`ITEMF_DIRECT`, `0x04` in the kind's flags byte):

| Kind | Effect | Mode | Why |
|---|---|---|---|
| Wand of teleport | `EF_TELEPORT` | Direct | It sends away the creature chosen, not whatever is nearest |
| Staff of healing | `EF_HEALING` | Direct | Healing doesn't fly, and an ally behind an enemy should be the one healed |
| Wand and parchment of sleep, wand of slowness | `EF_SLEEP`, `EF_SLOW` | Direct | A spell on the mind or body, not a missile |
| Wands of striking, fire, frost; staff of power; the missile, fire, frost and acid parchments | `EF_BOLT_*` | Bolt | They fly (actions.md D11) |

13 kinds use aim: 8 wands and staves, and 5 parchments. A row whose effect is still pending,
such as the wand of teleport's, uses its charge and does nothing yet (items.md R12).

## 4. Callers

- **The player:** `a`, which opens the inventory screen's aim list, or `a` or ENTER on the
  inventory screen; then `choose_tile()` with `TARGET_AIM` and `ACT_AIM_RANGE`. The
  inventory screen checks on the player's own tile, so an item isn't refused for want of a
  target there. A parchment says "You aim the", as a wand does.
- **The AI:** none.

## 5. Decisions

- **D1 Aim is separate from use.** The aim hook alone says that an item needs a tile
  (items.md, D8), so the two share no path that matters. *Rejected:* `ACT_USE` with a tile:
  it would give that action two shapes and its callers two prompts.
- **D2 A wand keeps its count at 0; a parchment is destroyed.** An empty wand can be
  recharged later, and it tells the player it is empty. A parchment has no count, so aiming
  one uses it up as `ACT_USE` would. This is a rule by subclass, which items.md R2 allows.
- **D3 The item says which mode, not the effect.** A flag on the item kind, so a wand of
  sleep could be a bolt while a gaze of sleep is direct. `ITEMF_DIRECT` is in the flags byte
  kinds already have, so it costs no table. *Rejected:* a column in the effects table, which
  ties every source of an effect to one mode and adds a byte per effect.

## 6. Not built yet

- **Monsters don't aim,** and `EV_AIMED` has no message for a monster.
- **A lost charge** if the effect can't apply (actions.md, section 8).
- **The CSpect check of the line modes,** by the user: aim the wand of teleport and the
  staff of healing past a creature, and see that the one chosen is reached.

## 7. Tests

`test_act`: a charge used; a wand at 0 kept and refused; a parchment destroyed; the actor's
own tile; range 8 exactly; the actor as source, so a kill names it; a wand of teleport past
a creature reaching the one chosen, and a wand of fire on the same line hitting the nearer
one, with the check giving the same target. `test_gen`: `ITEMF_DIRECT` is on exactly the
kinds in the table. `test_ui` has the keys.
