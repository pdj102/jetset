# ECS: design specification

How nge's entity component system works: what an entity is, how it lives and
dies, who owns it, and where its data is kept.

The public interface is in [nge_entity.h](../src/nge/ecs/nge_entity.h),
[nge_owner.h](../src/nge/ecs/nge_owner.h) and
[nge_ecs.h](../src/nge/ecs/nge_ecs.h). The component model is in
[nge_components.h](../src/nge/ecs/components/nge_components.h), and why it has
tables in [component_tables.md](component_tables.md). Addresses and page data are
in [memory_map.md](memory_map.md). Persistent and parked entities are specified in
[level_transitions.md](level_transitions.md).

This spec covers the entities, ownership, the data page and how the components and
systems use them. What the timer and render systems do beyond that is in their
headers, [nge_timer.h](../src/nge/ecs/systems/nge_timer.h) and
[nge_render.h](../src/nge/ecs/systems/nge_render.h).

## 1. Overview

**An entity is a number.** It is a one-byte ID with a state, some flags and a set of
component bits. It has no other data of its own: a component's data is in tables
indexed by the ID, one table per field, because indexing a byte table is the
cheapest access on the Z80.

**An entity lives through three states.** It is free, alive or dying, and it only
ever moves one way round:

```text
              nge_entity_create()            nge_entity_destroy()
     FREE -------------------------> ALIVE -------------------------> DYING
      ^                                       refused if persistent     |
      |                                                                 |
      +-----------------------------------------------------------------+
                  clean-up: nge_entity_next_dying(), nge_entity_release()
```

**Destruction is deferred.** `nge_entity_destroy()` only asks for it. The alive
loops skip the entity from then on, so no system sees it again, but its ID stays in
use, its entry stays in the used list and its data stays readable until the game's
clean-up releases it at the end of the frame. Until then no other entity can have that ID, so anything that still names it
(an event in the queue, a timer loop part way through a tick) is still naming the
same thing.

**nge provides the lifecycle, and the game runs the clean-up.** nge knows nothing
of inventories or features, so it only hands dying entities over one at a time. What
happens to a dead creature's pack is the game's business, in
`level_release_dying()`.

**Every entity in use has one owner:** another entity, the world or an area.
Ownership is one table, the owner of each entity, and it is how an entity's contents
and effects find their way out when it goes.

```text
 game code                    nge lifecycle                   game clean-up
 ---------                    -------------                   -------------
 nge_entity_create()  ------> free stack -> used list
                                              |
 nge_entity_destroy() ------> state dying, dying stack
 health_damage(), act_use(),                  |
 effects, level_travel                        |     (end of the frame, after
                                              |      the frame's events)
                                              v
                              nge_entity_next_dying() --> level_release_dying():
                                                          drop items, destroy
                                                          effects, pass on the rest
                                                                 |
                     used list, free stack <-- nge_entity_release() <--+
```

## 2. Terms

| Term | Meaning |
|---|---|
| Entity | An ID, `nge_entity_id_t`, 0 to `NGE_ENTITY_MAX`-1 |
| `NGE_ENTITY_INVALID` | `0xFF`: not an entity, returned when there is none to give |
| State | Free, alive or dying: bits 0-1 of the entity's flags byte |
| In use | Alive or dying: may have flags, components and an owner |
| Flag | One of bits 2-7 of the flags byte: `NGE_ENTITY_PERSISTENT`, and five for the game |
| Persistent | Has `NGE_ENTITY_PERSISTENT`: `nge_entity_destroy()` leaves it alive |
| Component | A kind of data an entity can have, with one bit per entity |
| Handle | `NGE_COMPONENT(n)`, a `nge_component_t`: component n's table and bit |
| Used list | `nge_entity_used[]`: the IDs in use, alive and dying, in no order |
| Free stack, dying stack | The IDs ready to be created, and those waiting for clean-up |
| Destroy | Ask for an alive entity to go: alive to dying |
| Clean-up | The game's `NGE_ENTITY_RELEASE_EACH_DYING` loop, which takes dying entities and releases them |
| Taken | Off the dying stack, with `nge_entity_next_dying()`, but not yet released |
| Release | Give a dying entity's ID back: dying to free |
| Owner | An entity, `NGE_OWNER_WORLD` or `NGE_OWNER_AREA(a)` |
| Owner chain | An entity's owner, that owner's owner, and so on up to the world or an area |
| ECS data page | Page 50, in slot 0 for the whole game |
| Guard area | `$0000`-`$00FF` of the ECS data page, a pattern that catches null writes |

## 3. Requirements

### Entities

- **R1** An entity is a one-byte ID. Its state, flags and component bits are in
  tables indexed by that ID.
- **R2** There are `NGE_ENTITY_MAX` entities, 100 today. `0xFF` is never an ID.
- **R3** `nge_entity_create()` returns `NGE_ENTITY_INVALID` when none is free. That
  isn't an error: the caller decides what it means.
- **R4** A new entity is alive, with no flags or components, owned by
  `NGE_OWNER_WORLD`, whatever its ID was used for before.
- **R5** After `nge_entity_init()` IDs are handed out 0, 1, 2...

### Lifecycle

- **R6** An entity is free, alive or dying, and moves only from free to alive, alive
  to dying, and dying to free. Nothing brings a dying entity back.
- **R7** `nge_entity_destroy()` on a persistent entity changes nothing and returns
  0. To destroy one, clear the flag first.
- **R8** `nge_entity_destroy()` on a dying entity changes nothing and returns 0, so
  destroying twice is harmless.
- **R9** Destroying a free entity, or passing an ID that isn't one, is a programming
  error and asserts.

### Deferred destruction

- **R10** `nge_entity_destroy()` frees nothing. The ID stays in use until
  `nge_entity_release()`.
- **R11** The alive loops skip a dying entity at once, so systems stop seeing it from
  that moment (section 4.4). It leaves the used list only when it is released.
- **R12** A dying entity keeps its flags, components, owner and data until it is
  released, so event handlers can read what they need.
- **R13** Only the clean-up releases entities. Game code destroys, never releases
  (G-ENT-2).
- **R14** Events are processed before release, so no event names a freed ID.
- **R15** nge runs no clean-up of its own. It gives the game the next dying entity
  and takes it back when the game is done.
- **R16** Clean-up may destroy more entities. The same loop then handles them, until
  the dying stack is empty.
- **R17** An entity owns nothing when it is released.
- **R18** Release clears the entity's flags and component bits, and its ID can be
  reused at once.

### Ownership

- **R19** Every entity in use has exactly one owner: an entity, the world or an area.
- **R20** Ownership is stored once, as the owner of each entity. There are no lists
  of what an owner has.
- **R21** Ownership never forms a cycle. An entity can't own itself or one of its
  owners, directly or indirectly.
- **R22** An entity in use can be given to an owner in use, dying or not, so clean-up
  can pass a dying entity's contents to an owner that is dying too.

### Loops

- **R23** A loop over the used list may destroy any entity and may create entities.
  No entity is visited twice, and a new entity isn't visited until the next loop.
- **R24** Only release moves entries in the used list, and nothing releases inside a
  loop over it.

### Components

- **R25** A component has a number, a handle, and one bit per entity. Numbers 0 to
  `NGE_COMPONENT_USER`-1 are nge's, and the game's follow.
- **R26** A component's data is one table per field, indexed by ID, and means nothing
  for an entity without the bit.
- **R27** A component's add function sets every field. Neither `nge_entity_create()`
  nor `nge_entity_release()` touches the data tables.
- **R28** Code that runs every frame uses the unchecked macros. Everything else uses
  the checked functions, which assert.

### The data page

- **R29** Data read per entity, per frame, by several systems or by common code is in
  the ECS data page, in slot 0 for the whole game (N-DATA-1).
- **R30** Nothing reads ECS data while the ROM is in slot 0 (N-DATA-5).
- **R31** In debug builds a write through a null pointer is caught, as RAM in slot 0
  would otherwise take it silently.

### Constraints

- **R32** The lifecycle and the functions that change or search ownership are banked,
  in page 48, with their bookkeeping in page 49. They aren't called while game code
  has its own page mapped in slot 6 or 7 (N-BANK-4).
- **R33** Flag and component access, and the tables, need no banked call.
- **R34** `nge_entity_init()` resets the module completely and can be called again
  (N-INIT-1).
- **R35** ecs includes no game code (N-LAYER-2) and writes no registers itself
  (N-LAYER-3), so it runs entirely in the PC tests.

## 4. Design

### 4.1 Files

| File | Where it runs | What it has |
|---|---|---|
| `nge_entity.c` | Common | Checked flag and component functions |
| `nge_entity_life.c` | Page 48 | Init, create, destroy, next dying, release, counts |
| `nge_entity_pdata.c` | Page 49 | Lifecycle bookkeeping: the two stacks, each ID's place in the used list |
| `nge_entity_cdata.c` | Page 50 | Flags, component bits, the used list |
| `nge_owner.c` | Common | `nge_owner_get()` |
| `nge_owner_set.c` | Page 48 | Set, owns, count, move all |
| `nge_owner_cdata.c` | Page 50 | `nge_owner_of[]` |
| `nge_ecs.c` | Common | `nge_ecs_init()`, the guard check |
| `components/` | Common, tables in page 50 | Location and block; the line test is in page 56 |
| `systems/` | Pages 52-55 | The render and timer systems |

`nge_ecs.c` is in common memory because `nge_ecs_init()` maps slot 0 and so can't
run from it.

### 4.2 The flags byte

| Bits | Meaning |
|---|---|
| 0-1 | State: `NGE_ENTITY_FREE` 0, `NGE_ENTITY_ALIVE` 1, `NGE_ENTITY_DYING` 2 |
| 2 | `NGE_ENTITY_PERSISTENT` |
| 3-7 | The game's five flags, from `NGE_ENTITY_FLAG_USER` |

`nge_entity_set_flags()` and `nge_entity_clear_flags()` assert if they are asked to
touch the state bits. Only the lifecycle changes those. A free entity's byte is 0,
so a free entity has no flags.

Every test on state or flags is one byte read. `NGE_ENTITY_IS_ALIVE()` is the check
to use before acting on an ID from somewhere else.

### 4.3 The lifecycle

| | Free | Alive | Dying, on the stack | Dying, taken |
|---|---|---|---|---|
| On | Free stack | Used list | Used list, dying stack | Used list: the clean-up holds the ID |
| `nge_entity_create()` may return it | Yes | No | No | No |
| Flags and components | None | Yes | Kept | Kept |
| Owner | Old value, meaningless | Yes | Kept | Kept |
| Can own entities | No | Yes | Yes | Yes |
| `nge_entity_destroy()` | Asserts | Dying, or 0 if persistent | 0 | 0 |
| `nge_entity_release()` | Asserts | Asserts | Must be taken first | Free |

**Create** pops the free stack, sets the state alone in the flags byte, clears the
component bits, sets the owner to the world and appends the ID to the used list.
The caller gives it its real owner straight away (G-ENT-1).

**Destroy** checks the ID is in use, returns 0 unless the entity is alive and not
persistent, changes its state and pushes it on the dying stack. Its other flags are
kept, and the used list doesn't change.

**Next dying** pops the dying stack and changes nothing else. The entity stays dying,
so `nge_entity_destroy()` on it still returns 0 and it can't be pushed twice.

**Release** asserts that the ID is valid and dying and that it owns nothing, takes it
off the used list, clears the flags byte and component bits and pushes the ID on the
free stack. It
doesn't check the ID was taken, and it leaves the owner and data tables alone. The
ID is the next one created, as the free stack is last in, first out.

**The stacks can't overflow.** Each holds at most `NGE_ENTITY_MAX` entries, which is
every entity, and an ID is on at most one of them at a time.

### 4.4 Deferred destruction

An entity is destroyed in the middle of something. A hit is being resolved, a timer
tick is part way through its entities, and events about it are in the queue. So
destruction has two steps, an end to what the entity does now and an end to its ID
later:

```text
 play_turn()                                     play_step()
 --------------------------------------------    --------------------------------
 a handler: health_damage()                      nge_event_process()
   posts EV_DIED                                   handlers read the dying entity
   nge_entity_destroy(target)                    change_level(), if requested
     dying: the alive loops skip it              level_release_dying()
   the tick's later timers skip it                 its ID is free from here on
 nge_event_process()                             nge_ecs_check_guard()
   handlers read the dying entity
 ...up to 32 advances of the clock...
```

**What a dying entity is to everything else:**

| Who | What happens |
|---|---|
| `NGE_ENTITY_FOR_EACH_ALIVE`, `NGE_OWNER_FOR_EACH_OWNED` | Not visited |
| `NGE_ENTITY_FOR_EACH_IN_USE` | Visited |
| `nge_location_find()`, `NGE_LOCATION_FOR_EACH_AT`, `nge_block_find()`, `nge_block_line()` | Not found, so it blocks nothing and can't be walked into or picked up |
| `nge_timer_advance()` | Doesn't fire, even later in the same tick |
| `nge_render_update()` | Not drawn, and `nge_render_follow()` no longer moves the camera |
| Game code that loops by ID (`inv_next()`, `inv_equipped()`) | Tests `NGE_ENTITY_IS_ALIVE()` itself |
| `effects_game_affectable()` | Refuses it, so effects don't act on it |
| Event handlers | Can read its flags, components, owner and data |
| `nge_owner_count()`, `nge_owner_move_all()` | Counted and moved, as an owner must still pass it on |
| Checked setters, `nge_owner_set()`, `nge_location_set()`, `nge_timer_add()` | Accepted, for clean-up. They don't bring it back |

Code that loops by ID sees dying entities unless it checks. A handler that needs a
living entity must check too: the ID in an event stays valid, but the entity may no
longer be alive.

**The clean-up loop.** `NGE_ENTITY_RELEASE_EACH_DYING` runs until the stack is empty:

```c
NGE_ENTITY_RELEASE_EACH_DYING(id) {
    ...clean up; may destroy other entities...
}
```

It is built on `nge_entity_next_dying()` and `nge_entity_release()`. The release is
in the loop's increment, so a body can't forget it and `continue` is safe. A `break`
or `return` would lose the entity taken. The ID is released after the body, so inner
loops over the used list have finished by then (R24).

Each entity taken is released in the same pass, and each destroy can only add an
alive entity, so the loop ends. Entities destroyed during clean-up go on the same
stack and are handled in the same loop, last first (R16).

**What the game's clean-up does.** `level_release_dying()`, in common memory as it
runs every frame, does this for each entity:

1. A feature with a location has its cell redrawn.
2. `inv_drop_all()` drops the items it holds on its tile, if it has a location.
3. Any alive non-item it owns, such as an active effect, is destroyed. A persistent
   one would refuse, and that asserts (level_transitions.md, R10b).
4. Whatever is left, items held off the map and effects already dying, passes to its
   own owner with `nge_owner_move_all(id, NGE_OWNER_OF(id))`.
5. The loop releases it.

Step 4 is what makes the order of release not matter. Dying entities are released in
the reverse of the order they were destroyed, so an owner may go before or after what
it owns. If the owner goes first, what it owned moves up the chain, to an owner that
is still in use because its own contents must be empty before it can go. If
what it owned goes first, the owner simply has one thing less.

**When it runs.** `play_step()` calls it once a frame, after the frame's events and
any change of level. `level_leave()` also calls it, so that the old level's entities
are free before the new level is generated (level_transitions.md, D5). That call is
inside `change_level()`, after the frame's events have been handled, and it leaves
nothing named in the queue. Parking can then end effects, which go dying and post
`EV_EFFECT_ENDED`. `change_level()` handles those events before `play_step()` reaches
its own clean-up.

**How long an entity stays dying:**

| Destroyed during | Released |
|---|---|
| A play frame | At the end of that frame, however many clock advances it ran |
| A chooser in a command | When the chooser returns and the frame reaches `play_step()`, as chooser frames run no world step |
| A frame that ends the game | At the end of that frame: `play_step()` still runs |
| Anything left when a game ends | Nothing releases on the title or game-over screens. The next game's `nge_entity_init()` clears it |

**What deferring costs.** A dying entity still holds its ID. Within a frame, dying
entities don't count as free, so a creation can fail although entities have been
destroyed (section 8).

### 4.5 The used list and loops

`nge_entity_used[]` holds `nge_entity_used_count` IDs in no order, and
`nge_entity_used_pos[]` holds each one's index. Create appends and release moves the
last entry into the gap, so both take constant time. Destroy doesn't touch the list,
so entries move only during clean-up.

There are three loops, for three different jobs:

| Loop | How it works | Visits |
|---|---|---|
| `NGE_ENTITY_FOR_EACH_ALIVE(i, id)` | Walks the list, skipping anything not alive | Alive entities |
| `NGE_ENTITY_FOR_EACH_IN_USE(i, id)` | Walks the list, unfiltered | Alive and dying entities |
| `NGE_ENTITY_RELEASE_EACH_DYING(id)` | Empties the dying stack | Dying entities, including those added by its own body |

`NGE_OWNER_FOR_EACH_OWNED` is the alive loop with a filter on the owner. Both list
loops go last to first, and each check is made when the loop reaches the entry. `i`
is the loop's own: nested loops each need one.

What a loop body may do:

| In the loop | Result |
|---|---|
| Destroy the current entity | Safe |
| Destroy an entity not yet visited | Safe. The alive loop skips it, and the in-use loop visits it as dying |
| Destroy an entity already visited | Safe, nothing changes |
| Create an entity | Safe. It is appended behind the loop, so it isn't visited |
| Change an entity's owner | Safe. The owned loop goes by its owner when it gets there |
| Release an entity | Forbidden (R24) |

No entity is visited twice, as the index only goes down and entries don't move. A body
that calls something which may destroy the current entity tests
`NGE_ENTITY_IS_ALIVE(id)` before going on with it.

Loops by ID, such as `nge_timer_advance()` and `NGE_LOCATION_FOR_EACH_AT`, go up
through the IDs and test each one when they reach it. Their rules are the same, except
that a new entity may be visited if its ID is higher.

### 4.6 Ownership

| Owner value | Meaning |
|---|---|
| 0 to `NGE_ENTITY_MAX`-1 (0-99) | That entity |
| `NGE_OWNER_WORLD` (100) | In the game but not placed. New entities start here |
| `NGE_OWNER_AREA(a)` (101-164) | Placed in area a, a depth of the dungeon |

An owner value is one byte, with the entity numbers as they are, so an entity can be
compared with its owner directly. `NGE_OWNER_COUNT` must stay at or below 255, which
is checked at compile time.

`nge_owner_of[]` is the only record of ownership (R20). To find what an owner has,
walk the used list with `NGE_OWNER_FOR_EACH_OWNED`. That is cheap at this many
entities, and ownership changes rarely.

| Function | What it does | Dying entities |
|---|---|---|
| `nge_owner_get()`, `NGE_OWNER_OF()` | The owner, checked or not | Answers |
| `nge_owner_set()` | Gives an entity a new owner | Either may be dying |
| `nge_owner_owns()` | Is this owner on its chain? | Answers |
| `nge_owner_count()` | How many it owns directly | Counted |
| `nge_owner_move_all()` | Moves all it owns directly | Moved, and may be the target |
| `NGE_OWNER_FOR_EACH_OWNED` | Walks what it owns directly | Not visited |

`can_own()` accepts the world, an area or an entity in use. `is_or_under()` walks a
chain to refuse cycles (R21). It stops after `NGE_ENTITY_MAX` + 1 steps and asserts if
it hasn't reached the end, as that means the table is corrupt.

Because the chain has no cycles, `level_holds()` can walk it with no limit of its own.

An owner and a location are separate. An entity on a map is owned by its area and has
a location. One that is held has no location (items.md, section 4.3).

### 4.7 Components

A handle packs the table in its high byte and the bit in its low byte, so
`NGE_ENTITY_HAS_COMPONENT()` with a constant handle is one byte test at a fixed
address plus the ID. A handle names one component and must never be ORed with
another. [component_tables.md](component_tables.md) has the reasoning.

There are 3 tables of 8, which is 24 components. nge has 4 and the game 12
([components.h](../src/game/components/components.h)), so 8 are free.

| Component | Number | Data | Where the data is | Added by |
|---|---|---|---|---|
| `NGE_COMPONENT_LOCATION` | 0 | Column, row, pixel in the tile | Slot 0 | `nge_location_set()` |
| `NGE_COMPONENT_SPRITE` | 1 | Pattern, animation, motion | Page 53, the render system's | `nge_render_add()` |
| `NGE_COMPONENT_BLOCK` | 2 | What it blocks, what it can enter | Slot 0 | `nge_block_add()` |
| `NGE_COMPONENT_TIMER` | 3 | Count, period, cycles, kind | Slot 0 | `nge_timer_add()` |

**The bit is the truth.** Release clears the bits and leaves the data, so going needs
no work per component and no destroy hook. A table's old values remain under a free
entity's ID (R27), and anything searching a table directly must test the state and
bit, as `nge_location_find()` does after its `memchr`.

The checked functions assert on the ID, on the state and on the handle being one bit
in a table that exists. The setters want an entity in use. The tests,
`nge_entity_has_flags()` and `nge_entity_has_component()`, check only the ID, and
answer 0 for a free entity.

### 4.8 Systems

**The timer system** walks IDs and acts only on alive entities with the component.
Handlers may destroy anything, their own entity included. The loop tests each entity
just before firing it, so an entity destroyed earlier in the tick doesn't fire. A
timer goes with its bit, so nothing needs removing when its entity dies.

**The render system** walks the alive entities once a frame, in one banked call, and
draws what is alive, located and owned directly by the view's area. It keeps one ID
in its own data, the entity it follows, and tests that it is alive at each update. A
test can't tell a reused ID from the original (D8), so it must not follow anything
that can go. The game follows only the player.

Because they see alive entities only, neither needs telling when one is destroyed.

### 4.9 The ECS data page

Page 50 stays in slot 0 for the whole game. Its tables start at `$0100`, and the first
256 bytes are the guard area.

`nge_ecs_init()` maps the page, fills the guard with a pattern that varies by address
and calls `nge_entity_init()`. `nge_ecs_check_guard()`, compiled out with `NDEBUG`,
aborts if the pattern has changed, and `play_step()` calls it once a frame. The
pattern stays broken until something checks it, so a null write made during a chooser
is caught when the frame reaches `play_step()`.

`*_cdata.c` files hold uninitialised variables only, with the marker
`/* nge-dataseg: PAGE_50 */`, and `scripts/check-banks.sh` checks that their symbols
landed in the page. See "ECS data in slot 0" in [memory_map.md](memory_map.md),
including when slot 0 is mapped out.

**Starting up:** `nge_hw_init()`, then `nge_ecs_init()` before any other ECS call.
**A new game** calls `nge_entity_init()` alone from `play_start()`: the page
stays mapped and the guard stays as it is. Init resets the free and dying stacks and
the used list, so anything left dying is simply gone, with no clean-up. Components
without an init of their own need none, as every bit is 0.

### 4.10 Capacity

| Limit | Value | Set by |
|---|---|---|
| Entities | 100 | `NGE_ENTITY_MAX`, at most 250 |
| Areas | 64 | `NGE_OWNER_AREA_MAX` |
| Component tables | 3, so 24 components | `NGE_COMPONENT_TABLES` |
| Game flags | 5 | The flags byte |

`NGE_OWNER_COUNT` is entities + 1 + areas, at most 255, so with 64 areas the
real limit on entities is 190. Both limits are checked at compile time.

The entity and owner tables take 601 bytes in page 50, and the bookkeeping 302 in
page 49, at 100 entities.

### 4.11 Banking

The lifecycle runs in page 48 with page 49 in slot 7, and those calls happen when
something changes, not per entity per frame. The tables are in slot 0, so the
lifecycle can write them and per-frame code can read them with no banked call. The
bookkeeping is in page 49 because only the lifecycle uses it (N-DATA-1).

`nge_entity_check_in_use()` is in common memory so that both sides can use it.

## 5. Decisions

- **D1 Tables, not structs.** Each field is a byte table indexed by ID, the cheapest
  access on the Z80. *Rejected:* an array of structs, which needs a multiply or
  pointer arithmetic for each read.
- **D2 Destruction has two steps.** An entity stops acting at once, and its ID stays
  taken until the end of the frame. Events and timer ticks can then go on naming it,
  with no check that the ID still means the same thing. *Rejected:* freeing at once,
  since a new entity could take the ID while an event still names the old one
  (level_transitions.md, D4, makes the same argument for a change of level).
- **D3 Dying entities are out of every system's sight.** Systems test only for alive,
  and don't each need a rule for dying. So a dead creature is gone from the map in the
  frame it dies, while its data is still there for the messages about it.
- **D4 The game runs the clean-up.** nge can't know what a dead entity's contents
  should do, and may not include game code (N-LAYER-2). A loop in the game is plain
  code. *Rejected:* a callback from nge, which adds a pointer to do what one loop does.
- **D5 There is no way back from dying.** Anything that has seen the entity die, such
  as an event or an active effect's end, would otherwise have to be undone.
  An entity that shouldn't die is persistent instead.
- **D6 The dying stack is last in, first out.** It is the cheapest structure, and
  entities destroyed during clean-up are handled in the same loop. The order means
  nothing, thanks to D7.
- **D7 An entity must own nothing when released.** So no owner value ever names a free
  entity, and an ID can be reused with nothing left pointing at it. Passing contents
  to the entity's own owner works in any release order, even when that owner is dying
  too (R22).
- **D8 IDs have no generation count.** They are bytes used as table indexes, and a
  count would widen every ID and table lookup. So an ID can be reused the moment it is
  released, and nothing can detect a stale one. The rule is G-ENT-3: an ID kept in data
  must belong to something that lives at least as long, or be cleared when it goes.
  An active effect records no source for this reason (effects.md, D3).
- **D9 Ownership is one table, with no lists.** It costs one byte per entity and can't
  fall out of step. *Rejected:* a list per owner, which needs memory per owner and
  updates at every change, to speed up a walk that is rare and short.
- **D10 Persistence is a refusal in `nge_entity_destroy()`.** Teardown can then
  destroy everything in an area and keep what must stay, with no special case in each
  caller. Killing a persistent creature is one line: clear the flag first
  (level_transitions.md, R10).
- **D11 Release clears bits, not data.** nge can't list the game's tables, and the
  systems' data is in their own pages, so clearing them all would need a hook per
  component. Add functions set their fields instead (R27).
- **D12 Entities leave the list at release, not at destroy.** Destruction is then
  deferred for the list as well as the ID, so a loop's entries can't move under it and
  any call a loop makes may destroy anything. The alive loop tests each entry, which
  costs one flag read per entity. *Rejected:* removing at destroy, which needed every
  loop to destroy only its current entity, a rule `level_leave()` broke through
  `effect_end()`. *Rejected:* looping by ID where it matters, which costs 100 steps a
  walk and leaves the trap for the next loop. *Rejected:* copying the IDs first, up to
  100 bytes of stack a loop. Removal swaps in the last entry, as the list has no order
  to keep, and `nge_entity_used_pos[]` is page data because only the lifecycle needs it.
- **D13 The tables are in slot 0, with a guard.** Hot data needs no banked call and
  leaves common memory for code, at the cost of losing the crash that a null write
  would cause on ROM (memory_map.md). The guard buys that back in debug builds.
- **D14 Macros and functions for the same thing.** Per-frame loops can't afford a call
  and a check per entity. Everything else gets the check.
- **D15 Three loops, named for what they do.** Alive entities are what nearly every
  loop wants, the entities in use are the same walk unfiltered, and emptying the dying
  stack isn't a walk at all, as the loop adds to what it consumes.

## 6. Not built yet

| Mechanic | What's there, what's missing |
|---|---|
| Saving a game | Most ECS state is in page 50, so one page holds nearly all of it. The stacks and `nge_entity_used_pos[]` are in page 49, and the render system's sprite data in page 53, so those must be saved too or rebuilt |
| The game's flags | Five are defined, and nothing uses them |
| Interrupts | A future IM2 handler must not read slot 0 (memory_map.md, "When slot 0 is mapped out") |

## 7. Tests

`test_entity` covers:

- init, create, IDs in order, and running out;
- destroy: dying, kept flags, a second destroy, persistent entities;
- next dying and release: last in, first out, and the ID reused first;
- flags, the handles, and the component bits at each table's ends;
- the loops: destroying the current entity, destroying others ahead of the loop and
  behind it, creating others, nothing visited twice, and the in-use loop seeing the
  dying;
- clean-up that destroys more entities, emptying the stack, with `continue` in the
  body;
- the asserts: release of an alive entity, destroy of a free or invalid ID, state bits
  in flags, handles of no component, of two, and of no table;
- 20,000 random operations against a reference model, checking the used list after
  each.

`test_owner` covers:

- the owner values, new entities in the world, and a reused ID;
- set, owns (direct and indirect), count, move all, and the owned loop, including
  destroying another entity from inside it;
- refused cycles, and invalid or free owners;
- release that still owns something, a dying owned entity, and a dying entity moving;
- clean-up of a chain `NGE_ENTITY_MAX` long, owners first and owners last.

`test_ecs` covers mapping page 50, the guard's fill and pattern, and the check.

`test_location`, `test_block`, `test_timer` and `test_render` cover the components and
systems, including not seeing dying entities.

`test_level` covers `level_release_dying()`: dropped items, effects going with a
target, and what happens to persistent entities held by something that dies.

`scripts/check-banks.sh`, part of the build, checks that the `*_cdata.c` tables are in
page 50 between `$0100` and `$1FFF`.

## 8. Known risks

- **Stale IDs.** No check can tell a reused ID from the original (D8).
  `nge_entity_destroy()` on a kept ID whose entity was released would destroy whatever
  has it now, and `NGE_ENTITY_IS_ALIVE()` would say yes. This depends on following
  G-ENT-3.
- **Release builds have no checks.** With `NDEBUG` the asserts are gone, so:
  - a release that still owns something leaves owners naming a free ID, which walking
    the chain then follows;
  - a release of an alive entity frees it with no clean-up;
  - a double release pushes the ID twice, so two entities can share it, and takes
    another entity off the used list;
  - an ID out of range reads and writes past the tables.

  G-ENT-2 keeps release in one place, which is the real protection.
- **Releasing inside a loop.** Release moves an entry, so one inside a loop over the
  used list would bring back the revisits that D12 removed. Nothing checks this. Only
  `level_release_dying()` releases, and neither of its callers is in such a loop. Calling
  it from one would be the mistake to watch for.
- **Leaving the clean-up loop early.** A `break` or `return` in
  `NGE_ENTITY_RELEASE_EACH_DYING` leaves the entity taken dying, on no stack, and its ID
  never comes back. It stays in the used list, so nothing detects it.
- **Dying entities hold IDs.** The free count doesn't include them, so with up to 32
  advances in a frame a busy frame can fail to create an entity although several have
  been destroyed. `level_leave()` avoids the largest case. A refused timed effect may
  go unreported (effects.md, section 8).
- **Entities are few.** The player's pack, parked entities, features, items and each
  active effect all take one of the 100.
- **A handle is one component.** ORing two handles in different tables tests the wrong
  bits. The checked functions assert on it, and the macros don't.
- **The guard is partial.** It catches writes only, up to 255 bytes past null. A write
  further on lands in the tables, a read returns the pattern, and release builds don't
  check at all.
- **zsdcc.** `NGE_COMPONENT()` must keep its unsigned shifts: zsdcc types `1 << 7` as a
  signed char, which would make table 255. The PC tests can't catch that being lost.
