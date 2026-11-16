# Noise: implementation plan

The steps that build to-do objective N1 ([todo.md](todo.md)) as
[noise.md](noise.md) specifies it. The spec says what and why; this file
says in what order, what changes, and how each step is checked.

Status: done (2026-11-16): steps 1 to 5 built.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game
  playable.
- **Every step:** `make`, `make test`, and a look at the map for what the
  step cost (section 3). A new `__*` or `asm_*` symbol in common memory is
  a library routine linked by mistake.
- **zsdcc:** read the asm of every changed function that reads several
  per-entity arrays in a loop (`ai_hear()`, the band scan) or a struct
  field (the noise row). These are the known miscompiles, and PC tests
  don't catch them. Copy fields into locals first (T-ZSDCC-1).
- **The CSpect check closes a step.** You run it, and the next step
  waits for the result. A step with nothing to see is checked through a
  later one, as marked.
- **After each step:** anything that differs from the spec goes in an
  "As built" section at the end of noise.md.

### 1.1 Tight spots

- **Page 102 (the level generator) has 39 bytes free** (8,153 of 8,192).
  Step 3's band numbers add a counter and a call in `place_random()`,
  about 25 bytes. If it doesn't fit, `place_random()` and its helpers
  move to the level travel pair (136/137), which has room. That was
  creature_extensions_plan.md's fallback, and it would be a commit of
  its own before step 3.
- **Page 130 (the AI) has 1,120 bytes free.** The plan adds about 530
  (spec 5.5). If a step goes over about 900, it stops and asks. The
  spec's fallback is to move `ai_hear()` to page 142.
- **Common memory (4.7K free in debug builds).** About 60 bytes: the
  mask, the filter and the dispatcher's line.

```text
 1 the noise module and the records   (no change in play)
   |
 2 hearing from the records           (fights wake rooms; steps replace hears())
   |
 3 bands and the alarm
   |
 4 the play test and tuning           (numbers only)
   |
 5 closing docs
```

Step 1 builds everything that makes and spreads noises, but no creature
reads its record yet, so the game plays exactly as before and the cost
of a noise can be timed on its own. Step 2 switches hearing over in one
go: the steps as noise and the removal of `hears()` have to change
together, or sleepers would hear the player twice.

## 2. Steps

### Step 1: the noise module and the records

Spec R1 to R8, R12, R18, D2 to D6, D10, 5.1, 5.2 (up to `ai_hear()`).
N1.1, N1.2.

- **Change:**
  - `src/banks.inc`: page 142, `#pragma define CRT_ORG_PAGE_142 =
    0x8EC000`, and the comment's pair list (142/143 `noise`; 144-151
    free).
  - `game/noise/noise.h`:
    - the `NOISE_*` flags and `noise_row_t`;
    - `noise_init()`, `noise_on_event()`, `noise_step_loudness()`;
    - `noise_event()` for the tests.
  - `game/noise/noise.c` (common memory):
    - `noise_mask[5]`, one bit for each event from `NGE_EVENT_USER`;
    - `noise_on_event()`: an id outside the table or a clear bit
      returns; otherwise it calls `noise_event(e)`.
  - `game/noise/noise_bank.c` (page 142, code and `const` table):
    - `noise_table[]`, spec 5.1's rows, sized `EV_LOOKED -
      NGE_EVENT_USER + 1`;
    - `noise_init()`: builds the mask from the rows with a loudness;
    - `noise_event(e)`, by the row's flags:
      - without `NOISE_ANYONE`, the player is the source or the target;
      - with `NOISE_STEP`, a move in p1 and p2;
      - the tile: the target's, the source's, or (p1, p2) in the
        source's area;
      - the entity at that tile is alive and placed;
      - the loudness, from `noise_step_loudness()` for a step;
      - then `ai_hear()` with a `noise_t` on the stack.
    - `noise_step_loudness()`: returns the step row's loudness (R12, Q1).
  - `game/ai/ai.h`: `noise_t { area, x, y, loud, source, target }`,
    `ai_hear()` (banked) and `AI_MUFFLE`. `AI_HEAR` stays until step 2.
  - `game/ai/ai_pdata.c`, `ai_internal.h` (page 131): `ai_heard[]`
    (heard loudness + 1, 0 none), `ai_heard_x[]`, `ai_heard_y[]`.
  - `game/ai/ai_hear.c` (new, page 130): `ai_hear()`.
    - It scans the used list, rejecting by the AI bit, the area, the
      source and target, and the distance.
    - For each creature still in range, it makes one
      `nge_block_line()` test for `AI_MUFFLE`.
    - It keeps the loudest noise and its tile.
    - The `noise_t` is copied into locals before the loop.
  - `ai_bank.c`: `ai_add()` clears the record.
  - `game.c`:
    - `noise_on_event(e)` goes after `ai_on_event(e)` in
      `game_on_event()`, with no filter there, as the filter is the
      mask;
    - `noise_init()` goes beside the other inits.
  - `tests/Makefile`:
    - `noise/noise.c`, `noise/noise_bank.c` and `ai/ai_hear.c` go in
      `ACT_GAME_SRCS`;
    - a `test_noise` target linked like `test_ai`.
- **Tests:**
  - **`test_noise` (new):**
    - The table and the mask:
      - after `noise_init()`, every row with a loudness has its bit
        set, and no other bit is set;
      - an event id below `NGE_EVENT_USER` or past the table makes no
        noise.
    - Who is involved and where (R4, R5):
      - the player hitting a monster, and a monster hitting the player,
        both make a noise at the target;
      - a monster opening a door is silent, and the player opening it
        isn't;
      - a trap set off by a monster makes a noise at the trap;
      - `EV_AREA` makes its noise at (p1, p2);
      - a player step makes a noise, and `EV_MOVED` with no move (a
        teleport) doesn't.
    - The records:
      - a creature at distance `L` is recorded with heard 0, and one at
        `L + 1` isn't;
      - a wall and a closed door between take `AI_MUFFLE` off, and can
        put a creature out of hearing;
      - the source, the target, and a creature in another area are
        never recorded;
      - a louder noise replaces the record and its tile, and a quieter
        one doesn't;
      - `ai_add()` clears the record.
  - **`test_ai`:** unchanged and passing (no creature reads its record
    yet).
- **Memory:** common about 60 bytes, page 142 about 0.4K, page 130 about
  0.3K, page 131 450 bytes.
- **CSpect (check 6, timing):** play as before; nothing should look
  different. Walk a corridor and fight a few monsters, and compare
  against the previous build: no visible slowdown. A debug build's
  `nge_event_peak()` is unchanged, as noises post no events.

### Step 2: hearing from the records

Spec R9 to R11, R19, D7, D8, D13, 5.3. N1.3, N1.5.

- **Change:**
  - `ai.h`:
    - `AI_NOTICE_DC` (12);
    - `AI_HEAR` goes;
    - `AI_STIM_NOISE`'s comment becomes "Heard a noise (asleep:
      wakes; idle: investigates its tile)";
    - `ai_alert()`'s comment says it wakes sleepers only.
  - `ai_sense.c`:
    - `hears()` goes, and with it the asleep branch's player test;
    - asleep: with a record, 1d20 + the Wisdom modifier + heard against
      `AI_WAKE_DC` gives `AI_STIM_NOISE`;
    - idle: with a record and the player not seen this turn, the same
      roll against `AI_NOTICE_DC` gives `AI_STIM_NOISE`, with the goal
      at the record's tile and tracking 0;
    - the record is cleared in every state, before the context is
      written;
    - the Wisdom roll becomes a helper, as `hears()` was, so test_ai's
      `seed_for_roll()` still finds the d20.
  - `ai_brains.c`: the default brain's idle + `AI_STIM_NOISE` becomes
    `AI_STATE_HUNT` (D7).
  - `ai_bank.c`: `ai_alert()` calls `ai_stimulus()` only when the
    creature is asleep (D13).
- **Tests:**
  - **`test_ai`:**
    - The brain table check (around line 283): idle + noise is now hunt.
    - The hearing tests (around line 430) are rewritten for the record:
      - asleep with a record of heard h wakes on a d20 of
        `AI_WAKE_DC - h` and not one less, with and without a Wisdom
        modifier;
      - asleep with no record never rolls, even with the player
        adjacent (D8).
    - Idle:
      - a record and the roll give hunting, with the goal at the
        record's tile and no target;
      - an idle creature that sees the player hunts the player and
        ignores the record.
    - Investigating:
      - with a clear path it steps towards the goal;
      - at the goal with no one seen, it goes back to idle
        (`AI_STIM_LOST`);
      - seeing the player on the way makes the player the target.
    - Hunting: a record is ignored and cleared.
    - `ai_alert()` on an idle creature changes nothing, and on a sleeper
      wakes it.
  - **`test_noise`:** one end-to-end case. A player hit posts its
    events, and they're handled through `noise_on_event()`. The
    sleeper's next turn, with a forced roll, wakes it.
- **Memory:** page 130 about +100 bytes (the roll and the idle branch,
  less `hears()`).
- **CSpect (checks 1 to 4):**
  1. A room of sleepers: attack one, and the others wake within a few
     turns ("The ... wakes up.").
  2. Sneaking past at 4 tiles: about as often as before. Adjacent: more
     often. Standing still next to a sleeper for 20 turns: it doesn't
     wake.
  3. A sleeper behind a closed door wakes less often than one in the
     open.
  4. An idle monster round a corner from a fight comes to look, and
     wanders off again if it doesn't see the player.

### Step 3: bands and the alarm

Spec R13 to R16, D9, D11, D12, D14, 5.4. N1.4.

- **Change:**
  - `ai.h`:
    - `AI_STIM_ALARM` (5, "A band member was hurt by the source: it
      becomes the target"), and `AI_STIM_COUNT` becomes 6;
    - `ai_band_set(id, band)` (banked).
  - `ai_brains.c`: the new column. The default brain sends asleep and
    idle to `AI_STATE_HUNT`; hunt stays the same. The none brain stays
    the same everywhere.
  - `ai_pdata.c`, `ai_internal.h`: `ai_band[]` (150 bytes, 0 for none).
  - `ai_bank.c`:
    - `ai_add()` sets the band to 0, and `ai_band_set()` sets it;
    - `ai_stimulus()` splits into `stimulus_one()` (the target, goal,
      brain and `EV_AI_CHANGED`, as today) and the band step after it;
    - the band step, for a creature with a band:
      - `AI_STIM_HURT` sends `stimulus_one(member, AI_STIM_ALARM,
        source)` to each other asleep or idle member in the same area;
      - an `AI_STIM_NOISE` that woke it (it was asleep) sends
        `stimulus_one(member, AI_STIM_NOISE, NGE_EVENT_NONE)` to each
        sleeping member;
      - `AI_STIM_ALARM` sets the target and goal as `AI_STIM_HURT`
        does.
    - A kill outright never reaches `ai_stimulus()` (D14): no change.
  - `level_gen.c` (page 102), `place_random()`:
    - a band number counter, reset at the start of each generation;
    - for a pick of 2 or more, the next number goes to each member
      placed, through `ai_band_set()`;
    - if page 102 overflows, the fallback in section 1.1 applies.
- **Tests:**
  - **`test_ai`:**
    - Two sleeping members of band 1 and a member of band 2. Hurting
      one of band 1 makes the other hunt the attacker, with the
      attacker as target and its tile as goal. The band 2 member
      sleeps on.
    - A member in another area isn't alarmed.
    - A member already hunting someone else keeps its target.
    - One hit gives one `EV_AI_CHANGED` for each member and no more (no
      cascade, R16).
    - A member woken by a noise wakes the band's sleepers to idle, not
      to hunting.
    - A member killed outright (dying when the events are handled)
      raises no alarm.
    - The brain table check gains the new column.
  - **`test_level`:** over many seeds, the members of each band share a
    number, the numbers are unique on a level, and creatures not in a
    band have 0.
- **Memory:** page 130 about +130 bytes, page 131 150 bytes, page 102
  about 25 bytes (or 0 if moved to 136).
- **CSpect (check 5):** a jackal pack (or a band at depth 2 to 3).
  Wound one, and the rest come at once. Kill one with one blow, and the
  rest may wake from the death cry but don't come straight for the
  player. If a band is hard to find, a dev-kit band out of sight on
  depth 1 (`GAME_DEV_KIT`, as creature_extensions_plan.md did), removed
  when the check passes.

### Step 4: the play test and tuning

Spec Q5, risks.

- **You play depths 1 to 5 with the Fighter and the Mage,** against the
  findings of play test 3 (rooms fought one at a time) and the new risk
  (rooms that all come at once at depths 1 and 2).
- **Changes are numbers only:** the table's loudness (5.1), `AI_WAKE_DC`,
  `AI_NOTICE_DC` and `AI_MUFFLE`. Each change and why goes in noise.md's
  "As built", with the table as tuned.
- Then difficulty.md section 8.7's W1 and W2 are marked built, and D1.5
  (C7 for the Mage) can be calibrated against this test, as D21 planned.

### Step 5: closing docs

- **noise.md:** the status, and "As built" with each step's differences
  and the numbers as tuned.
- **ai_default_brain.md:** R6 and D6 (hearing) point to noise.md, and
  `AI_HEAR` goes from its design section. **ai.md:** `AI_STIM_ALARM`, the
  default brain's idle + noise, `ai_band_set()` and `ai_hear()`.
- **events.md:** `noise_on_event()` in 4.5's dispatcher and its table of
  handlers.
- **memory_map.md:** page 142's row, page 130's and 131's sizes, common
  memory, and page 102 or 136.
- **difficulty.md:** section 8.7, W1 and W2 built.
- **todo.md:** N1.1 to N1.5 `[done]` with dates. N1.6 stays open (low,
  spec R17).

## 3. Memory

Free bytes before the plan (2026-11-15, from `bin/`), and the expected
total cost:

| Page | Used now | Free | Expected cost | Steps |
|---|---|---|---|---|
| Common memory | | 4.7K (debug) | about 60 bytes | 1 |
| 102 (level generator) | 8,153 | 39 | about 25 bytes, or 0 if moved to 136 | 3 |
| 130 (AI) | 7,072 | 1,120 | about 530 bytes | 1, 2, 3 |
| 131 (AI tables) | 1,050 | 7.1K | 600 bytes | 1, 3 |
| 142 (noise, new) | 0 | 8K | about 0.4K | 1 |
