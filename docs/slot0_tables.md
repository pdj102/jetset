# Slot 0 tables: making room for more entities

Status: implemented (2026-11-09), all four steps; `make` and `make test`
pass. CSpect checks (section 7) passed (2026-11-09). Hardware timing not
yet run (todo P1.1).

## 1. Goal

`NGE_ENTITY_MAX` is limited by page 50, the ECS data page in slot 0, not
by the compile-time cap of 250. At 67 bytes per entity plus 19 fixed bytes,
page 50 holds at most **118** entities (7,925 of its 7,936 bytes). This
spec lists the per-entity tables in page 50, says which can leave it and
where they go, and plans the moves in steps.

Figures are from `bin/main.map` of 2026-11-09 (`__PAGE_50_size` = `$1A3F`
= 6,719 bytes at 100 entities).

## 2. What is in page 50

Bytes per entity, with who uses each table. "Pages" are the code pages
that touch the table directly (110 is the game components' functions).

| Tables | Bytes | Used by | Verdict |
|---|---|---|---|
| entity flags, components ×3, used list | 5 | everything, through common macros, every frame | stays: hot |
| owner | 1 | common macros, every frame | stays: hot |
| location x, y, fx, fy | 4 | common macros, render, every frame | stays: hot |
| block of, pass | 2 | common `nge_block.c`, movement and line tests | stays: hot |
| timer count, period, cycles, kind, payload ×2 | 6 | timer system every tick, effects, the PC screen | stays: the clock and the saved state (memory_map.md) |
| kind class, index | 2 | `KIND_IS()` and `kind_index` in 20 files in 9 pages | stays: shared |
| feature state | 1 | `FEATURE_IS_*()` macros in 10 files in 6 pages | stays: shared, and only 1 byte |
| item slot, count | 2 | actions, UI, AI, effects, attrs | stays: shared |
| stats speed, abilities ×6, status | 8 | attrs writes; actions, AI, effects, UI, player read | stays: shared |
| health hp, hp_max, ac | 3 | attrs writes; actions, AI, player, UI read | stays: shared |
| health immune, resist, vuln | 3 | attrs (138) writes, health damage (110) reads, PC screen | stays: writes would cross pages for 3 bytes |
| mana mp, mp_max | 2 | attrs, player, the spell list, PC screen | stays: shared |
| **attribute bases** (health ×5, stats speed + abilities ×6, mana mp_max) | **13** | attrs (138); set once in the `*_add()` functions (110); read by `pc.c` (110) and the PC screen (104) | **moves to page 139** (step 1) |
| **attack** to_hit, dice, sides, to_damage, type, range ×2 uses | **12** | `attack.c` (110); read by the PC screen (104) and 3 actions (112) | **moves to page 111** (step 2) |
| **inventory** capacity, slots (`uint16_t`) | **3** | `inventory.c` (110); read by 2 actions (112), the inventory screen (116), the AI (130) | **may move to page 111** (step 3, optional) |
| **Total** | **67** | | |

The three moving groups are each read every turn at most, never every
frame, and each has one page that owns them. The pages they go to are
empty today: 111 (`gen` shared's odd page), 139 (attrs' odd page).

### What each step buys

| After | Bytes per entity | Most entities (`(7936 - 19) / bytes`) |
|---|---|---|
| now | 67 | 118 |
| step 1: bases out | 54 | 146 |
| step 2: attacks out | 42 | 188 |
| step 3: inventory out | 39 | 203 |

The new homes stay small even at the cap of 250: page 139 needs 3,250
bytes, page 111 needs 3,750. The other per-entity pages are not limits
below 250 except the render system: page 53 holds 24 bytes per entity plus
330 fixed bytes, at most 327 entities.

**A second limit: owner IDs.** An owner is one byte holding an entity ID, the
world (`NGE_ENTITY_MAX`) or an area (`NGE_ENTITY_MAX + 1 + a`).
`nge_owner.c` checks at compile time that `NGE_ENTITY_MAX + 1 +
NGE_OWNER_AREA_MAX <= 255`. With `NGE_OWNER_AREA_MAX` at 64, that caps
entities at **190**, below step 3's 203. The game uses 11 areas (depths 0
to `LEVEL_DEPTH_BOTTOM` 10). 150 is within both limits (D9).

### At `NGE_ENTITY_MAX` 150, after all three steps

| Page | Contents | Bytes | Free |
|---|---|---|---|
| 50 | slot 0 tables, 39 per entity + 19 | 5,869 of 7,936 | 2,067: 13 bytes per entity for future hot components |
| 49 | lifecycle stacks, 3 per entity + 2 | 452 | 7.7K |
| 53 | render, 24 per entity + 330 | 3,930 | 4.2K |
| 111 | attack and inventory, 15 per entity | 2,250 | 5.9K |
| 131 | AI, 6 per entity | 900 | 7.3K |
| 135 | spells, 4 per entity + 198 | 798 | 7.4K |
| 139 | attribute bases, 13 per entity | 1,950 | 6.2K |
| owners | 150 + 1 + 64 | 215 of 255 | |

## 3. Requirements

- **R1** Page 50 holds only tables that are hot (read per frame, or by
  common code) or shared by several pages.
- **R2** A moved table lives in the odd page of the one module that owns
  it, as page data (`*_pdata.c`, `nge-dataseg: PAGE_n`).
- **R3** Code outside the owning page reaches a moved table only through a
  banked function of the owning module, never by mapping its page by hand.
- **R4** No moved table is read through a banked call per entity per frame.
- **R5** The game plays the same: the PC tests pass unchanged except where
  they read a moved table directly, and the CSpect checks in section 7 pass.
- **R6** No new routine in common memory (no new `__*` or `asm_*` symbol);
  "bytes free below the stack" does not drop by more than the new banked
  stubs, a few bytes each.
- **R7** *(Changed, 2026-11-09: the user asked for it in this work.)* The
  last step raises `NGE_ENTITY_MAX` from 100 to 150, leaving room in page
  50 for future hot components.
- **R8** *(New, 2026-11-09.)* Page 50 can hold as many entities as possible,
  close to 200: all three moves are built, including the inventory.
- **R9** *(New.)* At 150, a turn and a frame take no visibly longer than at
  100. Some loops run over every ID, not only the ones in use (section 8),
  so they cost 1.5 times as much.

## 4. Decisions

- **D1 Move the bases to page 139, the attrs page's data page.** The attrs
  module is the one owner: it reads them to derive values and is the only
  code that changes them after creation (`effects_set_base()`,
  `effects_raise_base()`). Its constant `attrs_table[]` keeps pointing at
  the tables; only the address changes. `mods_of()` already maps page 121
  into slot 7 and back, so the bases are mapped again before the next read.
  - Rejected: page 111 with the component functions. Then the attrs module
    (138) would need a banked call for every base it reads.
- **D2 Creation sets bases through a new banked `effects_put_base(id,
  attr, value)`** in page 138, which stores without recomputing: the `*_add()`
  functions already set the derived values themselves. `stats_add()` calls
  it 7 times, once per entity created: a few thousand T-states at creation,
  not per frame.
  - Rejected: `effects_set_base()`. It recomputes all attributes each time:
    13 recomputations for a creature.
  - Rejected: one batch call taking an array. Saves little and adds a
    calling convention for 3 callers.
- **D3 Code outside page 138 reads a base through a new banked
  `effects_base(id, attr)`.** Callers: `pc.c` (levelling up, 2 reads) and the
  PC screen (13 reads per draw, when the screen opens).
- **D4 Move the attack tables to page 111, the data page of the component
  functions (110).** `attack_add()`, `attack_remove()`, `attack_hits()`,
  `attack_damage()` already run there.
- **D5 Code outside page 110 reads an attack through a new banked
  `attack_get(id, use, attack_kind_t *out)`,** which copies the whole profile
  into the caller's struct, which is on its stack. `attack_kind_t` already
  has the six fields. Callers: the PC screen, `act_shared.c` (type),
  `act_melee.c` and `act_fire.c` (range).
  - Rejected: one getter per field (`attack_range_of()`, ...). It needs more
    stubs, and the PC screen would make 5 calls.
- **D6 Step 3 (inventory) is built** *(Changed, 2026-11-09: it was optional;
  R8 asks for the most room).* It gains 15 entities for 4 callers in 3
  pages. The AI's think (130) makes one banked call per AI per turn, which
  is acceptable.
- **D7 Keep the PC test layout:** the moved tables stay ordinary globals
  there (`nge-dataseg` has no effect on the PC). Only the headers that
  declare them change.
- **D8 One step, one commit,** each leaving the game playable, as in
  earlier plans.
- **D9 Leave `NGE_OWNER_AREA_MAX` at 64.** At 150 entities, owners use 215
  of 255 values. Going past 190 entities would need fewer areas: the game
  uses 11, so 32 would allow up to 222. That change is not needed now.
  - Rejected: lowering it now. Nothing needs it below 190 entities.
- **D10 Raise `NGE_ENTITY_MAX` in a step of its own (step 4),** after the
  moves, so that a fault in a move is not mixed up with one from the
  larger count. Change the default in `nge_entity.h` from 100 to 150. The
  PC tests use the same header, so they run at 150 too.

## 5. Design

### 5.1 Step 1: attribute bases to page 139

- New `game/attrs/attrs_pdata.c` (`/* nge-dataseg: PAGE_139 */`): the 13
  tables, moved from `health_cdata.c`, `stats_cdata.c` and `mana_cdata.c`.
- Their `extern`s move from `health.h`, `stats.h`, `mana.h` to
  `attrs_internal.h`.
- `attrs.h` gains:

  ```c
  /** @brief Attribute attr's base for id, which has it (asserted). */
  uint8_t effects_base(nge_entity_id_t id, uint8_t attr) NGE_BANKED;
  /** @brief Store attr's base for id without recomputing: for the components' add functions. */
  void effects_put_base(nge_entity_id_t id, uint8_t attr, uint8_t value) NGE_BANKED;
  ```

- `health_add()`, `stats_add()`, `mana_add()` (page 110): set bases with
  `effects_put_base()`.
- `pc.c`: `effects_base(id, ATTR_HP_MAX)`, `effects_base(id, ATTR_MP_MAX)`.
- `screen_pc_bank.c` (page 104): `effects_base()` for the bases shown.
- `memory_map.md`: page 139 is no longer unused. Its row gets "the
  attribute bases (`attrs_pdata.c`, 13 bytes per entity)", and the note
  that page 121 is mapped into slot 7 *over* them while copying, and back.
- `attributes.md`: where the bases live.

### 5.2 Step 2: attack tables to page 111

- New `game/components/attack_pdata.c` (`/* nge-dataseg: PAGE_111 */`): the
  6 tables from `attack_cdata.c`, which is deleted.
- Their `extern`s move from `attack.h` into `attack.c` (or a small
  `attack_internal.h` if the tests need them).
- `attack.h` gains:

  ```c
  /** @brief Copy id's attack for use into out; id has the use's component (asserted). */
  void attack_get(nge_entity_id_t id, uint8_t use, attack_kind_t *out) NGE_BANKED;
  ```

- Callers change to one `attack_get()` into a local `attack_kind_t`:
  `screen_pc_bank.c`, `act_shared.c`, `act_melee.c`, `act_fire.c`.
- `memory_map.md`: page 111 is no longer unused ("the attack component's
  tables, 12 bytes per entity").
- Watch: page 110's code is not near 8K today, but the new function adds to
  it. The map shows the step's cost.

### 5.3 Step 3: inventory to page 111

- `inv_capacity`, `inv_slots` to `game/components/inventory_pdata.c`
  (page 111); new banked `inv_capacity_of(id)` and `inv_slots_of(id)`.
- Callers: `act_pickup.c`, `act_equip.c` (3 reads), `screen_inv_bank.c`
  (read once per draw into a local, not per item), `ai_bank.c` (once per
  think).

### 5.4 Step 4: `NGE_ENTITY_MAX` 150

- `nge_entity.h`: the default becomes 150. Its comment says the real limits:
  page 50 (about 203 at 39 bytes per entity) and owners (190 with 64
  areas), as well as the compile-time cap of 250.
- `memory_map.md`: replace "about 6.6K at `NGE_ENTITY_MAX` 100" with page 50's
  new size. Update the sizes of the page data that grows: 49, 53, 111, 131,
  135, 139.
- `ecs.md`, if it gives the entity count.
- Nothing else depends on the value: the IDs are `uint8_t`, and every loop
  over IDs uses a `uint8_t` counter, which is safe below 255.

### 5.5 What is unchanged

- Every table marked "stays" in section 2, and `NGE_OWNER_AREA_MAX` (D9).
- `scripts/check-banks.sh` already checks `*_pdata.c` files are in an odd
  page and `*_cdata.c` files in page 50: no change.

## 6. Steps

Each step: `make`, `make test`, compare `__PAGE_50_size`, `__PAGE_110_size`
and "bytes free below the stack" against the previous build, and check for
new `__*` or `asm_*` symbols in common memory.

| Step | Change | Page 50 at 100 entities | Limit |
|---|---|---|---|
| 1 | bases to page 139 (5.1) | 5,419 bytes | 146 |
| 2 | attacks to page 111 (5.2) | 4,219 bytes | 188 |
| 3 | inventory to page 111 (5.3) | 3,919 bytes | 190 (owners; page 50 alone: 203) |
| 4 | `NGE_ENTITY_MAX` 150 (5.4) | 5,869 bytes at 150 | 190 |

## 7. Tests and checks

**PC tests** (`make test`):
- Nine test files read a moving table directly: `test_health`,
  `test_stats`, `test_mana`, `test_pc`, `test_effects`, `test_chargen`,
  `test_gen`, `test_ui` (bases or attacks) and `test_attack`. They include
  the internal header that now declares the table, and should otherwise
  pass unchanged (D7).
- New: `effects_put_base()` stores without recomputing (the derived value
  is unchanged); `effects_base()` returns what was put.
- New: `attack_get()` returns what `attack_add()` stored, for both uses.
- `test_attack`, `test_act`: unchanged results.

**CSpect checks** (the user runs them):
1. Start a game as each class: the PC screen's bases and values, and the
   melee line, match the build before the change.
2. Drink a potion that raises a base: the base and the value change.
3. Level up: HP and MP maximums rise as before.
4. Fight in melee and fire a ranged weapon: range limits, hits and damage
   types as before.
5. Play through two levels with creatures, traps and effects: no assert,
   and the guard check stays quiet.
6. After step 4: play several levels down and back up, so that parked
   entities build up in their areas. No "no free entity" failure, and turns
   take no visibly longer than before (R9).

**Hardware:** steps 1-3 add nothing to time, since the moved reads happen
once per turn or once per screen draw. After step 4, time a monster turn
and an idle frame against the step 3 build (R9). Hardware runs at about
0.8 × CSpect's time.

## 8. Risks

- **A read of a moved table from the wrong page** compiles and links (it
  is only an address) but reads whatever is in slot 7. Mitigation: the
  `extern`s move to internal headers, so other modules can't see them.
  Also grep for each table name after each step.
- **zsdcc and struct copies** (memory note on HL reuse): `attack_get()`
  writes six fields through a pointer. Read its asm once.
- **Page 121 over page 139 in `mods_of()`**: a future change that reads a
  base while page 121 is mapped would read the effect definitions instead.
  The comment in `mods_of()` must say so.

- **Loops over every ID cost 1.5 times as much at 150** (R9). They run
  whether or not the IDs are in use:
  - the timer system's three loops (`nge_timer.c`), on every tick;
  - `nge_location_find()`'s `memchr` over the location table;
  - `inv_count()`, `inv_next()` and `inv_drop_all()` (`inventory.c`);
  - `act_pickup.c`, `player_bank.c` and the owner searches
    (`nge_owner_set.c`).

  If the timing check fails, the timer loops are the first to change: walk
  the used list instead (`NGE_ENTITY_FOR_EACH_IN_USE`). That would be a
  separate change.

## 9. Questions answered (2026-11-09)

- **Q1** What entity count does the game need? As close to 200 as
  possible: build all three moves (R8, D6). The owner limit (190 with 64
  areas, D9) is then the binding one.
- **Q2** Should raising `NGE_ENTITY_MAX` be part of this work? Yes, to
  150, which leaves headroom (R7, D10, step 4).

## 10. As built (2026-11-09)

Page sizes from `bin/main.map` matched section 6 at each step: page 50 was
5,419, 4,219 and 3,919 bytes at 100 entities, and 5,869 at 150. At 150,
pages 49, 53, 111, 131, 135 and 139 are 452, 3,930, 2,250, 900, 798 and
1,950 bytes, as in section 2. "Bytes free below the stack" stayed at 4,289
throughout, and no `__*` or `asm_*` symbol was added (R6). Page 110's code
went from 7,315 to 7,677 bytes: 515 bytes are left before page 111's data.

Differences from the design:

- **The `*_add()` functions add their component before the bases.**
  `effects_put_base()` asserts the attribute's component, as
  `effects_base()` does, so `health_add()`, `stats_add()` and `mana_add()`
  now call `NGE_ENTITY_ADD_COMPONENT()` before storing the bases.
  `health_add()` makes 5 calls, `stats_add()` 7, `mana_add()` 1.
- **The bases' `extern`s are in `attrs_internal.h`**, which now includes
  `stats.h` for `ABIL_COUNT`.
- **`attack_get()` reads through one flat offset.** `[use][id]` six times
  cost zsdcc about 250 bytes of page 110; casting each table to
  `const uint8_t *` and computing `use * NGE_ENTITY_MAX + id` once brought it
  to about 170 bytes plus the assert's text. The asm writes the six fields
  through `out` correctly (risk in section 8).
- **The attack and inventory `extern`s are in new internal headers**,
  `components/attack_internal.h` and `components/inventory_internal.h`, as
  the tests need them (section 5.2 allowed either).
- **`inv_capacity_of()` and `inv_slots_of()` return 0 for an entity with
  no inventory** instead of asserting: `act_unequip()` reads the capacity
  without checking `INV_HAS()` first. The old direct reads returned
  whatever was left in the table.
- **The inventory screen passes the slots to `listed()`**, read once in
  `collect_stowed()`, not once per item. `fitting()` still calls
  `inv_slots_of()` each time, but it runs once per action. `ai_can()` reads
  the slots once into a local.
- **`act_fire()` reads the range into its `ammo` struct**, which is set
  again before the strike.
- **Tests:** `test_effects` (the getter and put, without and then with a
  recompute), `test_attack` (`attack_get()` for both uses),
  `test_inventory` (the two getters, and 0 without an inventory). Ten test
  files include an internal header now: section 7's nine, plus
  `test_inventory` and `test_act` for the inventory tables.
- **Docs:** `memory_map.md` (pages 111 and 139, page 50's contents and size,
  and the page data that grows with the entity count), `attributes.md`
  (where the bases are), `ecs.md` (the entity count and owner values), and
  the comment on `NGE_ENTITY_MAX` in `nge_entity.h`.
