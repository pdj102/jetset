# Action: search

`ACT_SEARCH` looks for hidden features, such as traps, on the actor's tile and the eight
around it. The pattern every action follows is in [actions.md](actions.md). Hidden features
are in [features.md](features.md), whose requirement R13 this action meets.

Status: built. Code: [act_disarm.c](../src/game/actions/act_disarm.c), with disarm.

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area) | `amount`: how many were found | `EV_FOUND` for each found (source the actor), then `EV_SEARCHED` (p1 the count) | Yes: 1d20 for each hidden feature |

## 2. Requirements

- **R1** Each hidden feature within one tile is found on d20 plus the searcher's Wisdom
  modifier, if it has stats, at or above its kind's DC (`feature_found_by()`).
- **R2** A search always succeeds and uses the turn, finding something or not: finding
  nothing isn't a failure.
- **R3** A rooted actor can search.

## 3. Design

1. **Context and validate:** nothing can stop a search. `ACT_CHECK` succeeds here, rolling
   nothing.
2. **Resolve and apply:** for each tile of the 3 by 3 around the actor, each hidden feature
   there rolls a d20 (R1). One that is found is made known (`feature_find()`, which draws
   it) and gets `EV_FOUND`. Then `amount` is the count, and `EV_SEARCHED`, target the actor,
   p1 the count, so the messages can say when nothing was found.

The rolls and the finding are one loop, so the resolve and apply steps are together, as in a
strike. It has no reasons.

## 4. Callers

- **The player:** `s`.
- **The AI:** none.

## 5. Not built yet

- **A thief's bonus** to the roll (player_character.md R16), when classes have one.

## 6. Tests

`test_act` (`test_search_disarm`): a hidden trap found and not found, depending on the roll;
`EV_FOUND` and `EV_SEARCHED` with the count; nothing found still a success that uses the
turn; only the 3 by 3 searched; and check mode rolling nothing.
