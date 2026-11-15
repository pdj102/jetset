# Action: pick up

`ACT_PICKUP` takes an item from the floor of the actor's tile and stows it, or merges ammo
into a stack the actor already has. The pattern every action follows is in
[actions.md](actions.md). Inventories are specified in [items.md](items.md).

Status: built. Treasure added 2026-11-15 ([difficulty.md](difficulty.md) D12). Code:
[act_pickup.c](../src/game/actions/act_pickup.c), and the gold in `player_on_pc_event()`
([player_bank.c](../src/game/player/player_bank.c)).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `target`: the item, or `NGE_ENTITY_INVALID` for the first item on the tile | `target`: the item | `EV_PICKED_UP` | No |

## 2. Requirements

- **R1** Ammo merges into a stack of the same kind that the actor has, stowed or equipped,
  up to 255, and a merge needs no room in the pack.
- **R2** Only ammo merges. On anything else a count isn't a quantity: it is the charges of a
  wand, the fuel of a light or the uses of a pick ([items.md](items.md), section 4.2).
- **R3 (New, 2026-11-15)** The player's treasure (`ITEM_TREASURE`, gold and gems) goes to
  their gold: its count, with as many XP (player_character.md R12a, R20). It needs no room
  in the pack, and the item is destroyed. A monster's treasure is stowed as any item is, and
  is the player's gold when they pick it up from its body.

## 3. Design

1. **Context:** the actor's tile.
2. **Validate:**
   - the actor has an inventory;
   - the item: with no `target`, the first item on the tile; with one, an alive item on the
     actor's tile;
   - a stack to merge into (`find_stack()`: the actor's first item of the same kind, ammo
     only, with a count below 255), or else room in the pack (`inv_count()` below
     `inv_capacity_of()`), unless it is the player's treasure (R3), which needs none.

   `target` is set to the item. `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll.
4. **Apply:** with no stack, `inv_stow()`. With one, the counts add up: if all of it fits,
   the item on the floor is destroyed; if not, the stack goes to 255 and the rest stays on
   the floor. The player's treasure stays where it is. Then `EV_PICKED_UP`, target the
   item, source the actor.
5. **The gold (R3)** is a reaction to `EV_PICKED_UP` (G-ARCH-3), in
   `player_on_pc_event()`, beside the XP for kills: `pc_gain_gold()` with the item's count,
   then the item is destroyed. It is dying, so the messages, which run after, still name
   it: "You pick up a pile of gold.", then any new level. *Why there:* the gold could have
   been applied in the action, but `BANK_56` had 103 bytes free and page 110 (the PC
   component's) too little, not enough for either; page 114 has 2.3K.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NO_INVENTORY` | Refused | The actor can't carry things | None: the player can |
| `ACT_R_NOTHING_HERE` | Refused | No item on the tile, or `target` isn't there | "There is nothing here to pick up." |
| `ACT_R_NOT_ITEM` | Refused | `target` isn't an alive item | None |
| `ACT_R_FULL` | Refused | No stack and no room | "You can't carry any more." |

## 4. Callers

- **The player:** `g` or `,`, with no `target`.
- **The AI:** `opt_pickup()`, for an idle monster that can carry things, standing on an item.

## 5. Not built yet

- **The merge rule is in the action,** in `find_stack()`, where R12 of actions.md would put
  it in the inventory component. It was put here because page 110, where the component is,
  was short of room. It can move when that page has the room.

## 6. Tests

`test_act`: picking up by `target` and the first on the tile, each refusal, a full pack, and
stacks: a merge into a stowed or equipped stack with no room needed, exactly to 255 and past
it (the rest left on the floor), a full stack, another kind, and a wand that doesn't merge.
Treasure: the player's with a full pack, left on the floor for the handler; a goblin's
stowed. `test_ui`: the handler's gold and XP, the item destroyed, the stats window's gold,
and nothing for a monster's pick up or a target that isn't an item.
