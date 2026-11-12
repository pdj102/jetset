# Items aimed at a tile or an area: implementation plan

The steps that build I3 ([todo.md](todo.md)) as [items_aimed.md](items_aimed.md)
specifies it. The spec says what and why; this file says in what order, what changes, and
how each step is checked.

Status: done 2026-11-12 (steps 1 to 5; as built in items_aimed.md, "As built"). CSpect check 4's
third item, a ball over 9 or more creatures, wasn't run.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game playable.
- **Every step:** `make`, `make test`, and a look at the map for what the step cost: pages
  108, 109, 116, 124, 126 and `BANK_56`, and common memory. A new `__*` or `asm_*` symbol
  in common memory is a library routine linked by mistake.
- **The CSpect check closes a step.** The user runs it, and the next step waits for the
  result.
- **After each step:** anything that differs from the spec goes in an "As built" section
  at the end of items_aimed.md.

```text
 1 shapes and the helper --> 2 tile items (I3.1) --> 3 balls and bursts (I3.2) --> 4 their VFX
   (a refactor: no change                              |
    to play)                                     5 closing docs
```

Step 1 moves code without changing play, so the existing tests and page 109's bytes prove
it. Steps 2 and 3 each add a shape and its kinds. Step 4 is only what is shown, so step 3
can be played (impacts may show before the bolt lands) before it.

## 2. Steps

### Step 1: shapes and the helper

Spec 4.1 to 4.5 for the existing shapes; R1, R2, R12, R19, D1 to D3.

- **Before:** note the free bytes of the pages above and keep a copy of
  `bin/main_PAGE_109.bin`.
- **Change:**
  - `spells.h`: `SPELL_BALL` 4, `SPELL_BURST` 5, `SPELL_SHAPE_COUNT` 6 (no row uses
    them).
  - `gen_kinds.h`: `ITEMF_SHAPE` 0x1C, `ITEMF_TILE` 0x08, `ITEMF_BALL` 0x0C,
    `ITEMF_BURST` 0x10; `ITEMF_DIRECT` stays 0x04, its comment saying it is one value of
    the field.
  - `gen.h`, `gen_item_bank.c` (page 108): `gen_item_shape(kind)`, the 5-byte table,
    an assert on 5 to 7.
  - `act.h`: `ACT_BALL_RADIUS` 1, `ACT_BURST_RADIUS` 2, `ACT_AREA_MAX` 8.
  - `act_internal.h`, `act_shared.c`: `act_shape_t`, `act_shape_find()` and
    `act_shape_apply()` for self, bolt, direct and tile; the tile code moved from
    `act_cast.c` unchanged. Ball and burst assert for now.
  - `act_cast.c`: its shape branches and its apply call become the helper.
  - `act_aim.c`: the shape from `gen_item_shape()`; the own-tile rule for bolt and
    direct; otherwise the helper. `gen_item_flags()` is no longer called here.
- **Tests:** every existing `test_act` aim and cast test passes unchanged. `test_gen`:
  every kind's shape field is 0 to 4; `gen_item_shape()` gives `SPELL_DIRECT` for exactly
  the `ITEMF_DIRECT` kinds and `SPELL_BOLT` for the other aim kinds.
- **Check without CSpect:** `bin/main_PAGE_109.bin` is byte for byte the same.
- **Memory:** `BANK_56` about the same (code moved, a little added); page 108 grows by
  about 30 bytes.
- **Depth:** the helper is a near call inside `BANK_56`, so `act_aim()` →
  `effect_apply()` is as deep as before.
- **CSpect (check 1):** a Dev character:
  1. Cast Magic Missile at a monster and Blink to a free tile; Blink onto a monster is
     refused with the same message as before the change.
  2. Aim a wand of fire past a monster at one behind: the nearer one is burned.
  3. Aim the staff of healing at yourself: you are healed.

### Step 2: tile items (I3.1)

R3, R13, R15, R20; spec 4.4, 4.7 (the tile purpose), 4.10's first two rows.

- **Change:**
  - `act_aim.c`: nothing more, if step 1 passed the tile shape through; check the
    refusals reach `messages_act_failed()` with the cast's texts.
  - `target.h`, `target_bank.c`: `TARGET_AIM_TILE`, "Aim to where?" and its help;
    `TARGET_PURPOSE_COUNT` 6.
  - `player_bank.c`: `item_command()` reads the shape for the purpose.
  - `screen_inv_bank.c`: `allowed()` passes `ACT_AIM` refused with `ACT_R_NO_TARGET`.
  - `gen_kinds.h`, `gen_item_pdata.c`: `IT_SP_BLINK` 212, `IT_STAFF_BLINK` 213,
    `IT_COUNT` 214 for now.
  - `player_bank.c`: `DEV_ITEM_FIRST` becomes `IT_SP_BLINK`, so the dev G key cycles
    through I3's kinds.
- **Tests:** `test_act`: the tile cases of the spec's section 7. `test_gen`: R20; the
  two rows well formed; R4. `test_ui`: the aim list offers the blink parchment.
- **Memory:** page 109 grows by about 75 bytes (two kinds and their names).
- **CSpect (check 2):** a Dev character, G on a level for the two kinds:
  1. The blink parchment and the staff of blinking appear blue, with their names.
  2. Aim the parchment: the prompt is "Aim to where?"; choose a free tile 5 away: you
     land there, and the parchment is gone.
  3. Aim the staff at a monster's tile, at a tile behind a wall, and at your own tile:
     each refused with the message Blink gives for it, no charge used, no turn passes.
  4. Aim the staff 8 tiles away: it works, and the inventory shows one charge fewer.

### Step 3: balls and bursts (I3.2)

R4 to R11, R13, R14, R17; spec 4.3, 4.4 to 4.7, 4.9, 4.10's last four rows.

- **Change:**
  - `act_shared.c`: `act_area()`; ball and burst in `act_shape_find()` and
    `act_shape_apply()`.
  - `events.h`: `EV_AREA` (`NGE_EVENT_USER + 38`), and its row in the table.
  - `act_aim.c`, `act_cast.c`: `EV_AIMED` and `EV_CAST` with target `NGE_EVENT_NONE`
    for a ball or burst.
  - `anim_vfx.c`: `EV_AIMED` with no target flies nothing (a guard; the VFX come in
    step 4).
  - `target.h`, `target_bank.c`: `target_t.radius`; the cursor's bank (spec 4.7), in a
    function the tests can call.
  - `player_bank.c`: `choose_tile()` takes the radius; `item_command()` and
    `cast_command()` follow the spec's table (a burst asks for nothing).
  - `gen_kinds.h`, `gen_item_pdata.c`: 214 to 217, `IT_COUNT` 218.
  - Check what the death message says when the player kills itself, and make it read
    sensibly if it doesn't (spec 4.9).
- **Tests:** `test_act`: the ball, burst, cap and check-agrees cases of the spec's
  section 7, and the cast tests unchanged. `test_target`: the cursor's banks.
  `test_gen`: the four rows, R4. `test_ui`: the panic parchment goes without a prompt.
- **Memory:** page 109 grows by about 165 bytes, to about 195 free (spec section 8). Over
  300 for the six kinds means a name or field differs from the spec: check before going on.
  `BANK_56` grows by about 400 bytes.
- **Stack:** `act_shape_t` is 12 bytes on `act_aim()`'s and `act_cast()`'s stacks
  (toolchain.md T-STACK-1).
- **CSpect (check 3):** a Dev character, G on a level for the four kinds. The impacts may
  show before the bolt lands until step 4.
  1. The two wands, the fireball parchment and the panic parchment appear with their names
     and looks.
  2. Aim the wand of fireballs at a monster in a group: each one within 1 of it is burned;
     one round a wall corner from it isn't.
  3. Aim it at an empty tile next to a monster: the monster is burned. Aim it past a nearer
     monster: the nearer one is passed over, unburned.
  4. Move the cursor next to yourself: it turns amber. Aim there with a monster beside
     you: you are burned too ("You are burned!").
  5. Aim at your own tile, and at an empty floor with nobody near: refused, no charge used.
  6. Read the panic parchment with monsters within 2: they are terrified (some may resist)
     and flee; you aren't. With nobody near: refused, the parchment kept.
  7. Cast Magic Missile, Blink, and a self spell: as before.

### Step 4: the area VFX

R16; spec 4.8; vfx.md R20 for balls.

- **Change:** `anim_vfx.c`: the remembered effect and centre; `EV_AREA` flies the bolt
  and bursts at the centre; `damaged()`, `glow()` and `died()` wait for the centre's
  arrival while one is remembered; forgotten on the events the spec lists and in the
  level change's VFX clear.
- **Tests:** `test_anim`: the bolt starts on `EV_AREA`; an impact elsewhere waits for it;
  a later `EV_FIRED` forgets the centre.
- **Memory:** pages 126/127 grow by about 150 bytes.
- **CSpect (check 4):**
  1. The wand of fireballs: an orange bolt flies to the chosen tile, bursts, and then
     every monster burned splashes together, none before the burst.
  2. The wand of frost balls: the same in blue.
  3. A ball over a crowd of 9 or more: some impacts may be missing (the pool), and nothing
     hangs.
  4. The panic parchment: each monster frightened glows, with no bolt.

### Step 5: closing docs

- items_aimed.md: its status, and an "As built" section if anything differed.
- items.md: memory figures in section 8; memory_map.md's page 109 and `BANK_56` rows
  checked against the map.
- action_aim.md, action_cast.md, spells.md, actions.md, targeting.md, vfx.md: their
  "proposed" notes for I3 become "built".
- todo.md: I3.1 and I3.2 `[done]` with the date.
