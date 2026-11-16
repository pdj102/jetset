# Noise: design specification

Status: agreed (2026-11-15). Q1 to Q6 answered in section 9, as
recommended; D14 added (a band member killed outright raises no alarm).

Covers to-do objective **N1** (N1.1 to N1.6) in [todo.md](todo.md), from
play tests 2 and 3 ([difficulty.md](difficulty.md) section 8.7, D20,
D24): rooms are fought one monster at a time, because attacking one
doesn't wake the rest.

Builds on [ai_default_brain.md](ai_default_brain.md) (hearing, R6 and D6
there: `hears()` in `ai_sense.c`), [ai.md](ai.md) (the state machine and
the brains), [events.md](events.md) (the queue and the dispatcher) and
[creature_extensions.md](creature_extensions.md) M3 (bands).

## 1. Goal

- **Fights are heard.** A hit, a kill or a spell can wake the sleepers
  near it, so picking off one sleeper puts the room at risk.
- **Bands act together.** When one of a band is hurt, the rest come.
- **Sneaking is possible and rewarded.** The player's steps are a noise
  like any other: quiet at a distance, a risk up close, and silent when
  the player stands still. A closed door between the player and a room
  helps. Making the player quieter (stealth, a sneak mode) is one
  modifier on one noise, added later (N2, section 6).
- **One general system.** Events make noises from a table, so a new
  noise is a new row of data. Creatures decide for themselves whether
  they notice and how they react.
- **Cheap.** Silent events cost almost nothing, and a noise costs a scan
  of the level's entities, not of the map.

## 2. Terms

| Term | Meaning |
|---|---|
| **Noise** | A sound made at a tile of an area, with a loudness. Made by an event, by the noise table's rules |
| **Loudness** (`L`) | How far a noise carries, in tiles (Chebyshev distance, `nge_map_within()`) |
| **Heard loudness** | What reaches a creature: `L` less its distance, less `AI_MUFFLE` if a wall or closed door is in the way. Below 0, the creature doesn't hear it |
| **Record** | Each AI creature's loudest noise since its last turn: its heard loudness and its tile |
| **Notice roll** | At the creature's turn, if it has a record: 1d20 + its Wisdom modifier + the heard loudness, against a DC set by its state |
| **Investigate** | An awake creature with no target that notices a noise goes to the noise's tile (hunting with no target), and gives up there if it sees no one |
| **Band** | The creatures placed together as a band at generation (creature_extensions.md M3.2), tagged with a band number on the level |
| **Alarm** | A band member hurt makes the rest hunt whoever hurt it |

## 3. Requirements

### Noises from events (N1.1, N1.2)

- **R1** A noise system reads every event (events.md) and, from a table
  indexed by event, decides whether it makes a noise, how loud, and at
  which tile: the event's target, its source, or the tile in its
  payloads (`EV_AREA`).
- **R2** The table is data: a new noise source is a new row, or a
  changed number, with no new code, as long as its event exists and its
  tile is one of R1's three.
- **R3** The first sources are: an attack (hit or miss), a kill, a shot
  fired, a spell cast, a burst (`EV_AREA`), a trap going off, a door
  opened, closed or unlocked, and the player's steps (section 5.1).
  **Changed:** "a door forced" (N1.2) has no action yet, so it comes
  with that action.
- **R4** A row may count only when the player is the event's source or
  target. This applies to steps, doors, attacks, shots and spells, so a
  monster opening a door or walking doesn't wake its allies. Traps,
  bursts and kills count whoever made them (D6).
- **R5** A step is an `EV_MOVED` with a move in its payloads.
  Teleports and pushes (`EV_MOVED` with no move) are silent.
- **R6** A silent event costs one bit test in common memory and no
  banked call (D2).

### Hearing (N1.3, N1.5)

- **R7** A noise reaches every AI creature in its area within `L` tiles,
  except the event's own source and target. Each creature keeps the
  loudest noise reaching it since its last turn (its record). Nothing
  else happens until its turn (D3).
- **R8** A wall or closed door on the line between the noise and the
  creature takes `AI_MUFFLE` off the heard loudness (D5).
- **R9** At its turn, a creature with a record makes one notice roll and
  clears the record:
  - asleep: against `AI_WAKE_DC`; if it notices, it wakes (idle), as
    hearing does today;
  - idle (awake, no target): against `AI_NOTICE_DC`, and only if it
    didn't see the player this turn; if it notices, it investigates
    (R10);
  - hunting: it ignores the record (its tracking already follows the
    target).
- **R10** **New (N1.3's "coming to look").** Investigating is hunting
  with no target and the noise's tile as the goal. On the way, seeing
  the player makes the player the target, as it does today. At the goal
  with no one seen, it is lost and goes back to idle. This needs no new
  state (D7).
- **R11** **Changed (N1.5).** The player's steps are a table row like
  the rest. `hears()` and its special case are removed. A player who
  doesn't move makes no noise (D8).
- **R12** The player's step loudness goes through one function
  (`noise_step_loudness()`), so stealth can change it later (N2) without
  touching the system. In N1 it returns the table's value (Q1).

### Bands (N1.4)

- **R13** Each creature placed as part of a band gets the band's number,
  unique on its level. 0 means no band.
- **R14** A band member hurt (`AI_STIM_HURT`) raises the alarm. Every
  other member in the same area that is asleep or idle hunts the one
  who hurt it: that one is its target, and that one's tile is its goal
  (`AI_STIM_ALARM`, D9). A member killed outright raises no alarm
  (D14).
- **R15** A band member woken by a noise (or by `ai_alert()`) wakes the
  band's other sleepers, to idle.
- **R16** An alarm doesn't spread further. Members reached by an alarm
  raise none of their own, so one event can't start a cascade.

### Messages (N1.6)

- **R17** **Later (low priority).** Today's "The goblin wakes up." (a
  woken creature in sight) is kept, and covers a sleeper woken by a
  noise. A message for an investigating creature in sight ("The goblin
  hears something.") or for the player hearing a distant noise comes
  later.

### Cost and memory

- **R18** A noise costs one scan of the level's live entities (the used
  list), with the cheapest rejects first, and at most one line test for
  each creature in range. There are no flood fills and no map buffers
  (D4).
- **R19** The notice roll is made once for each creature turn, whatever
  the number of noises, so its rate doesn't depend on the player's speed
  or on how many events an action posts.
- **R20** The AI's page 130 grows by no more than about 600 bytes (it has
  1.1K free). The noise module gets its own page pair (D10).

## 4. Decisions

- **D1 Event push to a record, resolved at the creature's turn (R7,
  R9).** Noises are pushed from events to the creatures in range, and
  each creature resolves its record on its own turn.
  **Rejected:**
  - *Rolling at once, in the event handler:* the roll rate would
    depend on the player's speed and on how many events an action posts
    (a hit posts 2, a kill 3).
  - *A noise buffer read by each creature on its own turn (pull):*
    noises need a lifetime, a slow creature could hear one twice or miss
    it, and every creature turn would pay for the buffer even when
    nothing happened.
  - *A per-turn player noise level only:* cheap, but only the
    player's tile could make noise, so traps, deaths and bursts wouldn't
    count (N1.1).

- **D2 Filter in common memory as a bit mask, built from the table
  (R6).** `noise_on_event()` in common memory tests the event's bit in a
  5-byte mask (one bit an event) and returns at once if it is clear. `noise_init()` builds
  the mask from the banked table, so the two can't disagree.
  **Rejected:** the whole table in common memory (80 bytes, where 4.7K
  is free in debug builds). **Rejected:** the filter inside
  `ai_on_event()` (saves a call in the dispatcher, but it mixes "what
  makes noise" with "who hears it", and the player hearing noises, R17,
  would have to go through the AI).

- **D3 One record for each creature: the loudest, not a sum (R7).**
  A fight's hit, damage and kill in one turn count once, at the loudest.
  Three bytes for each entity in page 131 (loudness, x, y).
  **Option:** a saturating sum, so a long fight wakes more surely than
  one blow. Not needed yet: a fight lasting several turns already gives
  a roll every turn. **Accepted:** a parked creature keeps a stale
  record until it next acts on the player's return, which is at most one
  roll.

- **D4 Range by the Chebyshev distance, from a scan of the used list
  (R18).** The same test as sight and today's hearing. The scan rejects
  by the AI component bit first, then the area, then the distance. With
  6 to 10 monsters among about 100 entities, that's well under a
  millisecond for each noise at 28MHz. **Option:** a list of the level's
  AI creatures, kept on add, remove and level change: faster, but more
  state to keep right. Only if timing (P1.1) shows the scan.
  **Rejected:** sound spreading through the map (a flood fill, Brogue
  style): a scratch map and a fill for each footstep. Revisit only if
  D5's muffling doesn't feel right.

- **D5 Muffled by walls and closed doors: one line test (R8).** For each
  creature in range, `nge_block_line(NGE_BLOCK_SOLID)` blocked takes
  `AI_MUFFLE` (2) off. A closed door then matters for sneaking at the
  cost of one line test for each creature in range. **Rejected:** no
  muffling (walls don't matter, today's "walls or not"). **Rejected:**
  a flood fill (D4).

- **D6 The player must be involved, except for traps, bursts and kills
  (R4).** Monsters are allies (creature_extensions.md D6). If their
  steps and doors made noise, hunting monsters would keep waking
  everything, and idle ones would investigate their friends. A trap
  going off, a burst or a death cry is loud whoever caused it.
  **Option:** every noise counts. Brings monsters to monsters for no
  gain.

- **D7 Investigating is hunting with no target (R10).** The hunt
  state already handles it: with no target, the options that need sight
  (`opt_attack()`, `opt_shoot()`, `opt_cast()`, `opt_stand_off()`)
  return 0, `opt_approach()` steps to the goal, and arriving there
  unseen is `AI_STIM_LOST`, which goes back to idle. The only change is
  the default brain's idle + `AI_STIM_NOISE`, `SAME` to
  `AI_STATE_HUNT`. **Option:** a new `AI_STATE_SEARCH` state:
  clearer in the brain, but a state's handler and table row duplicating
  hunt. **Option:** asleep + noise goes straight to investigating: a
  woken creature then walks to the noise even though it has just woken.
  Today's waking to idle is kept: in the same room it sees the player
  anyway, and the next noise sends it to look.

- **D8 Standing still is silent (R11).** Today a sleeper within 4 tiles
  rolls each of its turns while the player waits. With steps as noise, a
  player who waits next to a sleeper never wakes it. This rewards
  sneaking (waiting for a wanderer to pass), but makes resting next to
  sleepers safe (A3, the rest command). **Option:** a "presence"
  noise of `L` 2 for the player each turn: costs a noise for every
  player turn. **Recommended:** silent, and A3 decides whether rest is
  allowed with monsters near.

- **D9 A new stimulus for the alarm (R14).** `AI_STIM_ALARM`: "a band
  member was hurt by this one". It sets target and goal as
  `AI_STIM_HURT` does, and the default brain sends asleep and idle to
  hunt. **Option:** reuse `AI_STIM_HURT`: no new column, but "attacked"
  and "a friend was attacked" may differ in later brains (a coward flees
  when hurt, not when its friend is). Costs 8 bytes of brain table.

- **D10 The noise module in a new page pair (R20).** `game/noise`: the
  filter in common memory (`noise.c`). The table and its rules (which
  tile, player involved, steps) go in page 142 (`noise_bank.c`, first
  free pair, memory_map.md). Only "who hears it" goes in the AI's page
  130 (`ai_hear()`, the record, the roll, the band alarm), where the AI's
  page data is. **Option:** all of it in page 130: one banked call
  less for each noise, but the table and rules would take about 400 of
  the page's last 1.1K.

- **D11 Band numbers per level, kept by the AI (R13).** `ai_band[]` in
  page 131 (150 bytes). The level generator numbers each band of 2 or
  more from 1 as it places it. Members are matched by number and area,
  so the numbers of parked creatures on other depths don't clash.
  **Rejected:** the leader's entity ID as the tag: IDs are reused, and a
  band must outlive its leader.

- **D12 No range for the alarm, inside the area (R14).** A pack's
  straggler, hurt across the level, brings the rest. Bands start in one
  room, so this mostly matters once they wander (Q3).

- **D14 A member killed outright raises no alarm (R14).** Killing a band
  member with one blow is rewarded: the band isn't told who did it. This
  needs no new code. Events are handled after the action, so a member
  killed by the blow is already dying when `EV_HIT` and `EV_DAMAGED`
  reach `ai_on_event()`, which skips anyone not alive. `ai_stimulus()`
  is never called, so neither is the alarm. The kill still makes its
  death cry (`EV_DIED`, section 5.1). The others roll for it as for any
  noise, and if they wake, they wake to idle or investigate, with no
  target. **Option:** a kill raises the alarm too: no reward for one
  clean blow.

- **D13 `ai_alert()` wakes sleepers only.** Its documentation already
  says "a sleeper wakes". With D7's brain change, `AI_STIM_NOISE` on an
  idle creature would start a hunt with no goal, which is lost at once.
  `ai_alert()` (the shriek, `EF_WAKE`) checks for asleep first.

## 5. Design

### 5.1 The noise table (`noise_bank.c`, page 142)

```c
/* A row's flags */
#define NOISE_AT_SOURCE  0x01   /* at the event's source's tile, not its target's */
#define NOISE_AT_P1P2    0x02   /* at (p1, p2) in the source's area (EV_AREA) */
#define NOISE_ANYONE     0x04   /* whoever made it; without it, only if the player is source or target */
#define NOISE_STEP       0x08   /* a step: a move in p1, p2; loudness from noise_step_loudness() */

typedef struct noise_row {
    uint8_t loud;    /* L, tiles; 0: silent */
    uint8_t flags;   /* NOISE_* */
} noise_row_t;

/* By event, from NGE_EVENT_USER; rows left out are silent */
const noise_row_t noise_table[EV_LOOKED - NGE_EVENT_USER + 1] = { ... };   /* or a new EV_COUNT */
```

The starting numbers, to tune in play (Q5):

| Event | `L` | At | Who | Why |
|---|---|---|---|---|
| `EV_MOVED` (a step) | 4 | target (the mover) | player | Today's `AI_HEAR`: a sleeper 4 tiles away rolls as today |
| `EV_HIT`, `EV_MISSED` | 6 | target | player | A fight carries across a room |
| `EV_DIED` | 6 | target | anyone | A death cry |
| `EV_FIRED` | 3 | source | player | The bowstring; the impact is the hit or miss |
| `EV_CAST` | 5 | source | player | Spoken words: a cost for the Mage (D1.5) |
| `EV_AREA` | 8 | (p1, p2) | anyone | A burst |
| `EV_TRIGGERED` | 8 | target (the trap) | anyone | A trap going off |
| `EV_OPENED`, `EV_CLOSED` | 3 | target (the door) | player | |
| `EV_UNLOCKED` | 2 | target | player | |

`EV_DAMAGED` is silent: it follows a hit, and from timed effects
(poison, burning) it would make a noise every tick.

### 5.2 The flow

```text
 an event is handled (game_on_event)
   ai_on_event()                     as today: the target of an attack is hurt
   noise_on_event(e)                 common: the bit for e->id clear -> return
     noise_event(e)                  banked, page 142: the row; the player involved?
                                     a step?; the tile and area; L (steps:
                                     noise_step_loudness()); L 0 -> return
       ai_hear(&n)                   banked, page 130: n = {area, x, y, L, source, target}
         for each live entity:       AI? in n.area? not n.source/n.target?
           heard = L - distance      below 0 -> next
           line blocked -> heard -= AI_MUFFLE; below 0 -> next
           heard + 1 > ai_heard[id] -> record heard + 1 and (x, y)

 an AI creature's turn: ai_sense()
   asleep, record:  1d20 + Wis mod + heard >= AI_WAKE_DC   -> AI_STIM_NOISE (wakes)
   idle, not seen the player, record:
                    1d20 + Wis mod + heard >= AI_NOTICE_DC -> AI_STIM_NOISE, goal = the
                                                              noise's tile, no tracking
   clear the record (any state)
 ai_stimulus()
   brain: asleep + NOISE -> idle; idle + NOISE -> hunt (D7); + ALARM -> hunt (D9)
   band: HURT -> ALARM (source) to the band's asleep and idle members (R14)
         NOISE that woke it -> NOISE to the band's sleepers (R15)
         neither for a member reached by an alarm (R16)
```

`noise_on_event()` goes after `ai_on_event()` in `game_on_event()`
(events.md 4.5). The order doesn't change the outcome, as records are
resolved at turns.

### 5.3 The numbers (`ai.h`)

```c
#define AI_WAKE_DC    15   /* asleep: 1d20 + Wis mod + heard loudness, as today's DC */
#define AI_NOTICE_DC  12   /* idle: investigates on this or more */
#define AI_MUFFLE     2    /* off the heard loudness through a wall or closed door */
/* AI_HEAR goes: the step's loudness is the noise table's */
```

Against today, for a sleeper with Wisdom modifier 0:

| Case | Today | N1 |
|---|---|---|
| Player walking, 4 tiles | 30% a turn | 30% a creature turn the player moved in |
| Player walking, adjacent | 30% | 45% |
| Player waiting, any distance | 30% within 4 | 0 (D8) |
| Fight 3 tiles away, same room | 0 | 45% a turn of fighting |
| Fight 3 tiles away, through a wall | 0 | 35% |
| Fight 7 tiles away | 0 | 0 |
| Spell cast 2 tiles away | 0 | 45% |

An idle creature 4 tiles from a fight, round a corner (muffled):
1d20 + 0 ≥ 12, 45%, to investigate.

### 5.4 Bands

- `level_gen.c` `place_random()`: for a band of 2 or more, takes the next
  band number (from 1, reset at each level generation) and gives it to
  each member placed, through `ai_band_set(id, band)` (banked, page 130),
  as `gen_creature()` gives the AI component.
- `ai_add()` sets `ai_band[id]` to 0 and clears the record.
- The alarm scans the used list for the same band number and area, as
  `ai_hear()` does, only when `ai_band[id]` isn't 0.
- `ai_stimulus()` raises the alarm through an internal
  `stimulus_one()` that changes one creature's state and doesn't look at
  bands (R16).

### 5.5 Files and memory

| File | Memory | What | Size (estimate) |
|---|---|---|---|
| `game/noise/noise.h` | | `noise_init()`, `noise_on_event()`, `noise_step_loudness()` | |
| `game/noise/noise.c` | Common | The mask (5 bytes), the filter | 50 bytes |
| `game/noise/noise_bank.c` | Page 142 | The table, `noise_init()` builds the mask, `noise_event()`, `noise_step_loudness()` | 400 bytes |
| `game/ai/ai_hear.c` (new) | Page 130 | `ai_hear()`, `ai_band_set()`, the band scan | 350 bytes |
| `ai_sense.c` | Page 130 | The roll on the record; `hears()` goes | +50 net |
| `ai_bank.c` | Page 130 | `ai_stimulus()`'s alarm, `ai_alert()` asleep only | +120 |
| `ai_brains.c` | Page 130 | `AI_STIM_ALARM`'s column, idle + noise | +8 |
| `ai_pdata.c` | Page 131 | `ai_heard[]`, `ai_heard_x[]`, `ai_heard_y[]`, `ai_band[]` | 600 bytes (1,050 to 1,650 of 8K) |
| `game.c` | Common | `noise_on_event()` in the dispatcher, `noise_init()` at start | 10 bytes |

Page 130: about +530 bytes of its 1.1K free. Page 142/143 is the first
free pair; memory_map.md gets its row.

### 5.6 Cost

| Case | Cost |
|---|---|
| A silent event | One bit test in common memory |
| A noise | 2 banked calls; a scan of the live entities (about 100), mostly rejected on the AI bit; for each creature in range, a subtraction and one line test |
| The player's step | One noise (the most frequent source) |
| A creature's turn | One roll if it has a record (as today's `hears()`, which rolled every turn within range) |

## 6. Scope

**In N1:** R1 to R16, R18 to R20: the table, the filter, hearing with
the record and the roll, muffling, investigating, steps as noise, bands
and the alarm.

**Later:**

- **N2 Sneaking (objective N2 in todo.md, Q1).** Everything that changes
  `noise_step_loudness()` or the notice roll, judged after a play test
  of N1:
  - a sneak mode: a command that toggles a status, steps 2 quieter, and
    the player's speed halved while it lasts (an effect, like haste in
    reverse);
  - Dexterity: its modifier off the step's loudness;
  - armour: heavy body armour 1 louder (needs a weight or class of
    armour on the item kinds);
  - a class or racial stealth bonus when classes get rules (pc.h: "no
    rule reads it yet");
  - monsters that are light sleepers or deaf (a trait or a kind's DC
    modifier).
- R17's messages (N1.6).
- A forced door as a noise, with its action (N1.2).
- A list of the level's AI creatures (D4), if timing shows the scan.
- Sound through the map (D4, D5), if muffling isn't enough.
- The player hearing noises out of sight (R17, a later "you hear
  fighting").

## 7. Tests

### PC tests (`make test`)

- `test_noise`: the mask built from the table matches every row; a
  silent event makes no noise; R4's player involvement (a monster opening
  a door is silent, a monster hitting the player isn't, a trap set off
  by a monster isn't); R5 (`EV_MOVED` with no move is silent); each
  row's tile (target, source, payloads).
- `test_ai_hear`: in range and out (L − distance); the source and target
  excluded; another area excluded; muffled by a wall and by a closed
  door; the record keeps the loudest and its tile; the record is cleared
  at the creature's turn, in every state.
- `test_ai` (extended): the asleep roll against `AI_WAKE_DC` with the
  heard loudness; idle against `AI_NOTICE_DC`; idle that sees the player
  ignores the record; investigating: goal set, hunting with no target,
  lost at the goal and back to idle, seeing the player on the way
  targets them; hunting ignores the record; `ai_alert()` on an idle
  creature changes nothing (D13); a player who waits doesn't wake an
  adjacent sleeper (D8).
- Bands: a member hurt makes the band's asleep and idle members hunt
  the attacker; a member killed outright raises no alarm, and the
  others only get the death cry's record (D14); another band and
  another area are untouched; a member
  woken by a noise wakes the band's sleepers to idle; no cascade (each
  member changes state once for one hit); band numbers unique on a
  level from `place_random()` (test_gen).

PC tests use the host's `rand()` (memory: PC rates differ), so the rolls
are tested by forcing them (a seed or a stubbed roll), not by rates.

### CSpect checks (the user runs them)

1. **A room of sleepers.** Attack one of three sleepers in a room. The
   others wake within a few turns of fighting ("The ... wakes up.").
2. **Sneaking past.** Walk past a sleeper 4 tiles away: it wakes about as
   often as before. Adjacent: more often. Stand still next to it for 20
   turns: it doesn't wake.
3. **A closed door.** Fight beside a closed door with a sleeper behind
   it: it wakes less often than in the open.
4. **Investigating.** An idle monster out of sight, round a corner from
   a fight, comes to the fight's tile, and if it doesn't see the player
   there, it goes back to wandering.
5. **A band.** Hit one jackal of a pack (or a band at depth 3+): the
   rest hunt the player at once.
6. **Timing.** A step in a corridor and an idle frame show no visible
   slowdown against the previous build. Hardware later.

## 8. Risks

- **Too hard at depths 1-2.** Fights waking rooms at once may overshoot
  play test 3's "too easy". The numbers (section 5.3) are one table and
  three defines. The balance check (`make balance`) doesn't model waking
  yet, so the judgement comes from a play test (D1.5 is calibrated after
  N1).
- **The Mage.** `EV_CAST` at 5 makes casting near sleepers a real choice,
  which play test 3's Mage didn't face. D1.5 is calibrated after N1, as
  planned.
- **Page 130 fills.** About 530 of 1.1K. If it runs short, `ai_hear()`
  and the band code move to page 142 with the noise module. They would
  then reach the AI's page data through banked setters, which costs more
  calls.
- **zsdcc.** The scan reads several per-entity arrays in one loop (the
  HL-reuse miscompile in memory). Copy fields into locals and read the
  assembly of `ai_hear()`.
- **The investigate flip.** An idle creature investigating posts
  `EV_AI_CHANGED` to hunt and back. "notices you" needs the player as
  source, so no wrong message is shown, but animations or messages that
  read hunt as "after the player" must check the target.

## 9. Questions

All answered (2026-11-15), as recommended:

- **Q1 (R12, N2)** Sneaking: the hook only in N1, or Dexterity's modifier
  now? **The hook only.** Stealth is objective N2 in todo.md, after a
  play test of N1.
- **Q2 (R10, D7)** Should idle creatures investigate noises, or only
  sleepers wake? **Investigate.**
- **Q3 (D12)** The alarm across the whole area, or only within
  `AI_SIGHT` of the one hurt? **The whole area.**
- **Q4 (R4, D6)** Monsters' own doors and steps silent, and traps, bursts
  and kills heard whoever made them? **Yes.**
- **Q5 (5.1, 5.3)** The starting loudness table and DCs, to tune in the
  CSpect play test? **Yes.**
- **Q6 (D8)** Standing still silent, with no "presence" noise?
  **Silent.**

## 10. Changes to other documents (when implemented)

- [ai_default_brain.md](ai_default_brain.md): R6 and D6 (hearing) point
  here; `AI_HEAR` goes.
- [ai.md](ai.md): `AI_STIM_ALARM`; the default brain's idle + noise.
- [events.md](events.md): `noise_on_event()` in 4.5's dispatcher and its
  table of handlers.
- [memory_map.md](memory_map.md): pages 142/143, page 130's and 131's
  sizes, common memory.
- [difficulty.md](difficulty.md): section 8.7's W1/W2 built.
- [todo.md](todo.md): N1's status.

## 11. As built: differences from the design

### Step 1: the noise module and the records

- **The noise is `ai_noise_t`, not `noise_t` (5.2).** It is declared in
  `ai.h`, so it carries the AI's prefix (G-NAME-1).
- **The tile's entity may be dying, not only alive (R1).** Events are
  handled after the action, so the target of a killing blow is already
  dying when its `EV_HIT` and `EV_DIED` arrive. `noise_event()` takes any
  entity in use and placed in an area. Without this, a kill would make
  neither the hit's noise nor its death cry.
- **The distance is `nge_map_within()`'s, not Chebyshev (section 2).**
  The heard loudness uses the least range that reaches the creature, on
  the same circle as sight and every other range. The larger of dx and dy
  (Chebyshev) is the first guess, and the creature is out of hearing
  without a range test when that is beyond the loudness. So a creature at
  (4, 4) from a hit (loudness 6) is at range 6 and hears it at 0.
- **`noise_table[]` is file-local** in `noise_bank.c`. `test_noise`
  checks the mask against the list of loud events, not the table.
- **`noise_init()`** is called once, in `game_run()` after
  `nge_event_init()`, since the mask depends only on the table.
- **`ai_hear()` copies the whole noise into a local struct**, not field by
  field. zsdcc reloaded the pointer for each field, which cost about 90
  bytes; the struct copy is one `ldir`.
- **Memory:** common memory +90 bytes (85 code, 5 the mask), 4,114 free in
  the debug build. Page 130 from 7,072 to 7,466 bytes, +394 (`ai_hear()` is
  384, against about 0.3K planned), 726 free. Page 131 from 1,050 to 1,500.
  Page 142 520 bytes. No new library routines in common memory.
- **The asm was read** for `ai_hear()`, `noise_event()` and
  `noise_on_event()`: the field offsets of the noise and the event, the
  argument order of `nge_map_within()` and `nge_block_line()`, and the
  registers kept across the calls are right.
- **Tests:** `test_noise` 34 checks (new), `test_ai` 227, unchanged.
- **CSpect check 6 (timing) passed** (2026-11-16): no visible slowdown.
