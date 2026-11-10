# Action: wait

`ACT_WAIT` does nothing for one turn. The pattern every action follows is in
[actions.md](actions.md).

Status: built. Code: [act_wait.c](../src/game/actions/act_wait.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive) | None | None | No |

## 2. Design

Any actor can always wait. Check mode succeeds with `act_checked()`, and do mode with
`act_succeed()`, which uses the turn. Nothing changes, and there is nothing to tell. It has
no reasons.

## 3. Callers

- **The player:** `.`, to let the monsters come.
- **The AI:** at the end of a turn in which no option used it (`ai_act.c`), so a monster's
  turn is always used.

Resting and searching for a while would be a player command that repeats `ACT_WAIT`,
stopping when something happens (actions.md, section 6).

## 4. Tests

`test_act`: it succeeds and uses the turn, a check doesn't, and `act_do()` matches.
