# Action: unlock

`ACT_UNLOCK` unlocks a locked door or chest next to the actor, with a key that fits or a
lock pick. It doesn't open it. This file also says where locks and keys are placed. The
pattern every action follows is in [actions.md](actions.md). Locks are part of the feature
component ([features.md](features.md)).

Status: built. Code: [act_open.c](../src/game/actions/act_open.c), with open and close; the
lock rules in [feature.c](../src/game/components/feature.c); the placement in
[level_gen.c](../src/game/level/level_gen.c).

## 1. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area); `item`: a key or a lock pick; `target`, or with `NGE_ENTITY_INVALID` a direction `dx`, `dy` | `target`: the feature | `EV_UNLOCKED` | Yes, for a pick: 1d20 |

## 2. Requirements

**The action:**

- **R1** Doors and chests can be locked. Keys and lock picks unlock them.
- **R2** Unlocking leaves the feature closed: opening it is another action, and another
  turn (D1).
- **R3** A key fits if its group is the lock's kind, or it is a skeleton key (`KEY_ANY`).
  It always works and is kept.
- **R4** A lock pick (any `ITEM_TOOL`) rolls d20 + its group (its bonus) + the actor's
  Dexterity modifier against `FEAT_PICK_DC`, 15, one number for every lock. Each try uses one
  of its count, win or lose, and it is destroyed at 0. Failing is `ACT_R_PICK_FAILED`,
  attempted.

**Where locks go:**

- **R5** Locks guard side rooms only, so a lock never gates progress and a key is a bonus. A
  side room is a room with one doorway and no stairs: a lock there cuts off that room and
  nothing else, as every other room joins up without it.
- **R6** Every side room is locked, `LEVEL_LOCKS_MAX` (2) a level at most. Its doorway gets a
  closed door if it hasn't one. Side rooms are few: 2,000 levels with the Next's `rand()` had
  0.98 locks a level, and 31% of levels had none, having no side room.
- **R7** A locked room holds one random item for the depth, and nothing else, so a lock
  never guards an empty room.
- **R8** Each lock's key is on free floor outside every locked room. If no tile is found
  there is no key, which R5 allows.
- **R9** What is placed at random keeps out of locked rooms: the player arriving anywhere,
  the quest item, parked entities, monsters and items (`free_floor()`, `scan_floor()`).
- **R10** Chests take the locks the side rooms leave, out of the same `LEVEL_LOCKS_MAX`: each
  chest is locked one time in three while locks are left, and its key is placed as a door's
  (features.md, D17).
- **R11** Keys are never random items: a key is made only by the level generator, for a
  lock it makes on that level (R8, R10), so every key found opens a lock at its depth. The
  skeleton key is therefore not generated. Lock picks are random items, the way into a lock
  whose key is lost.

## 3. Design

**The action:**

1. **Context and validate**, all refused, in this order:
   - the actor has the item;
   - `act_find_feature(a, FCAN_OPEN, ACT_R_CANT_OPEN)`, as open and close use (actions.md,
     4.13);
   - the feature is locked;
   - the item is a pick, or a key that fits (`feature_key_fits()`). Anything else, a key
     that doesn't fit or an item that is neither, is `ACT_R_WRONG_KEY`.

   `ACT_CHECK` stops here.
2. **Resolve:** a key turns. A pick rolls 1d20 for `feature_pick_works()` (R4), and is used
   up by one whatever the roll. A failure ends with `ACT_R_PICK_FAILED`, with no event: its
   message comes from the reason. The roll is made in the action, as `attack_hits()` takes
   its roll (actions.md D3).
3. **Apply:** `feature_unlock()`, then `EV_UNLOCKED`, target the feature, source the actor.

| Reason | Kind | When | The player is told |
|---|---|---|---|
| `ACT_R_NOT_CARRIED` | Refused | The actor doesn't have the item | None |
| `ACT_R_NOTHING_THERE` | Refused | No feature there, or only a hidden one | "There is nothing there." |
| `ACT_R_CANT_OPEN` | Refused | Not a feature that opens | "That doesn't open or close." |
| `ACT_R_OUT_OF_RANGE` | Refused | Not on or next to the actor's tile | "That is out of reach." |
| `ACT_R_NOT_LOCKED` | Refused | It isn't locked | "The (door) isn't locked." |
| `ACT_R_WRONG_KEY` | Refused | Not a key that fits, nor a pick | "That doesn't fit the lock." |
| `ACT_R_PICK_FAILED` | Attempted | The pick lost its roll | "The lock holds." |

**The lock.** Its kind (`KEY_*`) is in bits 4 to 6 of `feature_state[]`, with `FEAT_LOCKED`
(D2). `feature_lock()` sets both, and `feature_unlock()` clears them.

**Placing them** (`place_locks()` in the level generator, page 102):

- The rooms with one doorway and no stairs are found from the doorway list,
  `level_doorway_x[]` (D3). Each, up to `LEVEL_LOCKS_MAX`, gets a closed door locked with a
  kind for the depth, a random item inside, and its cell set in `level_locked_cells` (D4).
- All the rooms are marked first, so every key then lands outside every one of them. A key is
  made by its lock's kind (`IT_KEY_IRON` + kind). Key kinds have rarity 0, so
  `gen_item_random()` never makes one (R11).
- **The lock's kind** is `KEY_IRON` plus a roll over `1 + depth / 3` kinds, at most 4, so
  deeper levels add brass, silver and gold. A skeleton key's `KEY_ANY` is never a lock.
- Random placement skips locked rooms in one place, so no caller can forget (D5).
- `place_features()` then locks chests with what is left (R10).

**What can happen to a key:**

- A monster picks it up: it drops it when it dies, and the room stays optional.
- It is left on another level: the room stays optional, and the key may fit a later lock.
- Keys from `gen_item_random()` with no lock to match are a spare that may fit a later lock.

## 4. Callers

- **The player:** bump: a move into a locked door or chest, with a key in the pack that fits
  (`feature_fitting_key()`), unlocks it. Otherwise the bump is an open, refused with
  `ACT_R_LOCKED`. `p` opens the inventory screen's "Try a Lock" list of keys and picks, then
  asks a direction. The list makes no check, as the lock is chosen after the item, so any
  refusal comes after the direction.
- **The AI:** none. `ai_step()` goes round what it can't open.

## 5. Decisions

- **D1 Unlocking is its own action, and doesn't open.** One action is one turn (actions.md
  D5), and `ACT_OPEN` needs no item input. Bump combines them. *Rejected:* opening with the
  key in one action, which would make a locked door cost nothing once the key is found.
- **D2 A lock's kind goes in spare bits of `feature_state[]`.** `KEY_COUNT` is 6, so 3 bits
  hold it. *Rejected:* a new table of 100 bytes in slot 0, which has little left.
- **D3 A side room is found by counting doorways, not links.** A room has one doorway for
  each link, on a different wall, so the doorway list already has the answer. *Rejected:*
  counting bits in `level_links[]`, which needs the neighbours' bits too.
- **D4 The mask is `level_locked_cells`, 2 bytes in page 103.** *Rejected:* a flag in
  `level_room_t`, 12 bytes for what a bit holds.
- **D5 Random placement skips locked rooms in one place,** so no caller can forget.
  *Rejected:* monsters inside, which would need the AI to stay shut in, and is a surprise the
  player has no way to see coming.

## 6. Not built yet

- **A locked door looks like any closed door,** and look doesn't tell them apart. A look of
  its own needs a new tile in the tileset: the map has one palette bank, so it can't be
  recoloured.
- **Monsters don't unlock.**

## 7. Tests

`test_act`: `ACT_R_LOCKED` on open; each key and pick case (a key that fits, `KEY_ANY`, the
wrong key, an item that is neither, a pick that wins and loses, its count used and the pick
destroyed at 0); each refusal; and check mode drawing no numbers. `test_level`: every side
room locked, at most 2 a level, one item inside, each key outside every locked room, every
key on the level fitting a lock there (R11), and random placement keeping out. `test_gen`: no
key from `gen_item_random()` at any depth.
