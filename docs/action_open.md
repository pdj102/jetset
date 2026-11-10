# Actions: open and close

`ACT_OPEN` opens a feature next to the actor, such as a door or a chest. `ACT_CLOSE` closes
one. The pattern every action follows is in [actions.md](actions.md). Features are
specified in [features.md](features.md). Unlocking is its own action
([action_unlock.md](action_unlock.md)).

Status: built. Code: [act_open.c](../src/game/actions/act_open.c), with unlock.

## 1. Inputs and outputs

| Action | In | Out | Events | Random |
|---|---|---|---|---|
| Open | `actor` (alive, placed in an area); `target`, or with `NGE_ENTITY_INVALID` a direction `dx`, `dy` | `target`: the feature | `EV_OPENED` | No |
| Close | The same | The same | `EV_CLOSED` | No |

## 2. Requirements

- **R1** A locked feature can't be opened: it is refused with `ACT_R_LOCKED`, and `target`
  is the feature. It is a validation, so the AI's check sees it.
- **R2** A feature can be closed only with nothing else on its tile.
- **R3** An open container can't be closed: once it has spilled there is nothing to close it
  on.

## 3. Design

Both share `find_openable()`, which is `act_find_feature(a, FCAN_OPEN, ACT_R_CANT_OPEN)`
(actions.md, 4.13). It allows the actor's own tile, so an actor can try to close the door it
stands in: it is then refused with `ACT_R_OBSTRUCTED`, by itself.

**Open:**

1. **Context and validate:** `find_openable()`; the feature isn't open; it isn't locked
   (R1). `ACT_CHECK` stops here.
2. **Resolve:** nothing to roll.
3. **Apply:** `feature_open()`, so it blocks nothing and is drawn open, then `EV_OPENED`,
   target the feature, source the actor. A container spills its contents onto its tile in
   reaction (`feature_on_event()`, features.md 4.9).

**Close:**

1. **Context and validate:** `find_openable()`; the feature is open; it isn't a container
   (R3); nothing else is on its tile (R2). `ACT_CHECK` stops here.
2. **Resolve:** nothing to roll.
3. **Apply:** `feature_close()`, then `EV_CLOSED`, target the feature, source the actor.

| Reason | Kind | Open | Close | When | The player is told |
|---|---|---|---|---|---|
| `ACT_R_NOTHING_THERE` | Refused | Yes | Yes | No feature there, or only a hidden one | "There is nothing there." |
| `ACT_R_CANT_OPEN` | Refused | Yes | Yes | Not a feature that opens; for close, an open container | "That doesn't open or close." |
| `ACT_R_OUT_OF_RANGE` | Refused | Yes | Yes | Not on or next to the actor's tile | "That is out of reach." |
| `ACT_R_ALREADY_OPEN` | Refused | Yes | | It is open | "The (door) is already open." |
| `ACT_R_LOCKED` | Refused | Yes | | It is locked; `target` is it | "The (door) is locked." |
| `ACT_R_ALREADY_CLOSED` | Refused | | Yes | It is closed | "The (door) is already closed." |
| `ACT_R_OBSTRUCTED` | Refused | | Yes | An item or a creature on its tile | "Something is in the way." |

A chest is opened with `ACT_OPEN`, not a new action (features.md, D3).

## 4. Callers

- **The player:** bump opens a closed door or chest. If it is locked and a key in the pack
  fits, bump unlocks it instead (`feature_fitting_key()`), and the next bump opens it. `c`,
  then a direction, closes.
- **The AI:** `ai_step()` opens a closed door that blocks a step, if the monster can open
  (`AI_CAN_OPEN`). A locked door refuses it, so `ai_step()` goes round. No monster closes.

## 5. Tests

`test_act`: open and close by `target` and by direction, each refusal, the actor's own tile,
reach to all 8 neighbours and no further, a locked door refused with its `target`, a
container that spills on opening and refuses to close, and check mode.
