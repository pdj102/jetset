# Action: look

`ACT_LOOK` looks at a tile the actor can see, and tells the player what is on it. It is the
only action that uses no turn. The pattern every action follows is in [actions.md](actions.md).
The cursor that chooses the tile is in [targeting.md](targeting.md).

Status: built 2026-11-13 (L1 in [todo.md](todo.md)). Code:
[act_look.c](../src/game/actions/act_look.c). Section 4.6 lists the differences from the
design. CSpect (section 7): looking across a room and through a wall, 9 tiles away, and no
turn passing all passed on 2026-11-13. The hidden trap and blindness checks weren't run: the
PC tests cover both. Before, look was a player command, not an action (targeting.md R16 to R18): it reached
every tile on the map, through walls, and named hidden traps.

## 1. Goal

Look should tell the player only what their character can see. Today it answers for any tile,
so it shows monsters behind walls and traps that haven't been found. As an action, look gets
the same check as fire, aim and cast, so targeting colours its cursor and refuses ENTER with no
special case (targeting.md D7), and blindness refuses it like any other reason.

## 2. Inputs and outputs

| In | Out | Events | Random |
|---|---|---|---|
| `actor` (alive, placed in an area), `x`, `y` (a tile on the map) | None | `EV_LOOKED` (target the actor, p1 `x`, p2 `y`) | No |

## 3. Requirements

### Sight

- **R1** The actor sees a tile within `AI_SIGHT` (8) of its own, by `nge_map_within()`, when
  the line to it is clear: `nge_block_line()` with `NGE_BLOCK_SOLID`. This is the test a
  monster uses to see (`ai_in_sight()`), so the player and monsters see alike.
- **R2** Walls and closed doors on the way block sight. Creatures, items, open doors and pits
  don't. The two ends aren't tested, so a wall or closed door is itself seen, and so is a
  creature standing in a doorway.
- **R3** The actor's own tile is always seen, unless it is blinded.

### Reasons

- **R4** A blinded actor (`STATUS_BLINDED`) is refused on every tile with `ACT_R_BLIND`, a new
  refused reason: "You can't see!". It is tested first.
- **R5** A tile beyond `AI_SIGHT` is refused with `ACT_R_OUT_OF_RANGE`, and one behind a wall or
  a closed door with `ACT_R_NO_LINE`, after blindness, in that order. Both use the existing
  texts (D5).
- **R6** A tile the actor sees always succeeds, with something on it or not. An empty tile isn't
  `ACT_R_NO_TARGET`: "You see nothing there." is an answer.

### Time

- **R7** Look uses no turn: success sets `used` to 0, and so does every refusal. This is the
  exception to actions.md R10 (D1).

### What the player is told

- **R8** On success the action posts `EV_LOOKED`, and the messages give one line for each entity
  with a kind on the tile, in ID order, as now: "You see a ring of health.", "You see
  yourself." on the player's own tile, and "You see nothing there." if there is none
  (targeting.md R17).
- **R9** A hidden feature (`FEAT_HIDDEN`) isn't seen: no line for it. A hidden trap's tile reads
  as the floor it is drawn as (features.md: unknown to the actor is not there).
- **R10** The messages answer only for the player's own `EV_LOOKED`.

### The cursor (L1.2)

- **R11** Targeting runs look's check at the cursor (targeting.md R19, R20), so the cursor is
  green on a tile in sight, red beyond 8 tiles or out of sight, and amber everywhere when the
  player is blinded. ENTER on a red or amber tile gives the reason's text, and targeting goes
  on (targeting.md R21).
- **R12** Targeting's path without an action goes: every caller gives one (targeting.md D7).

## 4. Design

### 4.1 The action

```c
uint8_t act_look(act_t *a, uint8_t mode) NGE_BANKED
{
    /* 1. Context: the actor's tile and area */
    /* 2. Validate, with no randomness */
    if (act_status(actor, STATUS_BLINDED))                     -> ACT_R_BLIND
    if (!nge_map_within(ax, ay, x, y, AI_SIGHT))               -> ACT_R_OUT_OF_RANGE
    if (!nge_block_line(area, ax, ay, x, y, NGE_BLOCK_SOLID))  -> ACT_R_NO_LINE
    if (mode == ACT_CHECK) return act_checked(a);
    /* 3. Resolve: nothing to roll. 4. Apply: nothing changes; tell */
    nge_event_post(actor, EV_LOOKED, NGE_EVENT_NONE, x, y);
    return act_succeed_free(a);
}
```

- **File:** `act_look.c`, in `BANK_56` with the other actions, with the section's pragmas.
- **Number:** `ACT_LOOK` 21; `ACT_COUNT` becomes 22, and a case in `act_do()`.
- **The ending:** `act_succeed_free()` in `act_internal.h`: success with `used` 0
  (actions.md, section 4.3). It is `act_checked()` by another name, a `#define` to it, so it
  costs no bytes: both set `ACT_R_NONE` and `used` 0 and return `ACT_SUCCESS`. A `#define` to a
  function isn't a comma expression, so T-ZSDCC-2 (actions.md D7) doesn't apply.
- **The constant:** `AI_SIGHT` from `game/ai/ai.h`, read by the action, so the player's sight
  and a monster's have one number (D3).
- **The reason:** `ACT_R_BLIND` 53; `ACT_R_COUNT` becomes 54. Refused in `act_reason_flags[]`,
  and "You can't see!" in `messages_act_failed()`.
- **The event:** `EV_LOOKED` (`NGE_EVENT_USER + 39`), "Looked at tile (p1, p2) in its own area
  (`act_look()`)", in `game/events.h`. Its target is the actor, as for `EV_SEARCHED`.

### 4.2 The messages

`messages_look(x, y)` becomes static in `messages_bank.c` (page 118) and is called from
`messages_on_event()` on `EV_LOOKED` when the target is the player, with p1 and p2 (R10). Its
loop gains one test: an entity that is a feature and hidden (`FEATURE_IS(id) &&
FEATURE_IS_HIDDEN(id)`) is skipped and doesn't count as seen (R9). It leaves `messages.h`.

### 4.3 The player command

`look()` in `player_bank.c` becomes the same as the other tile commands:

```c
static void look(void)
{
    act_t a = { 0 };

    if (choose_tile(ACT_LOOK, &a)) {
        player_act(ACT_LOOK, &a);   /* used is 0: no time passes */
    }
}
```

`player_act()` already leaves the turn alone when `used` is 0, and gives the reason's text on a
failure. The key stays `x` (U3.5 moves it to `l` later).

### 4.4 Targeting

With every caller giving an action (R12), these go from `game/target/`:

- `target_t`'s `origin_x`, `origin_y` and `range`;
- `TARGET_RANGE_ANY` and `target_in_range()`;
- the `ACT_NONE` branches of `target_check()` and `refused()`, and "That is out of range.";
- `target_choose()`'s assert becomes `t->action < ACT_COUNT && t->act != NULL`.

`choose_tile()` stops setting them. `ACT_NONE` leaves `act.h`, as nothing else uses it.
targeting.md's R5 (without an action), R7, R16 and R18 are replaced by this spec.

### 4.5 Cost

| Where | Change | Bytes, estimated |
|---|---|---|
| `BANK_56` | `act_look.c` | About 120 |
| Common memory | A case in `act_do()`, a row in `act_reason_flags[]` | About 8 |
| Page 116 | `modes[]` in the inventory screen, sized by `ACT_COUNT` (actions.md 4.9) | 7 |
| Page 118 | The `EV_LOOKED` case and the hidden test; `ACT_R_BLIND`'s text | About 40 |
| Page 124 | The `ACT_NONE` path and the range test go | About -80 |
| Page 114 | `look()` shorter | About -20 |

`BANK_56` had 395 bytes free in the debug build of 2026-11-13 (actions.md, section 4.1). Built,
look is 291 bytes there and leaves 104 (section 4.6). That is the last action that fits:
`ACT_STEAL` will need another section (actions.md, section 8).

### 4.6 As built

- **No asserts in `act_look()`.** With the two actor asserts of `act_search()`, the debug build's
  `act_look.c` was 500 bytes, 137 of them the asserts' text, and `BANK_56` overflowed by 105.
  Without them it is 291 bytes. Its only callers, the look command and targeting, always pass
  the player, and the PC tests cover both.
- **The cost estimate was low.** 291 bytes, not 120: the five-argument calls to `nge_map_within()`
  and `nge_block_line()` and the banked call through `act_status()` dominate.
- **`refused()` and `target_check()`** keep the action in a local before copying the context
  (T-ZSDCC-1), as before.

## 5. Decisions

- **D1 Look is an action that uses no turn.** It gets the action's check, so blindness and sight
  are its rules, and targeting colours its cursor as it does for the others (targeting.md D7).
  The turn is the one exception to actions.md R10: looking changes nothing in the world, and a
  turn spent on it would only punish the player for asking. *Rejected:* a turn, like other
  actions (a roguelike doesn't charge for looking). *Rejected:* staying a command with a sight
  test of its own, which is the special case D7 set out to remove.
- **D2 The action says it uses no turn, with its own ending.** `used` is already what every
  caller reads (actions.md R8), so the change is one ending, in the one action, and no caller
  changes. *Rejected:* a table of free actions read by `act_succeed()`, which costs common
  memory for one entry. *Rejected:* `player_act()` ignoring `used` for look, which puts a rule
  of the action in its caller.
- **D3 The player sees as a monster sees.** Range `AI_SIGHT` with a clear `NGE_BLOCK_SOLID`
  line, the test in `ai_in_sight()` (answered 2026-11-13: 8 tiles, as monsters). A light radius
  can replace the range when light matters (I7.2). *Rejected:* any range with a clear line,
  which shows a long hall the monsters at its end can't see back. *Rejected:* calling
  `ai_in_sight()`, which is on page 130 and not banked; it is two nge calls, which the action
  repeats, and a test keeps the two in step (section 7).
- **D4 Not `act_line()`.** The line of fire is the wrong test for sight: it refuses the actor's
  own tile and an empty tile (`ACT_R_NO_TARGET`), and stops at a wall on the chosen tile, where
  sight sees the wall. *Rejected:* a third line mode, for one action that needs none of the
  walk's per-tile answers.
- **D5 Blindness refuses every tile, with a reason of its own.** One rule, tested first, so the
  cursor is amber everywhere and ENTER says "You can't see!" (answered 2026-11-13). *Rejected:*
  feeling the actor's own tile and its neighbours, which needs more rules and bytes than
  `BANK_56` has spare. *Rejected:* refusing before targeting opens, a special case in the
  player command.
- **D6 The existing texts for range and line.** `ACT_R_OUT_OF_RANGE` says "That is out of
  reach." and `ACT_R_NO_LINE` "Something is in the way.", which read well enough for looking.
  `messages_act_failed()` is by reason, not by action, so look-only texts would need the action
  in the context. *Rejected:* new reasons for "too far to see" and "you can't see there".
- **D7 What is said comes from an event.** The action posts `EV_LOOKED` and the messages react
  (actions.md R16), as for search. The tile is in p1 and p2; the area is the actor's.
  *Rejected:* the action calling `messages_look()`, which is a reaction in the action.
  *Rejected:* an event for each entity seen, which can fill the queue on a big pile.
- **D8 Hidden features aren't seen.** Look is the player's sight, and the screen draws a hidden
  trap as floor, so look says what the screen shows (R9). Today's look names it, which gives
  the trap away.

## 6. Callers

- **The player:** `x`, then a tile (targeting). The cursor starts on the player.
- **Targeting:** `ACT_CHECK` at the cursor's tile, for its colour and for ENTER.
- **The AI:** none. A monster's sight is `ai_sense()`, which needs no action.

## 7. Tests

`test_act` (`test_look`):

- a tile in sight succeeds with `used` 0, in both modes, and posts `EV_LOOKED` with x and y in
  p1 and p2; check mode posts nothing;
- the actor's own tile succeeds;
- `ACT_R_OUT_OF_RANGE` at the first tile beyond 8 by `nge_map_within()`, not before;
- `ACT_R_NO_LINE` behind a wall and behind a closed door; a wall and a closed door themselves
  are seen; a creature, an open door and a pit don't block;
- blinded: `ACT_R_BLIND` on the actor's own tile and on a tile in sight, before range and line;
- look and `ai_sees()` agree: for a monster and the player on a test map, looking from the
  monster at the player's tile succeeds exactly when `ai_sees()` says it sees them;
- no random numbers drawn; `act_do()` matches the direct call;
- the reason table: `ACT_R_BLIND` is refused, and the attempted reasons are unchanged.

`test_ui`:

- `EV_LOOKED` from the player on a tile with two items gives two lines, on an empty tile
  "You see nothing there.", on the player's tile "You see yourself." first;
- a hidden trap on the tile gives no line, and a found one does;
- `EV_LOOKED` from another actor gives nothing;
- `messages_act_failed()` with `ACT_R_BLIND` says "You can't see!".

`test_target`, with the real action, replacing the tests without an action:

- look at a monster in the open: green, chosen; behind a wall: red, "Something is in the way.";
  9 tiles away in the open: red; at empty floor in sight: green, chosen;
- blinded: amber on the player's own tile, and ENTER says "You can't see!".

CSpect checks (by the user):

- look across a room at a monster: green, and it is named; through a wall: red;
- look at a tile 9 tiles away down a corridor: red;
- a hidden trap in sight: "You see nothing there."; after searching finds it, it is named;
- blinded (a potion of blindness): amber everywhere, ENTER says "You can't see!";
- no turn passes: the turn count and the monsters don't move after a look.

## 8. Not built yet

- **Describing the tile** (a wall, the floor) when nothing is on it.
- **Fog of war and light:** what has been seen before, and a light radius in place of
  `AI_SIGHT` (I7.2).
- **Invisible creatures** (E4.8): look would skip them unless the actor sees invisible.
- **"a" or "an"** by name: all messages use "a" today.
- **A cap on the lines** for a pile bigger than the message window.
- **Starting the cursor on the last target** (T1).
