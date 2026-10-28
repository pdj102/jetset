# Component tables: requirements and design

Status: implemented; the CSpect checks in section 7 passed (2026-10-28).
D1 to D8 agreed as proposed (2026-10-28), which answers Q1 and Q2.
Section 10 lists where the build differs from the design.

The component mask, `nge_component_mask_t` in
[ecs/nge_entity.h](../src/nge/ecs/nge_entity.h), is a `uint16_t` per
entity: one bit per component. 15 of the 16 bits are taken (nge's 4, the
game's 11), and the game needs 6 to 8 more. This step replaces the one
16-bit mask with three byte tables, one bit per component, for 24
components, and makes every component test a single byte test.

## 1. Goal

- Room for 24 components: the 15 there are and up to 9 more.
- Component tests, adds and removes no slower than now, and faster where
  possible: per-frame code tests components (G-ARCH, docs/memory_map.md
  "Where component data goes").
- Little more slot 0 memory.

## 2. Terms

| Term | Meaning |
|---|---|
| Component number | 0 to `NGE_COMPONENT_MAX`-1, as now (ecs/components/nge_components.h) |
| Component table | One byte per entity, `[NGE_ENTITY_MAX]`: the bits of 8 components. Table t holds numbers 8t to 8t+7 |
| Component handle | A `uint16_t` naming one component: the high byte its table, the low byte its bit. What `NGE_COMPONENT(n)` gives, and what the `COMPONENT_*` and `NGE_COMPONENT_*` names are |

## 3. Requirements

### Capacity and cost

- **R1** Up to 24 components: `NGE_COMPONENT_MAX` 24. The compiler still
  stops the game going past it (components.h).
- **R2** Testing, adding or removing one component reads or writes one
  byte: no 16- or 32-bit arithmetic.
- **R3** A component named by a constant is tested at a fixed address plus
  the entity ID: no table arithmetic at run time.
- **R4** A component handle fits where a mask is kept now: a struct field
  (`effects_attr_t.component`) or a parameter (`nge_location_find()`),
  16 bits.
- **R5** The slot 0 tables grow by `NGE_ENTITY_MAX` bytes (100), not more.

### The API

- **R6** A handle names one component. Code that needs two tests them
  one at a time (D3).
- **R7** Unchecked macros, for per-frame code: test, add and remove one
  component. **New:** add and remove are macros, used by the components'
  add and remove functions instead of writing `nge_entity_components[]`
  directly. *Reason: the tables' layout is nge's; with three tables, code
  outside nge_entity.h shouldn't have to know which one.*
- **R8** Checked functions, as now: set, clear and test one component,
  asserting on the entity's state and that the handle is one component.
- **R9** The `<NAME>_IS()` macros and the component numbers stay as they
  are; only what they expand to changes.

### Out of scope

- **R10 (later)** More than 24 components: a fourth table
  (`NGE_COMPONENT_TABLES` 4), with no change to the handle or the API.
- **R11** The entity flags and lifecycle: unchanged.

## 4. Decisions

- **D1 One two-dimensional array: `uint8_t
  nge_entity_components[NGE_COMPONENT_TABLES][NGE_ENTITY_MAX]`,** in slot
  0 (`nge_entity_cdata.c`), `NGE_COMPONENT_TABLES` 3. With a constant
  handle, `nge_entity_components[c >> 8][id]` folds to one address plus
  `id` (R3); with a handle in a variable, the table offset is one `mlt`
  (zsdcc uses the Next's `MLT DE`). Measured with zsdcc `-SO3`:

  | Pattern | uint16 (now) | uint32 | Byte tables |
  |---|---|---|---|
  | Test one component | 17-18 | 23-25 | 16 |
  | Add / remove one | 18 | 28 | 10 |
  | Test a handle in a variable | 27 | 44 | 22 |

  (instructions per test function, with its frame.) **Rejected:**
  - `uint32_t` mask: 40-75% more code at every test, multi-bit tests copy
    the long to the stack (`ldir`), 200 more bytes in slot 0, and masks in
    structs and parameters double; the fourth byte would go unused.
  - Three arrays by name (`nge_entity_components0[]`...): a handle in a
    variable would need a pointer table or a switch to find its array.
  - Keeping the uint16 mask and adding one byte table for the new
    components: two kinds of component, the old ones still on 16-bit code,
    and the same work as this later to unify them.

- **D2 The handle: `typedef uint16_t nge_component_t`, `NGE_COMPONENT(n)`
  = `((n) >> 3) << 8 | 1 << ((n) & 7)`.** It replaces
  `nge_component_mask_t`: the type is renamed because a handle is one
  component, not a mask of any of them (R6), and the rename makes the
  compiler find every place that kept a mask. 0 means no component (used
  by `nge_location_find()`, D6). **Rejected:** the component number
  (`uint8_t`): smaller, but a constant test would need the compiler to
  fold `n >> 3` and `1 << (n & 7)` in every macro, and a variable one
  costs a shift loop for the bit.

- **D3 One component per handle and per test (R6).** OR-ing two handles
  is only right when both are in the same table; from different tables
  it tests the wrong table, silently (0 | 1 = table 1). Only three places
  test two components at once, and none of them per frame:
  `anim_bank.c` (`LOCATION|SPRITE`), an assert in `pc.c`
  (`HEALTH|STATS`) and `nge_location_find()` (`mask | LOCATION`). They
  become two tests. The checked functions assert one bit in the low byte
  and a table below `NGE_COMPONENT_TABLES`. **Rejected:** allowing OR
  within a table: a rule about which numbers share a table that every
  caller must keep, for one saved byte load at three sites.

- **D4 The macros, in ecs/nge_entity.h:**

  | Now | Becomes |
  |---|---|
  | `NGE_ENTITY_HAS_COMPONENTS(id, m)` | `NGE_ENTITY_HAS_COMPONENT(id, c)` |
  | `NGE_ENTITY_HAS_ANY_COMPONENTS(id, m)` | `NGE_ENTITY_HAS_COMPONENT(id, c)` |
  | `nge_entity_components[id] \|= m` | `NGE_ENTITY_ADD_COMPONENT(id, c)` (New, R7) |
  | `nge_entity_components[id] &= ~m` | `NGE_ENTITY_REMOVE_COMPONENT(id, c)` (New, R7) |
  | `nge_entity_set_components()` and so on | `nge_entity_set_component()`, `_clear_component()`, `_has_component()` |

  With one component, "all" and "any" are the same test, so there is one
  macro. The old names are removed, not kept as aliases, so the build
  finds every caller (about 160 lines in 56 files; most are the
  `<NAME>_IS()` macros and the components' add functions). **Rejected:**
  keeping the old names: a `|` inside one of them would still compile.

- **D5 The component numbers stay as they are.** Table 0: nge's 4 and
  kind, stats, item, inventory; table 1: health to pc (8 to 14) and one
  free; table 2: the new ones. With one component per test (D3), which
  table a component is in doesn't matter to its callers.

- **D6 `nge_location_find()` takes a `nge_component_t`, 0 for any
  entity,** and tests the location component separately:
  `NGE_ENTITY_HAS_COMPONENT(id, NGE_COMPONENT_LOCATION) && (c == 0 ||
  NGE_ENTITY_HAS_COMPONENT(id, c))`. Its four callers already pass one
  component or 0.

- **D7 `effects_attr_t.component` becomes `nge_component_t`:** the same
  size; each attribute already names one component.

- **D8 The lifecycle clears every table:** `nge_entity_init()`, create and
  release set the entity's byte in each of the three tables to 0, in a
  loop over `NGE_COMPONENT_TABLES` (nge_entity_life.c, three places).

## 5. Design

### 5.1 ecs/nge_entity.h

```c
#ifndef NGE_COMPONENT_TABLES
/** Component tables, 8 components each: NGE_COMPONENT_MAX is 8 times this. */
#define NGE_COMPONENT_TABLES  3
#endif

typedef uint16_t nge_component_t;   /**< One component: table (high byte), bit (low byte); 0 for none. */

#define NGE_COMPONENT(n)  ((nge_component_t)(((nge_component_t)((n) >> 3) << 8) | ((nge_component_t)1 << ((n) & 7))))

extern uint8_t nge_entity_components[NGE_COMPONENT_TABLES][NGE_ENTITY_MAX];

#define NGE_ENTITY_COMPONENT_BYTE(id, c)  (nge_entity_components[(c) >> 8][id])

#define NGE_ENTITY_HAS_COMPONENT(id, c)     ((NGE_ENTITY_COMPONENT_BYTE(id, c) & (uint8_t)(c)) != 0)
#define NGE_ENTITY_ADD_COMPONENT(id, c)     (NGE_ENTITY_COMPONENT_BYTE(id, c) |= (uint8_t)(c))
#define NGE_ENTITY_REMOVE_COMPONENT(id, c)  (NGE_ENTITY_COMPONENT_BYTE(id, c) &= (uint8_t)~(c))

void    nge_entity_set_component(nge_entity_id_t id, nge_component_t c);
void    nge_entity_clear_component(nge_entity_id_t id, nge_component_t c);
uint8_t nge_entity_has_component(nge_entity_id_t id, nge_component_t c);
```

### 5.2 ecs/components/nge_components.h

`NGE_COMPONENT_MAX` becomes `NGE_COMPONENT_TABLES * 8`; the check that
every number has a bit compares it with the tables instead of
`sizeof(nge_component_mask_t)`. The file comment describes the tables and
the handle.

### 5.3 Callers

- The `<NAME>_IS()` macros and the other tests: renamed (D4).
- The components' add and remove functions: the new macros.
- `anim_bank.c`, `pc.c`, `nge_location.c`: two tests (D3, D6).
- `effects_core.h`/`.c`: the type (D7).
- The comment in game/components/components.h: unchanged rules, the
  handle named.

### 5.4 Memory

Slot 0 (page 50): `nge_entity_components` from 200 to 300 bytes at
`NGE_ENTITY_MAX` 100. Slot 0's ECS tables are about 3.4K of `$0100`-`$1FFF`.
Code: smaller, as every test, add and remove is shorter (D1).

## 6. Scope

- **In:** the tables, the handle, the macros and functions, every caller,
  the tests, the docs that name the mask (nge_components.h, components.h,
  docs/memory_map.md; docs/player_character.md D3's "the mask becomes 32
  bits" gets a note pointing here).
- **Later:** a fourth table (R10); the new components themselves.

## 7. Tests

PC tests:

- **`test_entity`:** the model keeps three bytes per entity; set, clear
  and test a component in each table (numbers 0, 7, 8, 15, 16, 23); the
  others in the same byte untouched; all cleared on create and release;
  the checked functions halt (`TEST_EXPECT_HALT`) on a handle of two bits,
  a table of 3 or more, and 0.
- **`test_location`:** `nge_location_find()` with 0 and with a component
  in another table from the location's.
- **`test_gen`:** every component's handle has one bit and no two are the
  same (replacing the mask check).
- The rest unchanged, built against the new names.

CSpect checks:

1. A new game: the player, monsters, items and features shown as before;
   moving, opening a door, picking up, attacking and the effects behave as
   before.
2. The player character screen shows the effects as before.

## 8. Risks

- zsdcc miscompiles (see the memory notes): read the asm of
  `NGE_ENTITY_HAS_COMPONENT` with a constant and with a variable handle,
  and of `nge_location_find()`, after building.
- A missed `|` between handles in a caller: the rename (D4) and the
  checked functions' asserts (D3) catch most; review the diff for `|`
  next to a `COMPONENT_` name.

## 9. Questions

Q1 and Q2 are answered by D4 and D3 (agreed 2026-10-28): the type and
macros renamed; one component per test.

## 10. As built: differences from the design

- **The handle's shifts are unsigned** (5.1). zsdcc types the constant
  `1 << 7` as a signed char, -128, so component 7 (inventory) had the
  handle `0xFF80`, table 255: zsdcc warned "index 255 is outside of the
  array bounds" at every `INV_HAS()`. gcc, and so the PC tests, give
  `0x0080`. `NGE_COMPONENT(n)` casts both halves to `nge_component_t`
  before shifting.
- **The byte macro is `NGE_ENTITY_COMPONENT_BYTE()`,** not
  `NGE_COMPONENT_BYTE()` (5.1): the entity module's prefix.
- **Four places in src/ tested two components, not three** (D3):
  `nge_render.c`'s `drawn()` too, with `LOCATION|SPRITE`, the same tables.
  Tests did so in six more places (test_block, test_feature, test_gen,
  test_level), all now single tests.
- **nge_components.h's check** (5.2): `NGE_COMPONENT_MAX` is defined from
  the tables, so the old check had nothing left to compare; the new one
  checks that the table count fits the handle's high byte.
- **test_location:** `COMP_ITEM` is in table 2, so the randomised search
  covers a component in another table from the location's (section 7);
  `put()` sets no component for 0; the check of `COMP_ITEM |
  COMP_CREATURE` was removed (an OR of handles, D3).
- **The asm** (section 8): read for `inv_add()` (component 7: `set 7,(hl)`
  on table 0), `nge_location_find()` (the variable handle's table offset
  worked out with `MLT DE` once, before the loop) and `derive()` in
  effects_core.c (`effects_attr_t.component`, `MLT DE`). All correct.
- zsdcc notes "conditional flow changed by optimizer" at
  `attack_add()` and `attack_remove()`: `ATTACK_COMPONENT(use)` chooses
  between melee and ranged, which are in the same table, so the table
  part of the ternary folds away. Harmless.
