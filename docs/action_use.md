# Action: use

`ACT_USE` uses up an item from the pack: drink a potion, eat food, read a parchment that
isn't aimed. The pattern every action follows is in [actions.md](actions.md). Item hooks are
in [items.md](items.md).

Status: built, apart from items with a count (section 4). Code:
[act_use.c](../src/game/actions/act_use.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive); `item` | `effect`: the effect (`EF_*`) | `EV_USED`, then the effect's | No |

## 2. Design

1. **Context:** the actor and the item.
2. **Validate**, all refused, in this order: the actor has the item; it is stowed; it has a
   use hook (`GEN_HOOK_USE`) with an effect. `effect` is set. `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll. Any saving throw is the effect's.
4. **Apply:** `EV_USED`, target the item, source the actor; the item is destroyed; then
   `effect_apply()` on the actor at the hook's power, with the item as the source. The item
   is dying but still readable when the effect's handlers see it.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOT_CARRIED` | Refused | The actor doesn't have the item | None |
| `ACT_R_EQUIPPED` | Refused | It is in a slot | "You must take that off first." |
| `ACT_R_CANT_USE` | Refused | Using it does nothing (a sword) | "You can't use that." |

An item with an aim hook is aimed, not used ([action_aim.md](action_aim.md)). Hunger is a
use with `EF_NOURISH` (items.md, D11).

## 3. Callers

- **The player:** `u`, which opens the inventory screen's use list, or `u` on the inventory
  screen while browsing. The screen does it itself.
- **The AI:** `opt_drink()`, when a monster is at half its hit points or less and carries a
  potion whose use effect is `EF_HEALING`.

## 4. Not built yet

- **Using one of a count.** Every used item is destroyed. Where a subclass has a count that
  is a quantity, use should take one and destroy the item at 0, as fire does with ammo.
- **The item is lost if the effect can't apply** (actions.md, section 8).

## 5. Tests

`test_act`: a use with its events in order, the item destroyed and named as the source, each
refusal, and check mode.
