# Design patterns

Each pattern is written up once, in the header or document that owns it.
These rules point to the write-up and list the points a review checks. When
a pattern changes, change its write-up, and this list if a check point
changes. See [README.md](README.md) for the format and strengths.

### P-0 · must · New code of a kind follows its pattern, and a new pattern gets a write-up
Code of a kind listed here (a new action, generator, component, effect,
event, screen) follows the write-up named. A new kind of code that others
will copy gets a write-up in its owning header and a rule here in the same
commit.

### P-ACT-1 · must · Actions follow game/actions/act.h
- One function `act_<verb>(act_t *a, uint8_t mode) NGE_BANKED`, in
  `actions/act_<verb>.c` (page 112), with an `ACT_<VERB>` number and a case
  in `act_do()`.
- Four steps in order: context, validate, resolve, apply. Check mode
  (`ACT_CHECK`) returns after validate. It changes nothing, posts nothing
  and draws no random numbers.
- It ends through `act_fail()`, `act_succeed()` or `act_checked()`. Each new
  reason is an `ACT_R_*` with its row in `act_reason_flags[]` (`act.c`) and
  its text in `messages_act_failed()`.
- It posts events only in apply, after the change. It doesn't call other
  actions: callers combine them (bump to attack).
- Its header comment lists its inputs, outputs, events and reasons.

### P-GEN-1 · must · Generators follow game/gen/gen.h
- A class has a kind record type (`gen_<class>_kind_t`, `gen_internal.h`),
  kinds in `gen_kinds.h` (`<CLASS>_COUNT` ends each group), and a `const`
  table with designated initialisers in its odd page
  (`gen_<class>_pdata.c`).
- The generator (`gen_<class>_bank.c`) copies the record to the stack
  before calling other pages, creates the entity with `gen_begin()` and the
  owner it is given, adds every component the kind needs, and never sets a
  location.
- Kind data that never changes is read through a copying function
  (`gen_name()`, `gen_feature_info()`), not kept per entity.

### P-COMP-1 · must · Components follow game/components/components.h
The mechanism is in `ecs/components/nge_components.h`, the game's shape
in `game/components/components.h`.
- A new component's mask is added to the one list in `components.h`, as
  `NGE_COMPONENT(NGE_COMPONENT_USER + COMPONENT_COUNT)`, and
  `COMPONENT_COUNT` goes up by one. No mask is defined anywhere else. An nge
  component is added to the list in `nge_components.h`, below
  `NGE_COMPONENT_USER`.
- Its tables are one per field, `[NGE_ENTITY_MAX]` (N-DATA-3), in
  `components/<name>_cdata.c` (slot 0) when common code or several systems
  read them, else in its module's page data. They are read-only outside
  the component.
- `<name>_add()` gives an entity the component from a kind record (or a
  start state). Its functions hold its rules (G-ARCH-1), are banked in page
  110, and check their entity with asserts.
- Code outside the component tests for it with the macro in its header
  (`ITEM_IS()`, `FEATURE_IS()`, `INV_HAS()`), not the mask.
Check: `grep -rn "NGE_COMPONENT(" src` finds masks only in the two lists
(and `NGE_COMPONENT()` itself in `ecs/nge_entity.h`).

### P-EFFECT-1 · must · Effects follow game/effects/effects.h and effects_core.h
- A new effect is a row in `effects_pdata.c` named from a kind's hook. Only
  a new kind of op needs code, in `effects_game.c`.
- The core includes only `effects_core.h`, nge headers and `game/events.h`
  (`make test` checks it). Game knowledge goes through the seam.

### P-EVENT-1 · must · Events follow game/events.h and event/nge_event.h
- An event is named in the past tense, `EV_<WHAT_HAPPENED>`, and has a row
  in the table in `events.h` (target, source, payloads).
- It is posted after the change it reports. A refused action posts
  nothing.
- Handlers are in common code, called from `game_on_event()` in a fixed
  order. They copy the event's fields into locals (T-ZSDCC-1) and only react
  (G-ARCH-3, G-ARCH-6).

### P-TIMER-1 · must · Timer kinds follow game/timers.h and ecs/systems/nge_timer.h
- A kind is a `TIMER_*` number with a row in the table in `timers.h` and a
  case in `game_on_timer()` (`game.c`).
- A handler for entities that can be off the current level skips them
  (`level_holds()`, P-LEVEL-1).

### P-FEAT-1 · must · Features follow game/components/feature.h
- A new feature is an `FT_*` kind and a record in `gen_feature_pdata.c`:
  its looks, its `FCAN_*` abilities, what it blocks when closed.
- It is drawn as a map tile, never a sprite. Its tile is set to block
  nothing in `level_bank.c`, and the feature itself blocks.
- `feature_place()`, `feature_open()` and `feature_close()` keep the
  blocking and the tile in step. Actions check the rules first.

### P-UI-1 · withdrawn · Item screens follow game/ui/ui.h
*Withdrawn* (2026-10-19): item screens are choosers (P-CHOOSE-1). The old
text follows.
A new item screen is a row in the description table (`ui_bank.c`), a
`UI_*` number and a game state. It returns to `GAME_PLAY`, whose entry
action redraws what it covered.

### P-STATE-1 · withdrawn · Game states follow game/game.h
*Withdrawn* (2026-10-19): the game has no states any more (P-LOOP-1,
P-CHOOSE-1). The old text follows.
Each state has an entry action and a frame update, called from the
switches in `game.c` (a banked state's functions are called directly,
never through a table of pointers). Updates never wait: they take what
they need from the key events and return. The exceptions run frames
themselves until the player answers: the title's name prompt, and
targeting (`target_choose()`, game/target/target.h), whose frames do only
`game_frame_start()` and `nge_render_update()`.

### P-LOOP-1 · must · Loops that run frames follow game/game.h
- Every loop that runs frames calls `game_frame_start()` first and
  `game_frame_end()` last in each frame, or waits through
  `game_wait_key()`, which does both. The one other frame is an overlay
  screen's paused frame, `game_pause_frame()`, run only through
  `screen_frame()` and `screen_wait_key()` (game/ui/screen.h). There is no
  other frame loop.
- Only the play loop (`play_run()`) steps the world: the turn engine, the
  frame's events, a change of level, the clean-up and the guard check.
- The design and its requirements are in `docs/game_loop.md`.

### P-CHOOSE-1 · must · Choosers follow game/game.h
- A chooser (`screen_inv_run()`, `player_ask_direction()`,
  `target_choose()`) is called by a command, during the player's turn. It
  throws away waiting key events and shows its prompt or screen. A prompt
  waits with `game_wait_key()` and puts back play's help
  (`game_play_help()`); a screen is an overlay screen (P-SCREEN-1). It
  throws away waiting key events again, and returns the answer.
- It only chooses: it does no action and posts no events. The command does
  the action with the answer, through `player_act()`.
- Its answer to a key is a step function on one key event
  (`player_direction_key()`, `target_key()`, the inventory's `key()`), with
  PC tests: a screen's through its run function, the keys given one a
  paused frame by the test's `game_pause_frame()`.
- An action the screen offers is first asked in check mode (`ACT_CHECK`),
  and a refusal's reason shows on the screen (`screen_message_to()`).

### P-SCREEN-1 · must · Overlay screens follow game/ui/screen.h
- An overlay screen is a run function in its own file in `game/ui/`
  (`screen_<name>_bank.c`, page 116 while it fits). It calls
  `screen_open()`, opens its panels from a table (`ui/nge_panel.h`) on
  `SCREEN_BUFFER_OVERLAY`, covering rows 0-30, and draws them; waits with
  `screen_wait_key()` or `screen_frame()`; and closes its streams, then
  calls `screen_close()`, on every path.
- It draws only on `SCREEN_BUFFER_OVERLAY`, never on the main screen, and
  sets the help line (`screen_help()`) when it opens and when its context
  changes.
- What it tells the player comes from `messages/` (G-FLOW-3): sent to its
  own panel with `screen_message_to()` while it must show there.

### P-LEVEL-1 · must · Level changes follow game/level/level.h
- Leaving the player's level is a request (`level_request()`), carried out
  at the end of the frame. A non-player leaves with `level_send()`.
- Kept entities (persistent, or owned by a persistent one) are parked in
  their depth's area, frozen. Everything else on a level is gone when it is
  left.
- A persistent entity owned by another entity is an item (asserted in
  `level_release_dying()`).
The requirements are in `docs/level_transitions.md`.

### P-MOD-1 · must · Banked modules follow docs/memory_map.md
"Banked game modules" and "Banked code" (N-BANK-1 to N-BANK-7, G-MOD-1,
G-MOD-2).

### P-INIT-1 · must · Module initialisation follows src/nge/README.md
"Module initialisation" (N-INIT-1), for game modules as for nge's.
