# Armour with a property: implementation plan

The steps that build I2 ([todo.md](todo.md)) as [items.md](items.md) specifies it:
R21 to R30, sections 4.7 and 4.11, D14 to D19. The spec says what and why; this file
says in what order, what changes, and how each step is checked.

Status: planned 2026-11-12.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game playable.
- **Every step:** `make`, `make test`, and a look at the map for what the step cost.
  A new `__*` or `asm_*` symbol in common memory is a library routine linked by mistake.
- **The CSpect check closes a step.** The user runs it, and the next step waits for
  the result.
- **After each step:** anything that differs from the spec goes in an "As built" note
  in items.md section 4.11.

```text
 1 the data union and its readers --> 2 AC in the record (I2.1) --> 3 ego armour (I2.2)
                                                                     |
                                                               4 closing docs
```

Step 1 is a refactor with no change to play or to page 109's bytes, so it can be
checked by comparing the page before and after. Step 2 moves armour's AC without
changing play, so a mistake shows as an AC that differs from before. Step 3 is data
only.

## 2. Steps

### Step 1: the data union and its readers

R29, R30, D16, D19.

- **Before:** note pages 108, 109 and 112/113 and common memory free, from the map;
  keep a copy of `bin/main_PAGE_109.bin`.
- **Change:**
  - `gen_internal.h`: `typedef union gen_item_data { uint8_t ammo_group; uint8_t
    key_lock_group; uint8_t pick_bonus; uint8_t book_spells; uint8_t armour_ac; }
    gen_item_data_t;`, and the record's `group` becomes `gen_item_data_t data`,
    each member's comment naming its subclasses.
  - `gen_item_pdata.c`: every `.group = X` becomes the member for its subclass
    (`.data.ammo_group`, `.data.key_lock_group`, `.data.pick_bonus`,
    `.data.book_spells`): 32 rows. Armour comes in step 2.
  - `gen.h`, `gen_item_bank.c` (page 108): `gen_item_group()` goes; the five
    readers of items.md 4.7, each checking the subclass. `gen_item_armour_ac()`
    is written here too, returning 0 until step 2 sets the data.
  - Callers: `act_fire.c` (2) and `anim_vfx.c` call `gen_item_ammo_group()`;
    `feature.c` `gen_item_key_lock_group()` and `gen_item_pick_bonus()`;
    `act_learn.c` and `screen_spell_bank.c` `gen_item_book_spells()`.
- **Tests:** `test_gen`: `item_kind_ok()` reads the members; the accessor check
  (line 340) compares each reader with its member, and the other-subclass value
  for every kind; `test_level` (3 places) and the book checks use the new
  readers.
- **Check without CSpect:** `bin/main_PAGE_109.bin` is byte for byte the same as
  before, and the build gives no "excess elements" warning. That proves the union's
  initialisers compiled right (D19).
- **Memory:** page 108 grows by about 60 bytes (five readers for one); the callers
  stay the same size.
- **CSpect (check 1):** a Fighter fires nothing, so a Dev character: fire the short
  bow (arrows fly with their look), throw a dart; lock the nearest door with the dev lock key,
  pick a lock with the lock pick; read the basic book (`b`) and see its spells.

### Step 2: AC in the record

R21 to R26, D14, D15, D17. I2.1.

- **Change:**
  - `gen_item_pdata.c`: the 24 armour kinds with `EF_WORN_AC` get
    `.data.armour_ac = power` and lose their hook; gauntlets of might, boots of
    speed, boots of levitation and the fireproof cloak gain the AC of 4.11's
    table. The ring of armour and the amulet of guarding are unchanged.
  - `attrs_bank.c` (page 138), `derive()`: for an equipped item, add
    `gen_item_armour_ac(kind_index[owned])` to `d->value[ATTR_AC]` beside its
    hook. The kind index in a local before the call (T-ZSDCC-1).
  - `screen_inv_bank.c` (page 116): "AC n" from `gen_item_armour_ac()` when not 0,
    else from an `EF_WORN_AC` hook.
  - `effects.h`, `effects.md`: `EF_WORN_AC`'s comment says it is for jewellery
    and cursed armour.
- **Tests:**
  - `test_gen`: `item_kind_ok()` for armour: `fits` set, and an AC or a worn hook;
    no armour kind has `EF_WORN_AC` on its hook.
  - `test_effects`: plate +2 still adds 9 (the existing check, now through the
    field); gauntlets of might add AC 2 and Str 2, and unequipping removes both.
  - `test_chargen`: the Fighter's AC is still base + 4 (leather 2, shield 2).
  - `test_ui`: the iron helmet's "AC 2" row stays; a ring of armour +1 shows "AC 1".
- **Memory:** page 109 unchanged in size (the hook bytes stay, now `EF_NONE`). The
  call adds about 15 bytes to page 138.
- **Depth:** `effects_recompute()` → `derive()` → `gen_item_armour_ac()`: one banked
  call, as `gen_hook_of()` → `gen_item_hook()` is now, so no new depth.
- **CSpect (check 2):**
  1. A new Fighter: the C screen's AC is the same as before the change (base + 4),
     and the inventory shows "AC 2" on the leather armour and on the shield.
  2. A Dev character: AC base + 9 with plate +2; take the plate off and AC falls
     by 9; put it back.
  3. On a level, walk into a monster's attacks for a few turns: hits land about
     as often as before.

### Step 3: ego armour

R27, I2.2.

- **Change:**
  - `gen_kinds.h`: `IT_MAIL_FIRE_WARD` 208, `IT_DRAGON_MAIL` 209,
    `IT_ELVEN_CLOAK` 210, `IT_SHIELD_WARDING` 211; `IT_COUNT` 212.
  - `gen_item_pdata.c`: the four rows of 4.11's table, in the armour section.
  - `player_bank.c`: the dev G key starts its cycle at `IT_MAIL_FIRE_WARD` for
    this check, and goes back to `IT_SERRATED_SWORD` after it (As built), or the
    cycle start becomes a constant that names the newest kinds.
- **Tests:** `test_gen` (the count, the new rows well formed, the mix by depth
  still within R4); `test_effects`: dragon scale mail gives AC 6 and fire immunity
  together, and taking it off removes both.
- **Memory:** page 109 grows by about 154 bytes, to about 7,757 (435 free). Over
  500 would mean a name or field differs from the spec: check before going on.
- **CSpect (check 3):** a Dev character, G on a level:
  1. The four kinds appear with their names and looks (orange and red mail, a green
     cloak, a purple shield), each different from the older armour.
  2. Wear dragon scale mail: AC up by 6 against no body armour. The dev hurt key
     (3 damage, a new type each press) does nothing when its type is fire.
  3. Wear the elven cloak: Dex +1 and AC +1 on the C screen.
  4. Wear the shield of warding: AC +2; the inventory shows "AC 2".

### Step 4: closing docs

- spells.md (sections 10, D20, the table and tests: `group` becomes
  `data.book_spells`, `gen_item_group()` becomes `gen_item_book_spells()`),
  action_fire.md (`gen_item_ammo_group()`), and any other doc naming
  `gen_item_group()`.
- items.md: section 4.11's As built note; the memory figures in section 8 and
  memory_map.md's page 109 row (7.4K and "22 bytes a kind" checked against the map).
- ui_inventory.md: "AC n" from the record (R26).
- todo.md: I2.1 and I2.2 `[done]` with the date; I2.3 and I2.4 stay (low).
