# Action: unequip

`ACT_UNEQUIP` takes an equipped item off and stows it. The pattern every action follows is
in [actions.md](actions.md).

Status: built. Code: [act_equip.c](../src/game/actions/act_equip.c), with equip and
exchange.

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive); `item` | None | `EV_UNEQUIPPED` | No |

## 2. Requirements

- **R1** A cursed item stays on until its curse is lifted (`inv_cursed()`: the kind's
  `ITEMF_CURSED`, unless the item has `ITEM_UNCURSED`). Reading a parchment of remove curse
  (`OP_UNCURSE`) lifts the curse of everything the reader has equipped.
- **R2** The item needs room in the pack, as it goes back into it.
- **R3** Its worn effect stops counting before the event.

## 3. Design

1. **Context:** the actor and the item.
2. **Validate**, all refused, in this order: the actor has the item; it is equipped; it
   isn't cursed (R1); the pack has room (R2). `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll.
4. **Apply:** `item_slot[]` to `SLOT_NONE`, `effects_recompute()`, then `EV_UNEQUIPPED`,
   target the item, source the actor.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOT_CARRIED` | Refused | The actor doesn't have the item | None |
| `ACT_R_NOT_EQUIPPED` | Refused | It isn't in a slot | "You aren't using that." |
| `ACT_R_CURSED` | Refused | It is cursed | "It is cursed: it won't come off." |
| `ACT_R_FULL` | Refused | No room in the pack | "You can't carry any more." |

`ACT_R_CURSED` is tested before room, so a cursed item says so even with a full pack.

## 4. Callers

- **The player:** `r`, which opens the inventory screen's list of what is in use, or `r` on
  the inventory screen while browsing. The screen does it itself.
- **The AI:** none.

## 5. Tests

`test_act`: taking off, each refusal, a cursed item before and after `OP_UNCURSE`, the worn
effect gone before the event, and check mode.
