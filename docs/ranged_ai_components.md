# The ranged attack and AI components: requirements and design

Status: implemented; the CSpect check in section 7 passed (2026-10-28).
D1 to D6 agreed as proposed, which answers Q1 and Q2. Section 10 lists
where the build differs from the design.

Two placeholder game components, so the code that uses them later
(ranged weapons and the actions that fire them; monster AI) starts from
components that already exist, are tested, and have their numbers:

- **Ranged attack:** the attack component's second use, `ATTACK_RANGED`,
  alongside `ATTACK_MELEE` ([game/components/attack.h](../src/game/components/attack.h)).
  `COMPONENT_RANGED` has been reserved since the attack component was
  added (`NGE_COMPONENT_USER + 6`).
- **AI:** marks a creature whose turns are decided by the game, not the
  player: every monster.

Nothing uses either one yet: no action fires a ranged attack, and a
monster's turn still waits (`TIMER_MONSTER`, game/timers.h).

## 1. Goal

- A ranged attack can be given to an entity, read, rolled and removed
  through the attack component's API, as a melee attack is.
- Monsters can be told from the player and other entities by a component,
  `AI_IS(id)`, ready for an AI module to act on.
- Little memory now for what nothing uses yet.

## 2. Terms

| Term | Meaning |
|---|---|
| Use | A row of the attack component's tables (`ATTACK_*`): melee or ranged |
| Placeholder | A component with its number, header, add function and tests, which no kind or system uses yet (ranged), or which marks entities for code to come (AI) |

## 3. Requirements

### Ranged attack

- **R1** `ATTACK_RANGED` is the attack component's second use: the same
  fields as melee (to-hit, dice, sides, bonus, damage type, range), set
  from an `attack_kind_t` by `attack_add()`, removed by `attack_remove()`,
  with `COMPONENT_RANGED`.
- **R2** `attack_hits()` and `attack_damage()` work for it unchanged: one
  hit rule and one damage rule for both uses (G-ARCH-1).
- **R3** A ranged attack's range is 2 or more (a melee attack's is 1).
- **R4** An entity can have both a melee and a ranged attack.
- **R5 (later)** Ranged weapons and ammo (the ammo slot), kind records with
  a ranged attack, the action that fires one, its animation and messages,
  and the player character screen's ranged row.

### AI

- **R6** `COMPONENT_AI` is the next game component number,
  `NGE_COMPONENT_USER + 11` (component 15, the last of table 1).
- **R7** Every creature the generator makes is given it, except the
  player.
- **R8** `AI_IS(id)` tests for it (components.h's model).
- **R9 (later)** The AI itself: what a monster does on its turn, its
  behaviour, target and state, and the module that runs it.

## 4. Decisions

- **D1 Ranged gets its tables now: `ATTACK_USE_COUNT` 2.** The six attack
  tables are `[use][entity]` in slot 0, so a second use adds 6 x 100 = 600
  bytes. Page 50 has about 2.2K free (`__PAGE_50_size` $1657, up to
  `$1FFF`), so 1.6K would be left. **Option (Q1):** keep the number
  reserved, define `ATTACK_RANGED` and the range rule, but leave
  `ATTACK_USE_COUNT` at 1 until ranged weapons come: no memory now, but
  then `attack_add()` asserts on a ranged use and the placeholder can't be
  tested. **Rejected:** ranged tables in a page of their own: the hit and
  damage rules read both uses' tables the same way, `[use][id]`.

- **D2 `attack_add()` asserts the range by use (R3):** 1 for melee, 2 or
  more for ranged. `ATTACK_COMPONENT(use)` already gives
  `COMPONENT_RANGED`.

- **D3 No kind has a ranged attack yet (R5).** *Since superseded: launchers
  and thrown ammo have one (docs/items.md, "Attacks"), though no action fires it.* The creature and item kind
  records (`gen_creature_kind_t`, `gen_item_kind_t`) don't get a `ranged`
  field until a kind needs one: 6 bytes per kind for nothing. So
  `COMPONENT_RANGED`'s comment changes from "Reserved" to "No kind has one
  yet".

- **D4 The AI component is a marker for now: a bit, no tables.** What an
  AI needs (a behaviour by number, G-ARCH-7; a target; a state) depends on
  the AI module, and its data goes where that module reads it (N-DATA-1),
  probably its own page. So now: `ai_add()`, `ai_remove()` and `AI_IS()`,
  in `game/components/ai.{h,c}`, the functions banked in page 110 with the
  other component functions. **Rejected:** an `ai_kind[]` table in slot 0
  now (100 bytes): its place and fields are the AI module's to decide.

- **D5 The creature generator adds it (R7)** in `gen_creature()`, to every
  creature without `CREATURE_PLAYER`, beside the `TIMER_MONSTER` timer it
  already gives them. The `TIMER_MONSTER` handler (game/game.c) doesn't
  test it yet: every monster has it, and the turn still waits. **Option
  (Q2):** a creature kind flag (`CREATURE_AI`) so that a kind can be a
  monster without AI (a statue, a mimic waiting): no kind needs that yet.

- **D6 The name is `ai`: `COMPONENT_AI`, `ai_add()`, `AI_IS()`.**
  **Rejected:** `monster`: a monster is what the kind is (`CLASS_CREATURE`,
  not the player); the component says its turns are the game's to decide,
  which could later be true of a charmed ally too.

## 5. Design

### 5.1 Ranged (`game/components/attack.{h,c}`)

```c
#define ATTACK_MELEE      0
#define ATTACK_RANGED     1
#define ATTACK_USE_COUNT  2
```

`attack_add()`: `nge_util_assert(use == ATTACK_MELEE ? k->range == 1 : k->range >= 2);`
The file comment drops "later" for the ranged use and says no kind has
one yet. `attack_cdata.c` is unchanged: its tables take
`ATTACK_USE_COUNT`.

### 5.2 AI (`game/components/ai.{h,c}`)

```c
#define AI_IS(id)  NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_AI)

/** @brief Give a creature in use the AI component: the game decides its turns. */
void ai_add(nge_entity_id_t id) NGE_BANKED;

/** @brief Remove it. */
void ai_remove(nge_entity_id_t id) NGE_BANKED;
```

Both assert on the entity's state, as the other components' functions do.

### 5.3 components.h

```c
#define COMPONENT_RANGED  NGE_COMPONENT(NGE_COMPONENT_USER + 6)   /**< A ranged attack (attack.h). No kind has one yet. */
...
#define COMPONENT_AI      NGE_COMPONENT(NGE_COMPONENT_USER + 11)  /**< The game decides its turns: monsters (ai.h). */
#define COMPONENT_COUNT   12
```

### 5.4 Memory

- Slot 0: 600 bytes (D1); none for AI (D4).
- Page 110: `ai_add()` and `ai_remove()`, a few dozen bytes.

## 6. Scope

- **In:** the two components, the generator giving monsters AI, tests,
  docs/memory_map.md (attack's tables, ai.c in page 110).
- **Later:** R5 and R9.

## 7. Tests

PC tests:

- **`test_attack`:** a ranged attack added (range 4), read back, its
  component set and melee's not; both uses on one entity, each removed
  alone; `attack_hits()` and `attack_damage()` with the ranged use; the
  range asserts (`TEST_EXPECT_HALT`): melee 2, ranged 1.
- **`test_ai`** (new, linked like `test_mana`): `ai_add()`, `AI_IS()`,
  `ai_remove()`; asserts on a free entity.
- **`test_gen`:** a rat and a goblin have AI, the player doesn't; items
  and features don't; `COMPONENT_AI` in the list of components with a bit
  of their own.

CSpect checks:

1. A new game: monsters shown and waiting, the player moving and
   attacking, as before (nothing visible changes).

## 8. Risks

None known: both copy existing patterns (attack's uses; a marker
component, as `COMPONENT_PC` without its data).

## 9. Questions

Q1 and Q2 are answered by D1 and D5 (agreed 2026-10-28): the ranged
tables now; every monster gets AI.

## 10. As built: differences from the design

- **Memory** (5.4): page 50 grew from $1657 to $18AF bytes, the 600 of D1,
  leaving $651 (1.6K) free. docs/memory_map.md said "about 3.4K" for the
  ECS tables, which had gone stale; it now says 6.3K. **Page 110 has $77
  (119) bytes left** ($1F89 used): the next component's functions may not
  fit there, so a new component will need room found first (moving the
  level table out, or a second page for the component functions).
- **`test_gen`** also checks that items and features have no AI, besides
  the creatures; and `test_attack` checks the ranged hit rule against the
  melee one on the same entity (each its own to-hit).
- zsdcc's "conditional flow changed by optimizer" note at
  `attack_add()` and `attack_remove()` is unchanged (docs/component_tables.md
  section 10).

