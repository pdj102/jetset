# Diagonal movement: requirements and design

Status: built (2026-11-07). PC tests and the CSpect checks (section 7)
pass; the hardware checks are still to do. D1, D3, D4, D5 and D6 were agreed as written
(section 8). Section 9 lists where the build differs from the design.

Covers how the player gives one of the eight directions from a keyboard
with no numeric keypad: by holding two direction keys together (a chord).
It changes the direction keys of play, of the direction prompts and of the
targeting cursor, and adds two small things to `nge_input`.

## 1. Goal

Actions and the AI already use all eight directions ([actions.md](actions.md)
4.7, "Directions"): `act_move()` takes `dx`, `dy` of -1, 0 or 1 each, and a
monster can step or attack diagonally. The player's keys give only four, so
a monster standing diagonally can hit the player, and the player can't hit
back without first stepping. That is a limit of the command, not the action.

The Spectrum Next keyboard has no keypad, so 7 9 1 3 aren't available. The
cursor keys, CAPS+5-8 and h j k l are already the direction keys. Holding
two of them, for example UP and RIGHT, should step up-right.

## 2. Terms

| Term | Meaning |
|---|---|
| Direction key | A key that gives one direction: a cursor key, CAPS+5-8, or h j k l |
| Axis | Horizontal (left, right) or vertical (up, down). A direction key is on one axis |
| Chord | Direction keys on both axes held together: a diagonal |
| Press | A key event for a key that went down |
| Repeat | A key event made by holding a key down (`nge_input`'s repeat) |
| Chord window | The few frames after a press in which a key on the other axis still makes a chord |
| Snapshot | `nge_input`'s held keys for this frame (`nge_key_down()`) |

## 3. Requirements

### Directions

- **R1** In play, holding direction keys on both axes moves the player
  diagonally, with everything a move does today: bump to attack, bump to
  open or unlock a door, strike a web
  (`move()` in [player_bank.c](../src/game/player/player_bank.c)).
- **R2** The direction prompts (close, unlock, operate, disarm) take a
  diagonal answer the same way.
- **R3** The targeting cursor moves diagonally the same way.
- **R4** The direction key sets can be mixed: UP with `l`, or CAPS+7 with
  RIGHT, makes the same chord.
- **R5** Two keys on the same axis (LEFT and RIGHT) cancel on that axis: the
  key of the event decides it (D7).

### Timing

- **R6** The two keys of a chord don't have to go down in the same frame.
  A key on the other axis that goes down within the chord window still
  makes a diagonal.
- **R7** A single key still steps straight. It waits at most the chord
  window, and less if the key comes up first, as it does with a quick tap.
- **R8** A chord gives one step, not a straight step and then another: the
  second key's press doesn't also act.

### Holding

- **R9** Holding a chord repeats the diagonal, as holding one key repeats
  a straight step today.
- **R10 (New)** A repeat never changes direction. When the keys of a chord
  come up a frame or two apart, the key still held mustn't repeat into a
  straight step: in play that could be a bump attack the player didn't
  mean. A repeat only continues the direction of the press it came from,
  and a different direction needs a new press.
- **R11** Adding the second key while holding the first (walking straight,
  then adding UP) turns the walk diagonal at once, as that key's press
  comes with the first key already held.

### The engine

- **R12** `nge_input`'s interface keeps working as it does: the text users
  (`nge_key_ascii()`, the keyboard stream, the line editor) see no change.
  What is added is described in 5.1.
- **R13** Which keys are directions is the game's business, not the
  engine's: `nge_input` knows keys, not directions.

## 4. Decisions

**D1. Chords, not dedicated diagonal keys or a modifier.**

- *Chosen:* two direction keys held together. They are the same keys the
  player already uses, so there is nothing new to learn. A joystick, if the
  game reads one later, gives diagonals the same way.
- *Rejected: `y u b n`* (the vi-keys diagonals of NetHack and Angband). These
  are precise and need no timing, but `u` is Use and `y b n` would be keys
  to learn. They could be added later as well as chords (section 6), if
  chords turn out to be fiddly for single steps.
- *Rejected: a modifier* (for example SYMBOL SHIFT with UP for up-right).
  There is no natural mapping from four keys and a modifier onto four
  diagonals.
- *Rejected: special characters from `nge_key_ascii()`* (codes past 127 for
  the diagonals). That function turns one event into one character, and a
  diagonal is two keys. It would put a game meaning into the engine's text
  layer, and the two keys would still arrive as two events. `dx, dy` from
  `player_key_direction()` already carries a diagonal, so every caller
  (`move()`, the prompts, the cursor) gets one without change.

**D2. The direction comes from the event and the snapshot.** The event's
key gives its own axis. The other axis comes from the snapshot: whether a
direction key on that axis is held now. This alone covers R9 (a repeat of
one key of a held chord) and R11 (a press with the other key already down).
The chord window (D3) covers the rest, the press that comes before its
partner.

**D3. The chord window: 4 frames (80 ms at 50 Hz), one constant.**

- Two keys pressed "together" by a person go down 1 to 3 frames apart. Four
  frames catches that with a frame to spare, and is short enough not to
  feel slow. The figure is to be checked on CSpect and hardware (section 7),
  and is one constant (`PLAYER_CHORD_FRAMES`) to change.
- The window ends early when the first key comes up (R7), so a tap pays
  for its own length, not the whole window.
- *Rejected: no window, the snapshot only.* The game takes the first press
  in the frame it arrives, so a chord would always start with a straight
  step.
- *Rejected: a longer window that the player sets.* A setting to tune is a
  burden, and a long window makes every straight step lag.

**D4. Where the window applies: play and the prompts, not the cursor.**

- In play, a stray straight step is a turn spent, and perhaps an attack. A
  prompt's answer is final. Both use the window.
- The cursor is cheap to correct, as a wrong step is one more key, and it
  is moved many times in a row, so the lag would be felt. It uses the
  snapshot only (D2): a chord gives a diagonal once both keys are down, and
  a key that lands a frame late makes a straight step first.
- *Rejected: the window everywhere,* for the cursor's lag. *Rejected: play
  only,* because a wrong prompt answer costs as much as a wrong step.

**D5. The chord's second press is taken from the queue, not flushed.**
Once a diagonal is decided, if the next event waiting is a press of a
direction key on the other axis, it is taken and dropped (R8). Other
waiting events stay, so keys typed ahead still act.

- This needs a look at the next event without taking it: `nge_key_peek()`
  (5.1).
- *Rejected: `nge_key_flush()`.* Simpler, and the prompts already flush,
  but in play it throws away keys typed ahead during an animation.

**D6. Repeats are marked: `NGE_MOD_REPEAT`.**

- `nge_input` sets a new bit, `NGE_MOD_REPEAT 0x04`, in the high byte of a
  repeat event. The player module keeps the direction of the last press, and
  a repeat whose direction (D2) differs from it is ignored: no command, no
  turn (R10).
- `nge_key_ascii()` already masks the high byte to CAPS and SYMBOL, so text
  is unchanged (R12). Menus may use the bit later, for example so a held key
  doesn't run through a list too fast.
- *Rejected: the game compares the snapshot with the last frame's to tell a
  repeat.* That repeats what `nge_input` already knows.
- *Rejected: no rule, accept the stray step.* The release gap is 1 to 3
  frames and the repeat rate 4, so a stray straight step would end perhaps
  half of all diagonal walks.

**D7. Opposite keys on one axis.** The event's own key decides its axis,
as now. On the other axis, keys both ways held (UP and DOWN) give 0 there.
Nobody means it, and it needs no special case beyond this.

**D8. The module stays in the game.** The key sets, the chord window and
the repeat rule are in the player module (page 114). `nge_input` gains only
what any game could use: `NGE_MOD_REPEAT` and `nge_key_peek()` (R13).

- *Rejected: an `nge_input` direction reader with key sets passed in.* Only
  one game uses it, and the window has to run the game's own frames (the
  map and sprites keep drawing), which `nge_input` can't do.

## 5. Design

### 5.1 nge: `input/`

- **`NGE_MOD_REPEAT 0x04`** in `nge_input.h`, beside `NGE_MOD_CAPS` and
  `NGE_MOD_SYM`. `nge_input_update()` ORs it into the repeat it pushes. The
  repeat keeps the shift keys of its press, as now.
- **`nge_key_event_t nge_key_peek(void)`**, common memory beside
  `nge_key_get()`: the next event without taking it, or `NGE_KEY_NONE`.
- The header's notes on events say a repeat carries the bit.

### 5.2 The game: reading a direction (`player_bank.c`)

`player_key_direction(ev, &dx, &dy)` keeps its name and meaning, a pure
read of one event, and gains the snapshot (D2):

```text
player_key_direction(ev, dx, dy):
    axis and sign from the event's key (as now: nge_key_ascii -> 8 9 10 11, h j k l)
    not a direction key: return 0
    other axis = held_axis(...) on the snapshot (the cursor keys, CAPS+5-8, h j k l)
    return 1
```

`held_axis()` (now in `pan_view()`, under `GAME_DEV_KIT`) comes out of the
`#if` and is used for the other axis. The cursor (D4) needs nothing more.

A new function adds the window and the repeat rule, for play and the
prompts:

```text
player_read_direction(ev, dx, dy):     -- NGE_BANKED, page 114
    if !player_key_direction(ev, dx, dy): return 0
    if ev is a repeat:
        if (dx, dy) != (last_dx, last_dy): return 0       -- R10: no command
        return 1
    if (dx, dy) is straight:
        for up to PLAYER_CHORD_FRAMES frames, while the event's key is held:
            game_frame_start()                            -- draw-only frames, no time passes
            re-read the other axis from the snapshot
            if it is held: (dx, dy) is diagonal; stop
    if (dx, dy) is diagonal and nge_key_peek() is a press on the other axis:
        nge_key_get()                                     -- D5: the chord's second press
    last_dx, last_dy = dx, dy
    return 1
```

- `player_command()` calls `player_read_direction()` in place of
  `player_key_direction()`. A repeat refused by R10 is "no command, no
  turn", like an unknown key.
- `player_direction_key()` (the prompt's answer) calls it too, so R2 comes
  with it. Its "Never mind." stays for a key that isn't a direction.
  `player_ask_direction()` waits for a press and skips repeats: a flush
  doesn't stop a key held from play repeating, and a held key shouldn't
  answer the question, either straight or "Never mind." (New: today a held
  direction key's repeat answers it.)
- The window's frames are the ones `game_wait_key()` runs: they draw and let
  animations play, and no time passes in the world.
- `last_dx, last_dy` are two static bytes in page 114's data.

### 5.3 What doesn't change

- `act_move()` and the bumping in `move()` already take diagonals.
- `nge_key_ascii()`, the line editor and the keyboard stream (R12).
- The help bar. It is full, and the chord isn't a new key. A line in the
  manual or the title screen could come later (section 6).

### 5.4 Memory

- Common memory: `nge_key_peek()`, about 20 bytes, and one `OR` in
  `nge_input_update()`. Common memory is tight
  ([memory_map.md](memory_map.md)), so check the map after the build.
- Page 114: `player_read_direction()` and `held_axis()` leaving the dev
  kit, a few hundred bytes.

## 6. Scope

**In:** R1 to R13: chords in play, the prompts and the cursor,
`NGE_MOD_REPEAT`, `nge_key_peek()`.

**Later, if wanted:**

- `y u b n` as diagonal keys as well (D1), once Use has another key.
- A joystick (Kempston or Mega Drive pad) read into the same directions.
- A line on how to move diagonally, on the title or a help screen.
- Menus using `NGE_MOD_REPEAT`.

## 7. Tests

### PC tests

`tests/test_input.c`:

- A repeat carries `NGE_MOD_REPEAT` and a press doesn't. The repeat keeps
  its press's shift keys as well.
- `nge_key_ascii()` of a repeat is the same character as its press.
- `nge_key_peek()` gives the next event without taking it, and
  `NGE_KEY_NONE` on an empty queue. `nge_key_waiting()` is unchanged by it.

`tests/test_ui.c` (the fake keyboard holds keys across frames):

- UP and RIGHT down in the same frame: one step up-right, one turn.
- UP, then RIGHT two frames later: one step up-right. The RIGHT press is
  taken, and a key typed after it still acts.
- UP alone, held past the window: one straight step after the window.
  UP tapped (released after 2 frames): a straight step, in 2 frames.
- Mixed sets: `k` with RIGHT, CAPS+7 with `l`.
- A diagonal into a monster attacks it; into a closed door opens it.
- Holding a chord: repeats step diagonally. Releasing UP while RIGHT is
  held: the next repeat is refused, no step and no turn. A new press of
  RIGHT steps right.
- Walking right, then pressing UP: the next step is up-right (R11).
- LEFT and RIGHT and UP: the event's key decides the horizontal axis.
- Each direction prompt answered with a chord.
- A direction key held from play into a prompt doesn't answer it; a new
  press does.
- The cursor moves diagonally with both keys down, and straight with one.

### CSpect and hardware checks

- Pressing two keys "together" in play steps diagonally, every time, with
  the cursor keys and with h j k l. If not, try 5 or 6 frames.
- A single key's step doesn't feel slower than before.
- Walking diagonally and letting go never ends with a straight step.
- On hardware: two cursor keys held together read as both (the Next's own
  keyboard membrane), and so do CAPS with two of 5-8.

## 8. Review (2026-11-06)

1. **D1:** chords only for now. Agreed.
2. **D3:** the window starts at 4 frames. Agreed.
3. **D4:** no window for the cursor. Agreed.
4. **D5:** take only the chord's second press, keeping keys typed ahead.
   Agreed.
5. **D6:** refuse a repeat that changes direction. Agreed.

## 9. Build notes (2026-11-07)

- **`last_dx`, `last_dy`** are in common BSS (2 bytes), like the file's
  other statics: page 114 has no data segment of its own.
- **D5, the second press**, is taken only if it matches the diagonal's way
  on the other axis, not just the axis. It is also read with CAPS masked
  off: with CAPS held for CAPS+7, `l` arrives as `L`, which isn't a
  direction key, but `held_x()` counts it. If the `L` press comes *first*
  (before CAPS+7), it still acts as `L`, a development key, and nothing in
  release builds.
- **`player_command()`** returns at once on a refused repeat, so a
  direction key's character never reaches the command switch.
- **Banked depth (T-STACK-2):** page 114 reads a direction through a static
  `direction()`; `player_key_direction()` is its banked wrapper for the
  cursor. The deepest chain (`p`, unlock) is 6 banked calls.
- **R10 in practice:** `nge_input` repeats only the last key to go down,
  so letting go of *that* key stops the repeats outright. The stray straight
  step can only come from letting go of the other key, which is what the
  test does (RIGHT released, UP repeating: refused).
- **Size:** common code +31 bytes (`nge_key_peek()` and the `OR`); page 114
  +0.4K, 5.3K of 8K.

