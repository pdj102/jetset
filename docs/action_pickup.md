# Action: pick up

`ACT_PICKUP` takes an item from the floor of the actor's tile and stows it, or merges ammo
into a stack the actor already has. The pattern every action follows is in
[actions.md](actions.md). Inventories are specified in [items.md](items.md).

Status: built, apart from treasure (section 5). Code:
[act_pickup.c](../src/game/actions/act_pickup.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `target`: the item, or `NGE_ENTITY_INVALID` for the first item on the tile | `target`: the item | `EV_PICKED_UP` | No |

## 2. Requirements

- **R1** Ammo merges into a stack of the same kind that the actor has, stowed or equipped,
  up to 255, and a merge needs no room in the pack.
- **R2** Only ammo merges. On anything else a count isn't a quantity: it is the charges of a
  wand, the fuel of a light or the uses of a pick ([items.md](items.md), section 4.2).

## 3. Design

1. **Context:** the actor's tile.
2. **Validate:**
   - the actor has an inventory;
   - the item: with no `target`, the first item on the tile; with one, an alive item on the
     actor's tile;
   - a stack to merge into (`find_stack()`: the actor's first item of the same kind, ammo
     only, with a count below 255), or else room in the pack (`inv_count()` below
     `inv_capacity_of()`).

   `target` is set to the item. `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll.
4. **Apply:** with no stack, `inv_stow()`. With one, the counts add up: if all of it fits,
   the item on the floor is destroyed; if not, the stack goes to 255 and the rest stays on
   the floor. Then `EV_PICKED_UP`, target the item, source the actor.

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

- **Treasure goes to the player's gold** (player_character.md R20). `pc_gain_gold()` exists,
  but nothing calls it from pick up: treasure is stowed as any item is.
- **The merge rule is in the action,** in `find_stack()`, where R12 of actions.md would put
  it in the inventory component. It was put here because page 110, where the component is,
  was short of room. It can move when that page has the room.

## 6. Tests

`test_act`: picking up by `target` and the first on the tile, each refusal, a full pack, and
stacks: a merge into a stowed or equipped stack with no room needed, exactly to 255 and past
it (the rest left on the floor), a full stack, another kind, and a wand that doesn't merge.
