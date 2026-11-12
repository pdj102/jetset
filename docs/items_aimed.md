# Items aimed at a tile or an area: design specification

I3 in [todo.md](todo.md): items that blink their user to a tile, and items whose effect
reaches every creature in an area. The plan is [items_aimed_plan.md](items_aimed_plan.md).

Status: built 2026-11-12 (the plan's steps 1 to 5; what differed is in "As built" at the end).

What exists is specified elsewhere and not repeated here: the aim action in
[action_aim.md](action_aim.md), the cast action in [action_cast.md](action_cast.md), the
line of fire in [actions.md](actions.md) section 4.11, blink and `effect_apply_at()` in
[effects_extensions.md](effects_extensions.md) section 3.6, the items in
[items.md](items.md), targeting in [targeting.md](targeting.md) and the VFX in
[vfx.md](vfx.md).

## 1. Overview

Today every aimed item reaches one creature: the first on the line (a bolt), or the one on
the chosen tile (direct). Spells have one more shape, `SPELL_TILE`, which sends the caster
to a free tile, and nothing has an area.

I3 gives items the spells' shapes, and adds two area shapes that items and spells share:

| Shape | Who it reaches | Example |
|---|---|---|
| `SPELL_BOLT` | The first creature on the line | Wand of fire (as now) |
| `SPELL_DIRECT` | The creature on the chosen tile | Wand of sleep (as now) |
| `SPELL_TILE` | The user, sent to a free tile | Blink parchment, staff of blinking (I3.1) |
| `SPELL_BALL` **New** | Every creature within 1 of the chosen tile, the user too | Wand of fireballs (I3.2) |
| `SPELL_BURST` **New** | Every creature within 2 of the user, not the user. No tile is chosen | Panic parchment (I3.2) |

One helper in `act_shared.c` finds who a shape reaches and applies the effect. `act_aim()`
and `act_cast()` both call it, so the shapes behave the same for items and spells. No
spell uses a ball or a burst yet: a Fireball spell or a dragon's breath will be just a row.

```text
 item kind: aim hook + shape (in its flags)          spell row: shape
              |                                          |
              v                                          v
          act_aim()                                  act_cast()
              |   own tile, bolt or direct: the user     |
              +-------------------+----------------------+
                                  v
                act_shape_find()  (validate: no random numbers, so a check agrees)
                  bolt, direct  -> act_line(), one target
                  tile          -> a free tile in line: the user goes there
                  ball          -> direct line; centre = the chosen tile
                  burst         -> centre = the user's tile
                  ball, burst   -> act_area(): up to 8 creatures, nearest first
                                  |
                                  v
                act_shape_apply() (apply)
                  one target    -> effect_apply()
                  tile          -> effect_apply_at()
                  area          -> EV_AREA, then effect_apply() on each creature
```

## 2. Terms

| Term | Meaning |
|---|---|
| Shape | Who an aimed item or a spell reaches, `SPELL_*`: one set for both |
| Ball | A shape whose line is direct, and which reaches every creature around the chosen tile |
| Burst | A shape that reaches every creature around its user, and not the user, with no tile chosen |
| Area | The tiles a ball or burst covers: within its radius of the centre (`nge_map_within()`) |
| Centre | The tile an area is around: the chosen tile for a ball, or the user's tile for a burst |
| Radius | `ACT_BALL_RADIUS` 1 (9 tiles) or `ACT_BURST_RADIUS` 2 (21 tiles) |
| Ring | The tiles of an area at one distance from the centre: within d but not within d - 1 |
| Caught | The user inside the area of its own ball |

## 3. Requirements

### Shapes

- **R1** An aimed item has a shape, from its kind: `SPELL_BOLT`, `SPELL_DIRECT`,
  `SPELL_TILE`, `SPELL_BALL` or `SPELL_BURST`. Items and spells share the constants
  (todo.md I3.2). `SPELL_SELF` isn't an item shape: a use hook does that.
- **R2** Bolt and direct items behave exactly as now, the user's own tile included
  (action_aim.md R3: it picks the user).
- **R3** A tile item sends its user to a free tile within `ACT_AIM_RANGE` and a clear line,
  as `SPELL_TILE` does: its effect is the user's, at that tile (`effect_apply_at()`). A
  creature on the tile, a tile the user can't enter, and the user's own tile are refused.
- **R4** A ball's line is direct: it passes over creatures on the way, and a wall or
  closed door stops it. Its centre is always the chosen tile. Every creature within
  `ACT_BALL_RADIUS` (1) of the chosen tile gets the effect, **the user too** if it is there
  (answered 2026-11-12).
- **R5** A burst chooses no tile. Every creature within `ACT_BURST_RADIUS` (2) of the user
  gets the effect, never the user.
- **R6** A creature in an area is reached only if the line from the centre to it is clear,
  by the test `act_line()` uses (no wall, closed door or map edge), so an area doesn't reach
  through walls.
- **R7** An area with no creature in it but the user is refused with `ACT_R_NO_TARGET`, so
  a charge or a parchment isn't wasted. A ball aimed at the user's own tile is refused the
  same way: the cursor starts there, and ENTER pressed at once mustn't set the player on
  fire.
- **R8** An area reaches at most `ACT_AREA_MAX` (8) creatures, ring by ring from the centre,
  so the furthest are the ones left out (section 4.6, the event queue).
- **R9** Each creature reached gets the effect at the hook's power, with its own saving
  throw, and the user as the source, so a kill earns its XP (action_aim.md R5).
- **R10** A check reaches the same creatures as the action, with no random numbers
  (actions.md R5, R6).

### Spells

- **R11** `SPELL_BALL` and `SPELL_BURST` work in `act_cast()` through the same helper, at
  the spell's range for a ball. No spell row uses them yet (answered 2026-11-12).
- **R12** The existing spell shapes behave exactly as now: the refactor changes no spell.

### The player

- **R13** A tile or ball item asks for a tile with the targeting cursor; a burst item asks
  for none, and goes as soon as it is chosen in the aim list. A tile item's prompt is "Aim
  to where?"; a ball's is "Aim at what?", as now.
- **R14** **New:** aiming a ball, the cursor is amber on a tile in range within
  `ACT_BALL_RADIUS` of the user: aimed there, the user would be caught. It is still green
  further away and red out of range (targeting.md R5). The amber is a warning, and ENTER
  still chooses the tile.
- **R15** The inventory screen's aim check accepts `ACT_R_NO_TARGET` on the player's own
  tile, as the spell list does (action_cast.md R7), so a tile, ball or burst item is offered
  before a tile is chosen. The action itself still refuses it, with its message and no turn
  used.

### What is shown and said

- **R16** A ball shows a bolt in the effect's colour to the centre and a burst there; each
  creature's impact or glow starts when the bolt lands (vfx.md R8, R20). A burst item shows
  each creature's glow.
- **R17** The player is told "You aim the wand of fireballs.", as now, then each creature's
  messages from its effect. A player caught in its own ball is told as for any fire ("You
  are burned!").

### The items

- **R18** Six new kinds, data only (answered 2026-11-12; section 4.10): blink parchment,
  staff of blinking, wand of fireballs, wand of frost balls, fireball parchment and panic
  parchment.
- **R19** An item's shape is in its kind's flags byte: the record gains no bytes, and
  `ITEMF_DIRECT` keeps its bit and meaning, so no existing row changes.
- **R20** Only tile items have an effect that needs a tile (`EF_BLINK`), and every tile
  item has one.

### Constraints

- **R21** Page 109 grows by about 240 bytes, to about 195 free (section 8). The helper and
  the shapes' code go in `BANK_56`, which has about 2.7K free. Common memory doesn't grow.
- **R22** No multiply and no library routine (items.md D10).

## 4. Design

### 4.1 The shapes

The shape constants stay in [spells.h](../src/game/spells/spells.h), and gain two:

```c
#define SPELL_SELF    0   /* the caster */
#define SPELL_BOLT    1   /* the first creature on the line to a tile (ACT_LINE_BOLT) */
#define SPELL_DIRECT  2   /* the creature on the chosen tile, past any on the way (ACT_LINE_DIRECT) */
#define SPELL_TILE    3   /* a free tile in range and line: its effect on the caster, there */
#define SPELL_BALL    4   /* every creature within ACT_BALL_RADIUS of the chosen tile, the caster too (ACT_LINE_DIRECT) */
#define SPELL_BURST   5   /* every creature within ACT_BURST_RADIUS of the caster, not the caster */
#define SPELL_SHAPE_COUNT  6
```

| Shape | Line | Reaches | Refused when | x, y read |
|---|---|---|---|---|
| Self | None | The user | Never | No |
| Bolt | `ACT_LINE_BOLT` | `a->target`: the first creature | Out of range, no line, no creature | Yes |
| Direct | `ACT_LINE_DIRECT` | `a->target`: the one on the tile | The same | Yes |
| Tile | `ACT_LINE_DIRECT` | The user, at (x, y) | A creature there (`ACT_R_BLOCKED_ENTITY`), can't enter (`ACT_R_BLOCKED`), own tile, out of range, no line | Yes |
| Ball | `ACT_LINE_DIRECT` | The area around the chosen tile, the user included | Out of range, no line, own tile, nobody but the user in the area | Yes |
| Burst | None | The area around the user, the user left out | Nobody in the area | No |

`ACT_BALL_RADIUS` 1, `ACT_BURST_RADIUS` 2 and `ACT_AREA_MAX` 8 are in `act.h`, beside
`ACT_AIM_RANGE`. A burst has no range: a spell row's range is 0 for it, as for self.

### 4.2 An item's shape in its flags

The kind's flags byte has three bits for the shape. Bit 2 is `ITEMF_DIRECT` already, so it
becomes one value of a field and the 16 direct kinds stay as they are:

| Flags & `ITEMF_SHAPE` (0x1C) | Name | Shape |
|---|---|---|
| 0x00 | (none) | `SPELL_BOLT` |
| 0x04 | `ITEMF_DIRECT` | `SPELL_DIRECT` |
| 0x08 | `ITEMF_TILE` | `SPELL_TILE` |
| 0x0C | `ITEMF_BALL` | `SPELL_BALL` |
| 0x10 | `ITEMF_BURST` | `SPELL_BURST` |

A row sets one: `.flags = ITEMF_BALL`. 0x14 to 0x1C are invalid, which a test checks.
`gen_item_shape(kind)`, in page 108 with the other readers, returns the `SPELL_*` value
from a 5-byte table indexed by `(flags & ITEMF_SHAPE) >> 2`, unsigned (zsdcc's signed
shifts: [toolchain.md](guidelines/toolchain.md)). It asserts an invalid value. A kind with
no aim hook reads as a bolt, which nothing asks.

### 4.3 The helper

In `act_internal.h`, for the two actions:

```c
typedef struct act_shape {
    uint8_t shape;                         /* in: SPELL_* */
    uint8_t cx, cy;                        /* out: a tile's or an area's centre */
    uint8_t count;                         /* out: the creatures in ids[] */
    nge_entity_id_t ids[ACT_AREA_MAX];     /* out: an area's creatures, nearest first */
} act_shape_t;                             /* 12 bytes, on the action's stack */

/* Validate: who s->shape reaches from a->actor towards (a->x, a->y) within range */
uint8_t act_shape_find(act_t *a, act_shape_t *s, uint8_t range);

/* Apply: effect ef at power to whom act_shape_find() found, the actor as source */
void act_shape_apply(act_t *a, const act_shape_t *s, uint8_t ef, int8_t power);
```

**`act_shape_find()`**, returning `ACT_R_NONE` or why not:

- **Self:** `a->target` is the actor.
- **Bolt, direct:** `act_line()` with the shape's mode, as the two actions do now.
- **Tile:** `act_cast()`'s `SPELL_TILE` code, moved here unchanged; (cx, cy) is the tile,
  `a->target` the actor.
- **Ball:** `act_line()` with `ACT_LINE_DIRECT`. `ACT_R_NONE` (a creature on the chosen
  tile) and `ACT_R_NO_TARGET` with the chosen tile not the actor's (the line is clear, the
  tile empty) both mean the line reaches the tile, so the centre is the chosen tile. Any
  other reason, or the actor's own tile, is returned. Then `act_area()` with
  `ACT_BALL_RADIUS`, the actor included.
- **Burst:** the centre is the actor's tile; `act_area()` with `ACT_BURST_RADIUS`, the
  actor left out. x and y aren't read.

**`act_area()`** (static, in `act_shared.c`) walks the rings, d from 0 to the radius, and
in each ring the tiles of the square around the centre that are within d and not within
d - 1 (`nge_map_within()`, so the area is the same circle as every range: 9 tiles at 1, 21
at 2). A tile is skipped unless the line from the centre to it is clear (R6): the block walk
`act_line()` uses, from (cx, cy), with `LINE_PASS`. On a tile, each alive entity with
health and hit points goes into `ids[]` (the actor only if included), until `ACT_AREA_MAX`.
If none but the actor is in, it returns `ACT_R_NO_TARGET`. `a->target` is the first
creature in `ids[]` that isn't the actor, for the AI's check (action_cast.md, opt_cast).

The creatures are collected before any effect applies (D13). The walk visits 25 tiles at
most, each one a location walk and a line of at most 2 steps: done once in a check and
once in the action, it is cheap beside a frame.

**`act_shape_apply()`:**

| Shape | Does |
|---|---|
| Self, bolt, direct | `effect_apply(ef, power, a->target, actor)` |
| Tile | `effect_apply_at(ef, power, actor, actor, cx, cy)` |
| Ball, burst | `EV_AREA` (target none, source the actor, p1 cx, p2 cy); then for each creature in `ids[]` that is still alive, `effect_apply(ef, power, id, actor)` |

The check "still alive" is for an effect with a consequence on another creature; none has
one now.

### 4.4 The aim action

`act_aim()` keeps its order (action_aim.md section 3). What changes:

1. **Context:** the shape, `gen_item_shape()`, into a local.
2. **Validate:** the item checks as now. Then, for a bolt or direct item on the actor's own
   tile, the target is the actor (R2); for anything else, `act_shape_find()` with
   `ACT_AIM_RANGE`. The mode isn't read from `ITEMF_DIRECT` any more: the shape says it.
3. **Resolve:** nothing to roll, as now.
4. **Apply:** `EV_AIMED`, with target `a->target` for a bolt, direct or tile item (the
   actor, for a tile), and `NGE_EVENT_NONE` for a ball or burst; the charge used or the
   parchment destroyed; then `act_shape_apply()` at the hook's power.

The reasons are action_aim.md's, and `ACT_R_BLOCKED` and `ACT_R_BLOCKED_ENTITY` for a tile
item, with the texts the cast action already gives them.

### 4.5 The cast action

`act_cast()`'s three shape branches become one call of `act_shape_find()` with the spell's
range, after its own-tile rule for bolts and direct spells (it refuses the caster's tile,
action_cast.md R3, which `act_line()` already does). Its apply step calls
`act_shape_apply()`. `EV_CAST`'s target is `NGE_EVENT_NONE` for a ball or burst;
`ai_on_event()` already ignores that target, and each creature hurt reacts to its own
`EV_DAMAGED`.

### 4.6 Events

| Event | Target | Source | Payload | Posted by |
|---|---|---|---|---|
| `EV_AREA` **New**, `NGE_EVENT_USER + 38` | none | the user | p1: centre x, p2: centre y | `act_shape_apply()`, once, before the effects |

Nothing reacts to it but the VFX. Its messages come from the effects.

**The queue.** An action's events wait in the queue (32) until the game processes them
after the action. One creature's worst case is 3: `EV_DAMAGED`, `EV_DIED`,
`EV_EFFECT_STARTED` (action_cast.md, "a bolt that kills"). A full ball is `EV_AIMED` +
`EV_AREA` + 8 x 3 = 26. A ninth creature would make 29, so the cap of 8 leaves room for
what an effect might add later. Events that handlers post (XP, a level-up) come while the
queue drains, not on top of these.

### 4.7 Targeting

`target_t` gains `radius`: in, an area's radius around the chosen tile, or 0. The cursor's
bank (target_bank.c) becomes:

| Cursor's tile | Bank |
|---|---|
| Out of range | `BANK_RED`, as now |
| In range, radius not 0, within radius of the origin | `BANK_ORANGE` **New** |
| In range otherwise | `BANK_GREEN`, as now |

A ball's centre is always the chosen tile, so the amber is exact: the user is caught if and
only if the cursor's tile is amber (D12).

A new purpose, `TARGET_AIM_TILE`: "Aim to where?", with the help "Arrows: move ENTER: aim
Other: back". `choose_tile()` in `player_bank.c` gains the radius.

The player's commands pick the prompt from the shape:

| Shape | `item_command()` (aim) | `cast_command()` |
|---|---|---|
| Self | (not an item shape) | No prompt, as now |
| Bolt, direct | `TARGET_AIM`, radius 0 | `TARGET_CAST`, radius 0 |
| Tile | `TARGET_AIM_TILE` | `TARGET_CAST_TILE`, as now |
| Ball | `TARGET_AIM`, `ACT_BALL_RADIUS` | `TARGET_CAST`, `ACT_BALL_RADIUS` |
| Burst | No prompt | No prompt |

The inventory screen's `allowed()` passes `ACT_AIM` when the check fails with
`ACT_R_NO_TARGET` (R15).

### 4.8 What is shown

In `anim_vfx.c` (pages 126/127):

- **`EV_AIMED` and `EV_CAST` with no target** fly nothing; they remember the effect, for
  `EV_AREA`.
- **`EV_AREA`:** if the centre isn't the source's tile, a bolt from the source to the
  centre in the effect's colour, then a burst at the centre (the splash pattern, the
  effect's bank), which waits for the bolt as any impact does (vfx.md D4). The centre is
  remembered.
- **Impacts and glows elsewhere** (`EV_DAMAGED`, `EV_EFFECT_STARTED`, `EV_DIED`) start no
  earlier than the bolt to the remembered centre lands: `nge_vfx_arrival()` of the centre.
  The centre is forgotten at the next `EV_FIRED`, `EV_CAST`, `EV_AIMED` or `EV_AREA`, and
  on a change of level (vfx.md R14).
- A burst's centre is its user's tile, so it flies nothing; each creature glows as it
  would for a direct item.

A full ball can ask for more than the 12 VFX slots (a bolt, the centre's burst, and a splash
and flash for each of 9 creatures). The last ones aren't shown, as vfx.md section 5.1
allows: the rules don't wait for them.

### 4.9 What is said

`EV_AIMED`'s message reads only the item, so "You aim the wand of fireballs." is unchanged.
Each creature's effect gives its own messages, as for a bolt. A player killed by its own
fireball dies as from any damage; the plan checks what the death message says when the
killer is the player.

### 4.10 The kinds

Numbered at the end (items.md 4.8): 212 to 217, `IT_COUNT` 218. Data only.

| ID | Name | Look | Count | Flags | Hook | Band, rarity |
|---|---|---|---|---|---|---|
| `IT_SP_BLINK` 212 | blink parchment | parchment, `BANK_BLUE` | - | `ITEMF_TILE` | aim `EF_BLINK` 1 | 2-B, 3 |
| `IT_STAFF_BLINK` 213 | staff of blinking | staff, `BANK_BLUE` | 6 | `ITEMF_TILE` | aim `EF_BLINK` 1 | 5-B, 1 |
| `IT_WAND_FIREBALL` 214 | wand of fireballs | wand, `BANK_RED` | 4 | `ITEMF_BALL` | aim `EF_BOLT_FIRE` 4 | 6-B, 1 |
| `IT_WAND_FROSTBALL` 215 | wand of frost balls | wand, `BANK_BLUE` | 4 | `ITEMF_BALL` | aim `EF_BOLT_COLD` 4 | 6-B, 1 |
| `IT_SP_FIREBALL` 216 | fireball parchment | parchment, `BANK_ORANGE` | - | `ITEMF_BALL` | aim `EF_BOLT_FIRE` 5 | 4-B, 2 |
| `IT_SP_PANIC` 217 | panic parchment | parchment, `BANK_RED` | - | `ITEMF_BURST` | aim `EF_FEAR` 1 | 3-B, 2 |

B is `LEVEL_DEPTH_BOTTOM`.

- **Powers.** A wand of fire is 6 on one creature and the fire parchment 8; a ball is 4 or
  5 on up to 9, the user among them. Fear keeps its Cha save (DC 12), per creature.
- **Looks.** No staff or wand is blue or red yet. Parchments already share banks (items.md
  4.10); the names tell them apart.
- **Names.** "scare monsters parchment" is 24 characters, over items.md R19's 19, so it
  is the panic parchment. "wand of frost balls" is 19 exactly.
- **Range.** Every item aims at `ACT_AIM_RANGE` (8), blink too, though the Blink spell's
  range is 6 (action_aim.md R1: kinds have no range of their own).

## 5. Decisions

- **D1 Items take the spells' shapes.** One set of `SPELL_*` constants, read from a spell's
  row or an item's flags, and one helper reads them (todo.md I3.2). *Rejected:* an item-only
  set of shapes, which would need translating at the helper and would drift.
- **D2 The shape is a 3-bit field in the flags byte, with `ITEMF_DIRECT` as one value.** It
  costs no byte (R19) and changes no existing row. A field can't hold two shapes at once.
  *Rejected:* a shape byte in the record, 212 bytes of the 435 free. *Rejected:* three more
  flags beside `ITEMF_DIRECT`: the same bits, and they allow a kind that is both a ball and
  a tile.
- **D3 One helper for both actions** (answered 2026-11-12). A Fireball spell or a breath is
  then only a row, and the shapes can't behave differently for items and spells. `act_cast()`
  loses code it had. *Rejected:* the area only in `act_aim()`, copied into cast later.
- **D4 A ball catches its user** (answered 2026-11-12), as in NetHack. It makes the
  choice of tile matter. What follows from it: aiming at one's own tile is refused (R7), the
  amber cursor warns (R14), and an area with nobody but the user is refused (R7).
  *Rejected:* never catching the user, as in Angband.
- **D5 A burst leaves out its user.** It is around the user by definition, and a panic
  parchment that frightened its reader would be useless.
- **D6 Fixed radii: a ball 1, a burst 2** (answered 2026-11-12). No byte per kind or row.
  The burst is wider because it reaches what is around the user, which a ball can't choose.
  *Rejected:* a radius from the power, which already sets the damage; a byte per kind.
- **D7 An area needs a clear line from its centre** (R6). *Rejected:* every creature within
  the radius, which would burn a monster through a wall. At radius 1 a diagonal neighbour has
  no tile between, so it is always reached, round a corner too; that is accepted.
- **D8 At most 8 creatures, nearest first.** It keeps one action's events within the queue
  (4.6). *Rejected:* processing the queue between creatures, which an action mustn't do
  (actions.md R13, R16: callers and handlers own what follows). *Rejected:* a bigger queue,
  5 bytes an event in common memory, 160 bytes for 32 more, for a case a radius-1 ball
  can only just reach (9 tiles).
- **D9 An empty area is refused.** As a bolt with nobody on its line is (`ACT_R_NO_TARGET`),
  so a charge isn't lost by a slip. *Rejected:* letting it go off, which only wastes it.
- **D10 A ball bursts at the chosen tile** (asked for 2026-11-12). Its line is direct, as a
  wand of sleep's: it passes over creatures on the way, and only a wall or closed door stops
  it. The player places the area exactly, and the amber cursor is then exact (D12).
  *Rejected:* a bolt's line, bursting at the first creature on the way, as in Angband: a
  creature next to the player would make every fireball catch its user, and the cursor
  couldn't show the real area without walking the line at each move.
- **D11 A burst item has an aim hook, not a use hook.** A use hook reaches its user
  (items.md 4.4), and the aim hook says an item reaches others (items.md D8). The player
  chooses it with `a` and it goes without a prompt. *Rejected:* a use hook with a shape,
  which would give `ACT_USE` a second meaning.
- **D12 The amber cursor warns; it doesn't refuse.** A cornered player may choose to be
  caught. It is exact, as the centre is the chosen tile (D10). *Rejected:*
  refusing a ball that would catch the user. *Rejected:* drawing the area, 9 overlay sprites
  for a radius-1 ball.
- **D13 Creatures are collected before any effect.** An effect that moved or killed one
  couldn't then make the walk reach a creature twice or miss one. 8 bytes on the stack.
- **D14 No new cast purpose for a ball.** `TARGET_CAST` with a radius; a purpose only
  chooses the prompt. The tile item gets `TARGET_AIM_TILE`, as a tile spell has
  `TARGET_CAST_TILE`, because "Aim at what?" asks for a creature.

## 6. Scope

**In:** the five item shapes and the helper; `SPELL_BALL` and `SPELL_BURST` in
`act_cast()`; the six kinds; the amber cursor; `EV_AREA` and its VFX; the docs.

**Later:**

- Area spells and breath: rows only (spells.md section 6). An AI rule for when a monster
  would catch itself or another monster in a ball.
- Monsters aiming items (action_aim.md section 6).
- Cones, beams and walls of fire; an area that lasts (a cloud).
- I4's thrown potions could break as a radius-1 ball of their effect.
- Showing the area on the map while targeting.

## 7. Tests

PC tests:

- **`test_act`**, the shapes through `act_aim()`:
  - **Tile:** the user lands on the tile; a creature there, a wall beyond the line, the
    user's own tile and a tile past range are refused with their reasons; the parchment is
    used up, the staff's charge used.
  - **Ball:** it bursts at the chosen tile, empty or not; a creature on the way is passed
    over and isn't reached unless within 1 of the tile; a wall on the way refuses it;
    every creature within 1 of the tile is damaged, one behind a wall from it isn't;
    the user next to the centre is damaged too; the user's own tile, and an area with only
    the user, are refused with `ACT_R_NO_TARGET` and keep the charge.
  - **Burst:** creatures within 2 are frightened, one at 3 isn't, the user isn't; nobody
    around is refused.
  - **The cap:** a ball over 9 creatures reaches 8, the centre's first, and the events
    posted fit the queue.
  - **Check and do agree:** each case's check gives the same reason and `a->target`.
  - Bolt and direct items unchanged: the existing tests pass as they are.
- **`test_act`**, cast: the existing cast tests pass unchanged (R12).
- **`test_gen`:** the shape field holds 0 to 4 only; `gen_item_shape()` for each value;
  every kind with `EF_BLINK` is a tile item with an aim hook, and every tile item has an
  effect that needs a tile (R20); burst and ball kinds have aim hooks; the six rows well
  formed; the mix by depth still within R4.
- **`test_target`:** the cursor's bank: red out of range, amber within the radius of the
  origin, green beyond it, green everywhere in range with radius 0.
- **`test_ui`:** the aim list offers a ball item with nobody near; the panic parchment
  goes without a prompt.
- **`test_anim`:** `EV_AREA` after an `EV_AIMED` with no target starts a bolt to the
  centre; an impact on another tile waits for it.

CSpect checks are in the plan.

## 8. Known risks

- **Page 109.** About 240 bytes for six kinds (22 each and the names) takes it from 435 to
  about 195 free, 2.4%. The kind after these needs room made first: items.md section 8
  suggests moving the names to page 108.
- **The queue.** 26 of 32 for a full ball. An effect that posts more events per creature
  must lower `ACT_AREA_MAX`; a test sums the worst case.
- **The VFX pool.** A full ball can run out of slots, and the furthest impacts don't show.
- **A player's own fireball can kill it.** Power 4 or 5 is 4 or 5 fire or cold damage, less
  with a ward. The amber cursor and R7 guard against slips; play will say whether the
  powers are right.
- **zsdcc.** The shape lookup shifts a flags byte: unsigned, and the asm read once
  (toolchain.md T-ZSDCC-3). The helper reads `a->x`, `a->y` and the struct's fields into
  locals (T-ZSDCC-1).

## As built

### Step 1: shapes and the helper

- **Memory.** In the hardware build (`-DNDEBUG`) the step costs what the plan says:
  `BANK_56` grows by 14 bytes (the helper +456, `act_cast.c` -408, `act_aim.c` -34) and
  page 108 by 46. The CSpect build keeps its asserts, each with its expression and the
  file's name as strings in its own page: there `BANK_56` grows by 318 bytes, to 15,265 of
  16,384 (1,119 free), and page 108 by 179. R21's "about 2.7K free" is not the CSpect
  build's figure: steps 3 and 4 must be measured against the 1,119 bytes, with few asserts
  in the area code.
- **`act_shape_apply()` asserts nothing:** `act_shape_find()` has checked the shape.
- **`test_gen`'s direct-kinds loop** compares the whole field to `ITEMF_DIRECT`, not the
  bit: `ITEMF_BALL` (0x0C) has bit 2 set too. It checks the same 11 kinds.

### Step 2: tile items (I3.1)

- **As planned:** `act_aim()` needed no change, and a tile item's refusals give Blink's
  texts, as `messages_act_failed()` reads only the reason.
- **Memory:** page 109 grows by 78 bytes, to 357 free. The prompt and the shape check
  cost 19 bytes in page 116 (the inventory screen), 18 in page 124 (targeting) and 31 in
  page 114 (the player's commands). Common memory and `BANK_56` are unchanged.
- **`test_gen`** no longer exempts `EF_BLINK` from "every effect is made by some item".

### Step 3: balls and bursts (I3.2)

- **`act_area()` reads a table, not the rings.** The 21 offsets within radius 2, ring by
  ring (9 within 1), are a const table in `act_shared.c`: one loop of 9 or 21 tiles, with no
  distance test. The ring walk of section 4.3, `nge_map_within()` at d and d - 1 on each
  tile of the square at each ring, was 634 bytes in the hardware build; the table and its
  loop are 448. A check typedef ties the table to `ACT_BALL_RADIUS` 1 and
  `ACT_BURST_RADIUS` 2, and `test_act` checks it against `nge_map_within()` tile by tile,
  so the areas are still the circles of every range.
- **The line from the centre is `nge_block_line(..., NGE_BLOCK_SOLID)`,** not a block
  walk: the same tiles and the same test (walls, closed doors and the edge block;
  creatures don't), in one call. It doesn't test the ends, which changes nothing: the
  centre was tested by `act_line()` or is the user's tile, and a creature's tile is never
  solid.
- **`SPELL_IS_AREA(s)`** (spells.h) is `s >= SPELL_BALL`: ball and burst are the last
  shapes. One compare where the actions choose `EV_AIMED`'s and `EV_CAST`'s target.
- **`choose_for_shape()`** in `player_bank.c` holds section 4.7's table for both
  `item_command()` and `cast_command()`.
- **What is said (R17, 4.9).** A player caught in its own ball is told "You take 4 fire
  damage.", the text of all fire damage, not "You are burned!". Killed by itself it is told
  "You die...", as by anything: the death message doesn't read the killer, and
  `player_on_pc_event()` gives no XP for the player's own death. Nothing changed.
- **The queue.** A full ball where every creature dies posts 26 events, as section 8
  says; `test_act` checks that figure, so an effect that posts more fails it.
- **Memory.** Page 109 grows by 161 bytes, to 196 free. In the hardware build `BANK_56`
  grows by about 764 bytes (`act_shared.c` +724, `act_aim.c` +61, `act_cast.c` -21), not
  the plan's 400: zsdcc's code for the area's loop and banked calls. In the CSpect build
  `BANK_56` is 15,989 of 16,384: **395 bytes free**. Page 114 (the player's commands) grows
  by 118, page 124 (targeting) by 106, and pages 126/127 by 22 (the VFX guard). Common
  memory is unchanged.

### Step 4: the area VFX

- **Where the state is.** The remembered effect and centre (`anim_area_ef`,
  `anim_area_held`, `anim_area_x`, `anim_area_y`, 6 bytes) are in page 127 with the
  module's other page data, mapped while its code runs: common memory doesn't grow.
  `anim_init()` sets them.
- **`anim_vfx_clear()`** (anim.h) is `nge_vfx_clear()` and forgets the centre. `game.c`
  calls it on a change of level and a new game, where it called `nge_vfx_clear()`.
- **The centre is remembered only when a bolt flew to it.** A burst's centre is its
  user's tile: nothing flies, and its glows start at once.
- **A flash waits too:** `damaged()` gives the hit flash the same delay as the splash.
- **Memory.** Page 126 grows by 486 bytes in both builds (the code has no asserts), not
  the plan's 150: zsdcc's code for the new cases. The page has about 5K free. Page 127
  grows by 6.

### Step 5: closing docs

- **Not checked on CSpect:** check 4's third item, a ball over 9 or more creatures (the
  VFX pool running out). `test_act` checks that such a ball reaches 8 and that its events
  fit the queue; that nothing hangs when the pool is full is untested in the emulator.
