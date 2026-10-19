# Game rules

Rules for the game in `src/game/`. The architecture they come from is in
[src/game/README.md](../../src/game/README.md). Banking and data placement
follow [nge.md](nge.md), and the design patterns are in
[patterns.md](patterns.md). See [README.md](README.md) for the format and
strengths.

## Modules and files

### G-MOD-1 · should · A banked module has its own folder, split by memory
`src/game/<module>/` holds:
- `<module>.h`: the interface;
- `<module>.c`: common code (per frame, per turn, handlers) and data
  others read;
- `<module>_bank.c`: the banked code, if it is one file;
- `<module>_<part>.c` (`level_gen.c`, `level_travel.c`) or one file per
  item (`act_move.c`): banked code split by topic, all in the module's
  code page;
- `<module>_pdata.c`: its page data;
- `<module>_internal.h`: shared between these files only.

A module that isn't banked is a single `src/game/<module>.{h,c}`. See
"Banked game modules" in `docs/memory_map.md`.

### G-MOD-2 · must · A banked module's pages come from the game range
The module takes the next free even/odd pair from 102-131 in
`src/banks.inc`, with its origin, and gets a row in the page table in
`docs/memory_map.md` and in `src/game/README.md`.

### G-MOD-3 · should · Game components are in components/
A component is `components/<name>.{h,c}` (its functions, banked in page
110) and `components/<name>_cdata.c` (its tables in slot 0). See P-COMP-1.

## Naming

### G-NAME-1 · should · Each module has one prefix
A module's public functions, globals and constants start with one prefix,
its name or a documented short form (`act_` for actions, `inv_` for the
inventory, `gen_` for generators). File-local names have no prefix
(C-NAME-4).
Check: globals in a module's `*_cdata.c` and `*_pdata.c` files, and
functions in its header, all start with the same prefix.

### G-NAME-2 · should · Value groups have short, unique prefixes
Kinds and IDs that appear everywhere use short group prefixes (`CR_`,
`IT_`, `FT_`, `EF_`, `EV_`, `ACT_`, `ACT_R_`, `DMG_`, `SLOT_`). Each is unique
and listed with its group (`gen_kinds.h`, `events.h`, `act.h`).

## Architecture

The rules of "Architecture" in `src/game/README.md`, numbered as there.

### G-ARCH-1 · must · A rule lives in one place
A game rule (how damage is taken, whether an attack hits) is a function of
its component, called by every action and effect that needs it. Nothing
works the rule out again.

### G-ARCH-2 · must · Actions orchestrate; they don't own rules
An action finds and checks its target, then calls component functions.
Anything an actor chooses to do, the player or a monster, is an action.

### G-ARCH-3 · must · Reactions go in event handlers, not in actions
An action posts what happened (`EV_DIED`). Messages, the screen and game
over each react in their own `on_event()`. A new consequence is a new
handler, not a change to the action.

### G-ARCH-4 · should · Systems only for batch work
A system serves "every entity with these components, every frame or tick".
Finding the one entity that wants to act is an action started by its timer
or a key.

### G-ARCH-5 · must · Effects over time are entities with timers
Poison, regeneration and the like are active effects (P-EFFECT-1), owned by
what they affect, with a timer. They are not counters in a component.

### G-ARCH-6 · must · Derived data is kept by the code that changes it
Attributes are worked out by `effects_recompute()`, called directly by
whatever changes their inputs. Never by an event handler: events can be
dropped or arrive late.

### G-ARCH-7 · must · Behaviour is chosen by number
Timer kinds, action IDs, event IDs, effect ops: a number, dispatched by a
`switch` in common code (N-BANK-6).

## Entities

### G-ENT-1 · must · Every entity is created with an owner
Code that creates an entity gives it its owner straight away (the
generators take the owner as an argument). An entity on a map is owned by its
area and has a location. An entity that is held has no location.

### G-ENT-2 · must · Destroy, never release
Game code calls `nge_entity_destroy()`. Only the clean-up,
`level_release_dying()`, releases entities, at the end of the frame after
the frame's events are handled, so no event names a freed ID.

### G-ENT-3 · must · Don't keep entity IDs that can outlive the entity
An ID kept in data (a component table, an event's payload held past the
frame, a static variable) must belong to something that lives at least as
long, or be cleared when it goes. IDs are reused at once. An active effect
records no source for this reason.

### G-ENT-4 · must · The player is identified by KIND_IS_PLAYER() or actor_player
Rules that treat the player differently test `KIND_IS_PLAYER(id)` in
components and actions (no dependency on `actors.h`), or `actor_player` in
game, player and message code. Never test the persistent flag for this:
other entities are persistent too.

## Game flow

### G-FLOW-1 · must · Change level only by a request
Code that moves the player to another depth calls `level_request()`. Only
`change_level()` in `game.c` and the new game call `level_generate()`. See
P-LEVEL-1.

### G-FLOW-2 · withdrawn · Change state with game_set_state() or game_over()
*Withdrawn* (2026-10-19): the game has no states any more
([docs/game_loop.md](../game_loop.md)). A game ends through `game_over()`
(G-FLOW-4), and the player chooses through choosers (P-CHOOSE-1).

### G-FLOW-3 · should · What the player is told comes from messages/
Texts for events and refused actions are in `messages_bank.c`
(`messages_on_event()`, `messages_act_failed()`). Other modules call
`screen_message()` only for their own prompts and screens.

### G-FLOW-4 · must · A game ends only through game_over()
Code that ends the game (the player dies, quits, escapes) calls
`game_over()` with a `GAME_OVER_*` reason. It stops the clock, so nothing
more acts, and the play loop ends at the end of that frame, once the
frame's events are handled. Nothing else ends the play loop.

## Development aids

### G-DEV-1 · must · Development aids are under #if GAME_DEV_KIT and listed in dev.h
Starting kits, debug keys, test entities and fixed seeds are compiled only
with `GAME_DEV_KIT`, and each is listed in `game/dev.h` and in the `dev.h`
row of `src/game/README.md`.

## Tests

### G-TEST-1 · should · Game logic has PC tests
Components, actions, generators, effects, the level and the item screens
have tests in `tests/`, linked through their lists in `tests/Makefile`.
New game code with rules goes in the matching test, or a new one.
