# Action: move

`ACT_MOVE` moves the actor one tile, in any of the 8 directions. The pattern every action
follows is in [actions.md](actions.md).

Status: built. Code: [act_move.c](../src/game/actions/act_move.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `dx`, `dy`, each -1, 0 or 1, not both 0 | `target`: the blocker, or `NGE_ENTITY_INVALID` | `EV_MOVED`, then `EV_STOOD_ON` for what is on the new tile | No |

## 2. Design

1. **Context:** the actor's area, the tile at its location plus `dx`, `dy`, and its pass
   flags (`NGE_BLOCK_PASS()`, or none without a block component).
2. **Validate:** `nge_block_can_enter()` with those flags, so a ghost's walls are the block
   component's business. Diagonals are allowed. `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll, but a rooted actor fails with `ACT_R_ROOTED`, attempted
   (actions.md R21, R22).
4. **Apply:** `nge_location_set()`, then `EV_MOVED` (p1 `dx`, p2 `dy`), then `EV_STOOD_ON`
   for each other entity on the tile, with the actor as source. At most 4 items are told
   about (`STOOD_ON_ITEMS_MAX`), as a pile has no limit but the event queue has (events.md,
   D13). A trap on the tile goes off in reaction ([features.md](features.md), 4.8).

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_BLOCKED` | Refused | The map blocks the way: a wall, the edge | "You bump into a wall." |
| `ACT_R_BLOCKED_ENTITY` | Refused | An entity blocks it; `target` is it | "There is a (blocker) in the way." |
| `ACT_R_ROOTED` | Attempted | The actor is rooted | "You can't move!" |

## 3. Callers

- **The player's movement keys** ([diagonals.md](diagonals.md)), through the bump rule
  (actions.md, 4.7). The move command checks the move first, and a refusal with
  `ACT_R_BLOCKED_ENTITY` becomes a melee, an open or an unlock on `target`.
- **The AI:** `ai_step()` tries the move, and opens a closed door that blocks it.

## 4. Tests

`test_act`: a move onto free floor, into a wall, into an entity (with `target`), every
direction, `EV_STOOD_ON` for each entity and at most 4 items, rooted (attempted, the check
passes, a blocker comes first), and check mode.
