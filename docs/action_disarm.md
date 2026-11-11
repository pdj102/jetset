# Action: disarm

`ACT_DISARM` tries to remove a known trap next to the actor or on its tile. It may fail, or
set the trap off. The pattern every action follows is in [actions.md](actions.md). Traps,
and how one goes off (`act_spring()`), are in [features.md](features.md), section 4.8. This
action meets its requirement R14.

Status: built. Code: [act_disarm.c](../src/game/actions/act_disarm.c), with search;
`act_spring()` in [act_spring.c](../src/game/actions/act_spring.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `target`, or with `NGE_ENTITY_INVALID` a direction `dx`, `dy` | `target`: the trap | `EV_DISARMED`; or, set off, `EV_TRIGGERED` and the trap's | Yes: 1d20 |

## 2. Requirements

- **R1** A known trap with `FCAN_DISARM` can be disarmed from its tile or next to it. A
  hidden trap isn't known, so it isn't there.
- **R2** The roll is d20 plus the disarmer's Dexterity modifier, if it has stats, against
  the trap's DC (`feature_disarm_margin()` gives by how much it beats it). At or above it,
  the trap is destroyed. Short by less than 5, the attempt fails. Short by 5 or more
  (`FEAT_DISARM_SET_OFF`, -5), the trap goes off at the disarmer.
- **R3** Both failures use the turn: they are attempted.
- **R4** A rooted actor can disarm.

## 3. Design

1. **Context and validate:** `act_find_feature(a, FCAN_DISARM, ACT_R_CANT_DISARM)`
   (actions.md, 4.13). `ACT_CHECK` stops here.
2. **Resolve:** `feature_disarm_margin()` with a d20.
3. **Apply:**
   - margin 0 or more: `EV_DISARMED`, target the trap, source the actor, and the trap is
     destroyed. Clean-up draws its cell's terrain again. Success.
   - margin -5 or less: `act_spring(trap, actor)`, then `ACT_R_SET_OFF`. The disarmer may be
     next to the trap, not on it.
   - otherwise: `ACT_R_DISARM_FAILED`, with no event: its message comes from the reason.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOTHING_THERE` | Refused | No feature there, or only a hidden one | "There is nothing there." |
| `ACT_R_CANT_DISARM` | Refused | Not a trap that can be disarmed; `target` is it | "You can't disarm the (feature)." |
| `ACT_R_OUT_OF_RANGE` | Refused | Not on or next to the actor's tile | "That is out of reach." |
| `ACT_R_DISARM_FAILED` | Attempted | The roll was short by less than 5; `target` is it | "You fail to disarm the (trap)." |
| `ACT_R_SET_OFF` | Attempted | Short by 5 or more: the trap went off at the actor | None: `EV_TRIGGERED` says so |

**A trap going off** is `act_spring(trap, victim)`, a reaction to `EV_STOOD_ON`, not an
action. It is in `BANK_56` because it strikes as an action does, with `act_strike()`. Its
steps are in features.md 4.8: `EV_TRIGGERED`; found if hidden; its attack rolled, or its hit
hook applied at once; a trap that leads down drops a victim left alive and not rooted to the
next depth.

## 4. Callers

- **The player:** `D` disarms the known trap the player stands on, or asks a direction. It
  is the only way: a step onto a known trap walks onto it (features.md, D12, changed
  2026-11-11).
- **The AI:** none.

## 5. Not built yet

- **A thief's bonus** to the roll (player_character.md R16), when classes have one.

## 6. Tests

`test_act` (`test_search_disarm`, `test_traps`): a disarm that succeeds, one that fails and
one that sets the trap off, with the turn used; each refusal; a hidden trap refused; a
rooted actor; the trap going off with and without an attack; and check mode drawing no
numbers.
