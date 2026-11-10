# Actions: ascend and descend

`ACT_ASCEND` and `ACT_DESCEND` take the stairs the actor stands on, up or down. `ACT_DESCEND`
also goes down through a known trapdoor. The pattern every action follows is in
[actions.md](actions.md). Level changes are in [level_transitions.md](level_transitions.md),
and leaving the dungeon is in [winning.md](winning.md).

Status: built. Code: [act_stairs.c](../src/game/actions/act_stairs.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed on the current level) | `target`: the stairs | `EV_LEFT_LEVEL` for anyone but the player | No |

## 2. Requirements

- **R1** The player's level change is a request, carried out at the end of the frame,
  because the whole level changes (actions.md, D8). Anyone else leaves at once.
- **R2** Only the player, carrying the quest item, can leave the dungeon from depth 1.
- **R3** A known trapdoor is a way down that arrives anywhere on the next level, as there
  are no stairs at the bottom of it. A hidden one isn't known to be there.
- **R4** A rooted actor can't take the stairs: it fails with `ACT_R_ROOTED`, attempted
  (actions.md R21).

## 3. Design

Both are `take_stairs(a, mode, can, step)`: ascend with `FCAN_ASCEND` and -1, descend with
`FCAN_DESCEND` and 1.

1. **Context:** the first feature on the actor's tile that can do `can` and isn't hidden. A
   trapdoor (`FCAN_TRAP`) means a fall (R3).
2. **Validate:** there are such stairs; `target` is set. The depth they lead to isn't the
   surface, unless the actor is the player and carries the quest item (`quest_carried()`,
   R2). `ACT_CHECK` stops here.
3. **Resolve:** nothing to roll, but a rooted actor fails with `ACT_R_ROOTED` (R4).
4. **Apply:** the player asks for the level with `level_request()`, arriving at the up
   stairs going down, at the down stairs going up, or anywhere for a fall. From depth 1 going
   up, that is leaving the dungeon. Anyone else leaves for the level with `level_send()`,
   which posts `EV_LEFT_LEVEL`.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NO_STAIRS` | Refused | No stairs the right way on the tile | "There are no stairs that way here." |
| `ACT_R_NO_QUEST` | Refused | Out of the dungeon, but not the player with the quest item | "You can't leave without the (quest item)." |
| `ACT_R_ROOTED` | Attempted | The actor is rooted | "You can't move!" |

A trap that drops its victim (a trapdoor going off) isn't this action: it is
`act_spring()`, a reaction ([features.md](features.md), 4.8).

## 4. Callers

- **The player:** `<` and `>`.
- **The AI:** none. Monsters leave a level only by `level_send()` from other causes.

## 5. Tests

`test_act`: up and down, no stairs, the quest rule for the player and for others, the player's
request and the arrival kind, another actor leaving with `EV_LEFT_LEVEL`, rooted (attempted,
the check passes), a known trapdoor arriving anywhere, a hidden one refused, and check mode.
