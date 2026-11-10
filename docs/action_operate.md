# Action: operate

`ACT_OPERATE` uses a feature next to the actor or on its tile: pray at a shrine or altar,
drink from a fountain. The pattern every action follows is in [actions.md](actions.md).
Features, their kinds and their hooks are in [features.md](features.md), whose requirements
R5 and R6 this action meets.

Status: built. Code: [act_operate.c](../src/game/actions/act_operate.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `target`, or with `NGE_ENTITY_INVALID` a direction `dx`, `dy` | `target`: the feature; `effect`: its use hook's effect | `EV_OPERATED` (p1 the effect), then the effect's | No |

## 2. Requirements

- **R1** A found feature with `FCAN_OPERATE` can be operated: its use hook is applied to the
  actor at the kind's power, with the feature as the source.
- **R2** A feature with `FCAN_ONCE` is spent after one use, and refuses the next with
  `ACT_R_SPENT`. It shows its used look.
- **R3** A rooted actor can operate, as it doesn't change its tile.

## 3. Design

1. **Context and validate:** `act_find_feature(a, FCAN_OPERATE, ACT_R_CANT_OPERATE)`
   (actions.md, 4.13); the feature isn't spent (`FEATURE_IS_SPENT()`). `effect` is set from
   its use hook. `ACT_CHECK` stops here.
2. **Resolve:** nothing to roll. The effect may have a saving throw of its own.
3. **Apply:** `EV_OPERATED` first, as `act_use()` posts its event first, so the effect's
   messages follow it; `effect_apply()` on the actor, the feature as source; then, with
   `FCAN_ONCE`, `feature_spend()`.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOTHING_THERE` | Refused | No feature there, or only a hidden one | "There is nothing there." |
| `ACT_R_CANT_OPERATE` | Refused | It does nothing when used (a door); `target` is it | "You can't use the (feature)." |
| `ACT_R_OUT_OF_RANGE` | Refused | Not on or next to the actor's tile | "That is out of reach." |
| `ACT_R_SPENT` | Refused | It has been used, and works once; `target` is it | "The (feature) is spent." |

## 4. Callers

- **The player:** `o`, then a direction.
- **The AI:** none.

## 5. Tests

`test_act` (`test_operate`): a shrine and a fountain applying their hooks, the feature as
source, a once-only feature spent and refused after, each refusal, a hidden feature not
there, a rooted actor, and check mode.
