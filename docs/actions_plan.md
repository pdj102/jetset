# Actions: implementation plan

The steps that build what [actions.md](actions.md) section 6 specifies. The spec says
what each action does and why, and this file says in what order, what changes, and how
each step is checked.

Status: plan written 2026-10-31. D10 to D23 agreed. Steps 1 to 13 done: step 1 passed in
CSpect and on hardware, the others in CSpect, with step 5 checked through step 7. Step
14 needs its own specs first.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game playable and adds
  no half-built command.
- **Every step:** `make`, `make test`, and a look at the map for what the step cost.
- **The CSpect check closes a step.** The user runs it, and the next step waits for the
  result. Hardware is checked at the points marked below.
- **After each step:** update `act.h`, `actions.md` and the other docs listed. Anything
  that differs from the spec goes in an "As built" section in `actions.md`.
- **New reasons and events are added when a step needs them,** not ahead of it, as
  `ACT_COUNT` sizes a table.

```text
 1 BANK_56 --> 2 one distance rule --> 3 act_strike --+
                                                      |
                     4 line walk --> 5 act_line ------+--> 7 ACT_FIRE --> 8 stacks
                                                      |        |
                     6 attack data -------------------+        +--> 9 ACT_AIM
                                                               +--> 10 AI shooting

 11 two hands, curses     (needs only step 6's accessor)
 12 lock mechanism        (needs step 1)
 13 lock placement        (needs step 12; Q1: option A)
 14 ACT_CAST, ACT_STEAL   own specs first
```

Steps 1 to 9 are the core: when they are done, 28 of the 35 inert item kinds work. Steps
12 and 13 are the other 7.

## 2. Steps

### Step 1: move the actions to `BANK_56`

No change in behaviour (D22).

- **Change:** `#pragma codeseg` and `constseg` from `PAGE_112` to `BANK_56` in the 10
  `act_*.c` files that have them. `src/banks.inc`: `CRT_ORG_BANK_56 = 0x70C000` in place
  of `CRT_ORG_PAGE_112`.
- **Docs:** page 112 in the comments of `act.h`, `act_internal.h`, `act.c`,
  `act_shared.c` and every action's header, P-ACT-1, `memory_map.md`'s rows and its
  892-byte figure, `src/game/README.md`.
- **Tests:** none change, as the PC tests don't see sections.
- **CSpect:** a short game touching every action: move, fight, pick up, drop, equip,
  use, doors, stairs. Then **hardware** once: this is the first `BANK_n` section in the
  project.

A trial on a scratch copy (2026-10-31) built and passed `check-banks.sh`, with the same
addresses and size. Padded to 10,320 bytes, it put `act_wait()` at `$E79B`, and the
bytes in the NEX file were in page 113. That shows the build works, not that the code
runs: nothing was run past `$E000`. So the CSpect check isn't done until code past the
8K line has run, which step 1 alone can't show. Add a temporary constant in the check
build to push `act_wait()` over the line, press `.` and remove it.

Done 2026-10-31. The normal build's NEX file was byte-identical to the one before, so the
check ran a scratch build with 8K of padding in `act_drop.c`, which put nearly every action
and the shared endings in page 113. It passed in CSpect and on hardware.

### Step 2: one distance rule

No change in behaviour (D23, R23).

- **First:** a test that melee refuses a target on the actor's own tile, and that melee
  and open reach exactly the 8 neighbours, passing on the code as it stands.
- **Change:** `act_melee.c` uses `nge_map_within()` with `attack_range[]`, and a test
  that the tiles differ. `act_open.c` uses `nge_map_within()` with range 1. `act_melee()`
  stops reusing `roll` for the distance. `act_distance()` goes.
- **Docs:** `actions.md` 4.1, 4.10 and section 8.
- **CSpect:** diagonal monster attacks and opening a door next to the player.

### Step 3: `act_strike()`

No change in behaviour (D21).

- **Change:** melee's roll, hit, damage and hook move to `act_strike()` in `act_shared.c`,
  taking who attacks, the use, and whose hook applies. `act_melee()` keeps its validation.
- **Tests:** `test_act` unchanged and passing is the check.
- **Read the asm** of `act_strike()`, for T-ZSDCC-1: it reads the context's fields across
  branches.

### Step 4: the line walk

A new engine function, with nothing in the game using it yet (spec, 6.2).

- **Change:** in `nge_block_line.c`, a walk state on the caller's stack with
  `nge_block_walk_start()` and `nge_block_walk_next()`, giving each tile from the first after the
  start. `nge_block_line()` is rewritten on it.
- **Why a state and not a callback:** a pointer to a banked function doesn't switch banks
  (N-BANK-6), and a walk can stop as soon as the action has its answer.
- **Tests:** `test_block`: all 8 directions, steep and shallow lines, the same tiles as
  `nge_block_line()`'s, one step, and the tile itself.
- **CSpect:** monsters still see and chase as before, as sight now runs on the walk.

### Step 5: `act_line()`

Spec, 6.2. Nothing calls it until step 7.

- **Change:** `act_line()` in `act_shared.c`, in validation style: no randomness, returns
  a reason. `ACT_R_NO_LINE`, with its row and text.
- **Tests:** called directly in `test_act`. Stopped by a wall, a closed door, a creature
  first in line, an open door, a pit, the map's edge.
- **A commit of its own:** nothing in the game calls it until step 7, but the tests do.

### Step 6: the attack data

- **Change:** `gen_item_group()`, `gen_item_flags()` and `gen_item_attack()` beside
  `gen_item_fits()`, in page 108. `attack_hits()` and `attack_damage()` take a bonus, and
  `act_strike()` passes 0.
- **Memory:** page 110 has 460 bytes left, and this is two parameters. Check the map.
- **Tests:** `test_gen` for the accessors, `test_attack` for bonuses, with 1 and 20 still
  always missing and hitting.

### Step 7: `ACT_FIRE`

The first new action (spec, 6.3).

- **Change:** `act_fire.c`; `ACT_FIRE`; `ACT_R_NO_AMMO`, `ACT_R_WRONG_AMMO`; `EV_FIRED`
  with a message such as "You fire."; `TARGET_FIRE`.
- **Command:** `f` uses `target_choose()` from the player's tile with the shooter's range,
  and `PLAYER_PROMPT_FIRE` goes. With nothing to fire it says so before the cursor
  appears, from a check.
- **Dev kit:** a short bow, arrows and darts in `dev_kit[]`, listed in `dev.h` (G-DEV-1).
- **Tests:** the spec's section 7 list, the largest action against events.md R25, and the
  range test against `target_in_range()` on the same tiles.
- **CSpect:** equip a bow and arrows, shoot, miss, kill for XP, run out. Darts, a flask of
  oil's fire. A wall in the way, a green cursor refused, a rat between the player and a
  goblin.
- **Not in this step:** the projectile animation. It needs overlay patterns
  (animations.md R14) and gets `EV_FIRED` when it comes.

Ammo equips through the ordinary `w` screen, as it fits `SLOT_AMMO`.

### Step 8: stacks

- **Change:** `ACT_PICKUP` merges into a stowed or equipped stack of the same kind, up to
  255, and the rest stays where it is. A merge needs no room. `EV_PICKED_UP` as now.
- **Why here:** with fire working, a second sheaf of arrows is otherwise a nuisance.
- **Tests:** merge, no room but a merge, 255, the item destroyed, `EV_STOOD_ON` limits
  unchanged.
- **CSpect:** pick up a second sheaf of arrows, stowed and equipped, then fire them.
- **Treasure** going to gold is left to player_character.md R20, with its question Q3.

### Step 9: `ACT_AIM`

Spec, 6.4. Reuses steps 5 and 7's shape.

- **Change:** `act_aim.c`; `ACT_AIM`; `ACT_R_CANT_AIM`, `ACT_R_NO_CHARGES`; `EV_AIMED`;
  `TARGET_AIM`; a list of aimable items on the inventory screen, `LIST_AIMABLE`, with an
  `a` key.
- **Tests:** a charge used, a wand at 0 kept, a parchment destroyed, own tile, the actor
  as source (a kill gives XP).
- **CSpect:** a bolt, sleep (which no rule reads yet, so only its message), the staff of
  healing on oneself, the wand of teleport doing nothing yet.

### Step 10: monsters that shoot

- **Change:** `AI_CAN_RANGED` in `ai_can()`. `opt_shoot()`, ahead of `opt_approach()`. It
  tests sight before its check, as a check walks a line (section 8 of the spec).
  `opt_wield()` learns launchers and ammo.
- **Limit:** only monsters that pick up a launcher can shoot. A creature kind has no ranged
  part (`gen_creature()` adds melee only), so the gold snake's spit needs a kind record
  change, and is out of this step.
- **CSpect:** a bandit that has picked up a bow and arrows, placed there for the check.

### Step 11: two-handed items and curses

Spec, 6.8.

- **Change:** the rule in the inventory component, with `gen_item_flags()` from step 6;
  `ACT_EQUIP` and `ACT_UNEQUIP` call it; `ACT_R_CURSED`. Page 110 has the least room of
  anything here, so check it first. If it won't fit, the rule goes in `act_equip.c`, with
  a note against R12.
- **`OP_UNCURSE` is built in this step** (Q2), as an uncurse is the only way back from a
  cursed item.
- **Tests:** both orders for a shield and a two-handed weapon, a cursed item staying on,
  dropped creatures' cursed gear (`inv_drop_all()` isn't an action, so it isn't refused).

### Step 12: the lock mechanism

Everything that doesn't depend on Q1. Can be built before Q1 is answered. It changes
nothing in play until step 13, as no door is locked.

- **Change:** `FEAT_LOCKED` and the lock's `KEY_*` in `feature_state[]` (D19);
  `feature_lock()`, `feature_unlock()` and the pick rule in the component, in page 132; `ACT_OPEN`
  refuses with `ACT_R_LOCKED`; `act_unlock.c`; `EV_UNLOCKED`; bump to unlock; a list of
  tools on the inventory screen.
- **Dev kit:** a key that locks the nearest door, with a key and a pick in `dev_kit[]`.
  A dev aid only, so it can be seen in CSpect with no generator.
- **Tests:** spec 6.5's table, and `ACT_CLOSE` on a locked door's state. The AI doesn't
  walk into a locked door for ever.
- **Open detail:** the pick's difficulty number, to set with Q1, as it depends on what a
  lock guards.

### Step 13: placing locks and keys

Q1 is answered: option A, and the spec is in [actions.md](actions.md), section 6.12. Level
generation and the placement of keys. A locked door's own look needs new tile art, so it
keeps the closed door's until then.

### Step 14: `ACT_CAST` and `ACT_STEAL`

Each needs its own spec first: spells and who knows them, and fleeing (ai.md R18). Then
`ACT_CAST` reuses step 9's ending and step 5's line. `ACT_OPERATE`, `ACT_SEARCH` and
`ACT_DISARM` wait for feature kinds.

## 3. Memory

| Step | Actions' section | Elsewhere |
|---|---|---|
| Start | 7,300 of 16,384 | Common: 4,711 free. Page 110: 460 free |
| 1 | The same | |
| 2 | -49: 7,251 | |
| 3 | +195: 7,446 | |
| 4 | | Page 56: +355, to 651 |
| 5 | +336: 7,782 | Page 118: +13. Common: +1 |
| 6 | +7: 7,789 | Page 108: +303. Page 110: +18, 442 left |
| 7 | +841: 8,630 | Common: +28, 4,682 free. Pages 114 +76, 116 +7, 118 +123, 124 +57 |
| 8 | +316: 8,946 | |
| 9 | +706: 9,652 | Common: +28, 4,653 free. Pages 114 +107, 116 +181, 118 +116, 124 +55 |
| 10 | | Page 130 +275, page 118 +86 on hardware, with common unchanged. Debug: page 130 +284, page 102 +200 (the dev bandit), common +14, 4,640 free |
| 11 | +76: 9,728 | Page 110: +186, 256 left. Page 120 +95, page 118 +46. Common +1, 4,639 free |
| Between 11 and 12 | | `feature.c` moved to its own pair, 132/133: page 110 -928, about 1,180 left |
| 12 | +498: 10,226 | Page 132 +462, 6,802 free. Page 118 +188, page 116 +133, page 114 +525 with the dev key. Common +30, 4,609 free |
| 13 | | Page 102: +790, 822 left in debug. Page 103 +2 |

The estimates come from the spec, 6.11. The actions ended at 10.2K of 16K, so the
section has room. **Page 110 was the real limit:** the game's component functions are
there. The game code range was extended to 102-151 and `feature.c`, which step 12 and the
features after it add to, moved to pair 132/133. If page 110 fills again, the next component
to move is the one with the most still to come (G-MOD-3).

## 4. Open questions

**Q1 How is a key guaranteed to be reachable?** Answered: option A, built in step 13. What
the options were:

| Option | How it works | Cost | Weakness |
|---|---|---|---|
| A. Locks guard side rooms only | The way between the stairs never crosses a lock, so a key is a bonus and need not be reachable | Tell a dead-end room from the route, from the room links in page 103 | Locks can never gate progress |
| B. Key placed on the free side | Lock a door, flood from the arrival stairs without crossing it, and place the key in what was reached | A flood fill at generation; two arrival points | More code in the level page. A level reached by falling arrives anywhere (`LEVEL_ARRIVE_ANY`) |
| C. Picks as the fallback | Locks anywhere, and something always opens them: picks, force, a spell | Needs a way through for a player with neither | Bad luck can still end a game |

Whichever is chosen must also handle: a monster picking up the key (`opt_pickup()`), a key
left on another level, and keys that `gen_item_random()` makes with no lock to match.

**Q2 Is a cursed item acceptable before `OP_UNCURSE` is built?** Answered: no, the op was
built in step 11.
