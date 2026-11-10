# Action: drop

`ACT_DROP` puts a stowed item on the floor of the actor's tile. The pattern every action
follows is in [actions.md](actions.md).

Status: built. Code: [act_drop.c](../src/game/actions/act_drop.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `item` | None | `EV_DROPPED` | No |

## 2. Design

1. **Context:** the actor and the item.
2. **Validate:** the actor has the item, and it is stowed, not equipped. `ACT_CHECK` stops
   here.
3. **Resolve:** nothing to roll.
4. **Apply:** `inv_place()` on the actor's tile, then `EV_DROPPED`, target the item, source
   the actor.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOT_CARRIED` | Refused | The actor doesn't have the item | None: the player chooses from their own list |
| `ACT_R_EQUIPPED` | Refused | The item is in a slot | "You must take that off first." |

A dropped stack stays one item: nothing merges on the floor.

## 3. Callers

- **The player:** `d`, which opens the inventory screen's drop list, or `d` on the
  inventory screen while browsing. The screen does the drop itself.
- **The AI:** none.

## 4. Tests

`test_act`: a drop, each refusal, and check mode.
