# Game loop: requirements and design

Status: steps 1 to 4 implemented (section 6 describes the code as built,
section 9 lists where it differs from the proposal). The CSpect checks
passed on 2026-10-19. Replaces the game state machine in `game/game.c` with
structured flow: a title, a game and a game-over screen run one after
another, and the player's screens and prompts are calls that return the
player's choice. Also sets the requirements that later work (a demo mode,
action animations, a UI layer that doesn't scroll with the map) must be
able to meet, so this design leaves room for them.

## 1. Goal

The game is turn-based. Whenever the player is deciding (a command, an item
to equip, a direction, a tile), the world is frozen. The loop should say
that directly:

- a game is one loop that steps the world, and nothing else does;
- asking the player is a function call that returns their answer, and the
  command that asked carries on in order;
- every frame keeps the screen live, whatever is waiting.

## 2. Terms

| Term | Meaning |
|---|---|
| Frame | One pass between vertical blanks, 50 a second |
| Drawing | The part of a frame every loop does: wait for vblank, upload sprites, redraw the map if it changed, read the keyboard (frame start); build the next sprites (frame end) |
| World step | The part of a frame only a game in progress does: the turn engine, handling events, a change of level, releasing dying entities, the ECS guard check |
| Play loop | `play_run()`: the frames of one game, from the first level to game over. The only place world steps run |
| Chooser | A function that shows a screen or prompt, runs frames until the player answers, and returns the answer: `ui_choose_item()`, `player_ask_direction()`, `target_choose()` |
| Command | What the player's key does on their turn (`player_command()`): it may call choosers, then does an action |
| Command source | Where the player's commands come from: the keyboard, or later a demo's AI |
| Overlay sprite | A hardware sprite the game owns above the render system's 0 to 63, not an entity: the targeting cursor, later projectiles and effects |

## 3. What is wrong with the current loop

- **World steps run in every state.** `post_update()` handles events,
  checks for a change of level, releases dying entities and checks the
  guard on the title and game-over screens too, where there is no world.
  It is harmless only because there is nothing for them to do. The title
  also redraws whatever map was set up behind it.
- **Choosing is split across a state machine.** Each item screen and
  direction prompt is a game state with:
  - a case in two switches;
  - an entry action that draws;
  - an update that only forwards one key;
  - a key function that does the action;
  - a return to `GAME_PLAY`, whose entry action redraws everything.

  That is 7 of the 11 states. The parts are in three modules (`game.c`,
  `ui_bank.c`, `player_bank.c`).
- **State that should be local is worked out twice.** `ui_items_key()`
  collects the item list again, because the list `ui_items_enter()` drew
  didn't survive into the next frame.
- **Choosing and acting are mixed.** The item screens do the action
  themselves, while targeting (`target_choose()`) returns only the tile and
  the command acts. A demo or a test can't use a screen's choice without its
  action.
- **Two patterns for one job.** Targeting already blocks, as a documented
  exception (P-STATE-1). It was simpler than the state-based direction
  prompts next to it.

## 4. Requirements

Numbered so tests and commits can name them. **Later** marks a requirement
that this design must leave room for, but that isn't built in the steps of
section 7.

### Frames and drawing

- **R1** Every frame, in every part of the program, does the drawing: the
  screen stays live (sprites animate, a moved view is redrawn) whatever is
  waiting for the player.
- **R2** The drawing is in one place, two functions (`game_frame_start()`,
  `game_frame_end()`), called by every loop that runs frames.

### The world

- **R3** World steps run only in the play loop, in a fixed order each
  frame: the turn engine, then the frame's events, then a change of level,
  then releasing dying entities, then the guard check.
- **R4** The world's time is spread over frames: at most
  `CLOCK_ADVANCES_MAX` advances of the clock a frame.
- **R5** Consequences happen in order: events are handled after each advance
  of the clock, after the player's command, and after an arrival on a new
  level.
- **R6** A game over (the player dies, quits or escapes) stops the world:
  nothing acts after it, even later in the same frame's advances. The play
  loop then ends at the end of that frame, after its events are handled, so
  the death message comes before "Game over".

### The player's turn and choosing

- **R7** No time passes while the player decides, at any screen or prompt.
- **R8** Choosers return a choice. The command does the action with it
  (`player_act()`). No chooser does an action.
- **R9** A chooser restores what it drew over before it returns: the map and
  sprites under an item screen, play's help bar.
- **R10** Keys typed for one prompt don't leak into the next: key events
  waiting are thrown away when a chooser starts and when it ends.
- **R11** A chooser's key handling is a step function that takes one key
  event (`target_key()`), so the PC tests drive it without frames, and a
  fake frame start drives the chooser itself (as `test_target` does).

### Flow

- **R12** The program runs the title, then a game, then the game-over
  screen, then the title again, for ever.
- **R13** The title shows no level: the map window is cleared and no
  entity sprites are shown. The game-over screen leaves the last level
  showing under its message.
- **R14** A new game resets every module the game uses, as
  `new_game_enter()` does today. The player's name and the message window
  are kept.

### Constraints

- **R15** No function pointers to banked code. Banked calls nest at most 8
  deep, and the stack (512 bytes) holds the deepest chain: section 6.9.
- **R16** The behaviour a player sees doesn't change in steps 1 to 4: every
  command, screen and prompt works as now.

### Later

- **R17 (later): demo mode.** The title can run a game whose player's
  commands come from an AI, not the keyboard; any key ends it and returns
  to the title. It plays a new game with a fixed seed and leaves no trace.
- **R18 (later; built in step 7, [animations.md](animations.md)): action animations.** An entity that acts plays the
  action's animation: a slide from the old tile to the new, a lunge at what
  it attacks. One that doesn't act is still or plays its idle animation.
  Actions themselves stay instant: the rules never wait for the screen.
- **R19 (later; built in step 7): pixel scrolling.** The view scrolls a pixel at a time with
  the player's slide, so the map doesn't jump a tile.
- **R20 (later): a UI layer.** The stats, messages, help bar and screens
  don't scroll with the map (R19 needs this).

## 5. Decisions

- **D1 Structured flow, not a state machine.** The top level is plain code:
  `title_run(); reason = play_run(mode); over_run(reason);`. With the item
  screens and prompts gone (D2), only these three states would be left,
  and they always follow one another. A state machine adds switches and
  entry actions to say what three calls in order say. **Rejected:** keeping
  the state machine and moving the world steps into the play update. It
  fixes the first problem of section 3 but none of the others.
- **D2 Choosers block.** A chooser runs frames until the player answers,
  like `target_choose()` and the title's name prompt today. This is sound
  because no time passes during the player's turn anyway (R7). The chooser's
  list or prompt stays in its locals, the command reads in order, and a
  new screen is one function, not a state with four parts.
- **D3 A chooser returns a choice; the command acts (R8).** This is
  targeting's split, applied to every chooser. The demo's AI (R17) and the
  tests call actions directly.
- **D4 Drawing and world steps are separate functions (R1 to R3).** Drawing
  is in `game_frame_start()` and `game_frame_end()`, which every loop
  calls. World steps are in `play_step()`, which only the play loop calls.
- **D5 One waiting function.** `game_wait_key()` runs drawing-only frames
  until a key event arrives (or a time-out) and returns it. Choosers and
  the title wait through it, so there is one copy of the waiting frame.
- **D6 Overlay sprites are a small pool the frame start uploads.** Their
  records are in common memory, and `game_frame_start()` uploads them with
  the render system's sprites. The targeting cursor is the first. Later,
  projectiles, hit flashes and death animations use them (R18): a
  killed entity is released at the end of its frame, so its own sprite
  can't play its death, and an arrow in flight shouldn't take an entity
  slot.
- **D7 Game over is a flag the play loop tests.** `game_over(reason)`
  stops the clock (R6) and records the reason. `play_run()` returns it at
  the end of that frame. There is no `GAME_OVER` state to switch to.
- **D8 Animations gate the world, they don't block it (R18, later).**
  Animations start from events (an `anim` module's `on_event()`), advance in
  `game_frame_end()`, and the turn engine waits while one that holds the
  turn plays. Actions never run frames. Section 6.10 has the design.
- **D9 The UI moves to Layer 2 (R20, later).** The stats, messages, help
  bar and screens are drawn on a 320 x 256 Layer 2 above the sprites and
  the tilemap, transparent over the map. The tilemap then holds only the
  map and can scroll freely (R19). The pages are already reserved (16 to
  31). **Fallback:** the Copper splits the tilemap's scroll at the edges of
  bands across the full width, which needs no memory but no side panel.
  A prototype decides (section 7, step 5). **Decided (2026-10-20): Layer 2.**
  The prototype (section 10) showed the layering and pixel scrolling right
  on hardware, and a message drawn in 16.6 ms, within a frame.

## 6. Design

### 6.1 The top level

```c
void game_run(void)
{
    for (;;) {
        title_run();                         /* returns when a game is to start */
        over_run(play_run());                /* the game, then why it ended */
    }
}
```

- `title_run()` (page 104): clears the map window and hides the entity
  sprites (R13), shows the title, waits for a key with `game_wait_key()`,
  asks the name (the name prompt, which already blocks), and returns. Later
  (R17): a time-out on the wait runs a demo game and shows the title
  again.
- `play_run()`: section 6.3. Returns `GAME_OVER_*`. It takes a mode (normal
  or demo) when demo mode comes (6.11), not before.
- `over_run(reason)`: shows why the game ended, waits for a key, returns.
  The last level stays on the screen (R13).

`game_state_t`, `game_state`, `game_set_state()` and both switches go.

### 6.2 Frames

```c
void game_frame_start(void);   /* wait for vblank, upload the sprites and overlay sprites,
                                  redraw the map if it changed, read the keyboard */
void game_frame_end(void);     /* build the next frame's sprites (later: advance animations) */

/* Run drawing-only frames until a key event, or frames frames (0: no limit): the event, or NGE_KEY_NONE */
nge_key_event_t game_wait_key(uint16_t frames);
```

`game_wait_key()` returns at once if an event is waiting. Every loop that
runs frames calls `game_frame_start()` first and `game_frame_end()` last:
the play loop, `game_wait_key()`, and the name prompt. The name prompt
uses the line editor's non-blocking form (`nge_line_begin()`, then
`nge_line_update()` once a frame, `io/nge_line.h`) inside a loop of the
game's frames, so nge needs no change.

A chooser called from the play loop runs inside a play frame that has
started but not ended. `game_frame_start()` ends an open frame first, so
the sprites it uploads are always built through the camera as it is, and
each frame's sprites are built once (`frame_open` in `game.c`). This is
what lets a chooser move the camera (targeting's scrolling) between waits.

### 6.3 The play loop

```c
static uint8_t play_run(void)
{
    play_start();                  /* what was new_game_enter(): every module reset, the first level (R14) */
    while (!ended) {
        game_frame_start();
        play_turn();               /* the world acts, or the player's command (6.4) */
        play_step();               /* the frame's events, a change of level, the clean-up, the guard */
        game_frame_end();
    }
    return end_reason;
}
```

- `play_step()` is today's `post_update()` without the sprite build:
  `nge_event_process()`, `change_level()` if `level_pending()`,
  `level_release_dying()`, `nge_ecs_check_guard()` (R3).
- `game_over(reason)` sets `ended` and `end_reason` and stops the clock
  (D7). The loop ends after that frame's `play_step()`, so the
  frame's events (the death message) are handled first (R6).
- `change_level()` no longer tests the state for game over: it tests
  `ended`.

### 6.4 The turn engine

As today (`pass_time()`, `show_turn()`), with one more condition later:

```c
static uint8_t world_can_act(void)
{
    return !player_turn && !level_pending() && !ended;   /* later: && !anim_holding() (6.10) */
}

static void play_turn(void)
{
    if (!player_turn) {
        pass_time();
        show_turn();
    }
    if (player_turn && !ended) {
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE) {
            player_command(ev);    /* may call choosers: they run frames until answered */
        }
        /* later (R17): in a demo, demo_command() instead, and any key ends it */
    }
}
```

`ended` in `world_can_act()` replaces the stopped clock as the reason
nothing acts after a game over. `nge_timer_stop()` stays in `game_over()`:
it also stops the advances already running in this frame.

### 6.5 Choosers

Each chooser:

1. throws away waiting key events (R10);
2. draws its screen or prompt and sets the help bar;
3. loops: `ev = game_wait_key(0)`, then its step function on `ev`, until
   the step answers;
4. restores what it drew over (R9) and throws away waiting key events;
5. returns the answer.

| Chooser | Returns | Replaces |
|---|---|---|
| `ui_choose_item(screen)` (page 116) | an item, or `NGE_ENTITY_INVALID` (cancelled, or nothing to choose: it says so) | `GAME_DROP`, `GAME_EQUIP`, `GAME_UNEQUIP`, `GAME_USE`, `ui_items_enter/update/key()` |
| `ui_show_inventory()` (page 116) | nothing: any key closes it | `GAME_INVENTORY` |
| `player_ask_direction(prompt, &dx, &dy)` (page 114) | 1 and a direction, or 0 | `GAME_FIRE`, `GAME_CLOSE`, `player_direction_enter/update/key()` |
| `target_choose(&t)` (page 124) | 1 and a tile, or 0 | (already a chooser: moves onto `game_wait_key()` and the overlay pool) |

- The item screens keep their table of descriptions (`ui_bank.c`), minus
  the action column. The list is collected once, in the chooser's locals
  (`ids[LIST_MAX]`, 25 bytes).
- The direction prompts keep their table of prompts and help texts, minus
  the action column.
- Each chooser has a step function for the tests (R11):
  `ui_item_key(screen, ids, n, ev)`, `player_direction_key(ev, &dx, &dy)`,
  and `target_key()` as now.
- Restoring: an item screen draws over the map window and hides the
  sprites, so it calls `game_play_redraw()` (today's `play_enter()`: the
  sprites' view, the map, the stats, the depth, the turn, play's help).
  Targeting and the direction prompts draw only the help bar, so they call
  `game_play_help()`.

### 6.6 Commands

Commands in `player_bank.c` read in order: choose, then act.

```c
case 'w': {
    nge_entity_id_t item = ui_choose_item(UI_EQUIP);

    if (item != NGE_ENTITY_INVALID) {
        a.item = item;
        a.kind = SLOT_NONE;   /* the first free slot it fits */
        player_act(ACT_EQUIP, &a);
    }
    break;
}
case 'c':
    if (player_ask_direction(PLAYER_PROMPT_CLOSE, &a.dx, &a.dy)) {
        a.target = NGE_ENTITY_INVALID;   /* what is in that direction */
        player_act(ACT_CLOSE, &a);
    }
    break;
```

A command's events are handled in that frame's `play_step()`, as a
state's events are today (R5).

### 6.7 What changes, module by module

| Module | Change |
|---|---|
| `game.c`, `game.h` | `game_run()` as 6.1; `play_run()`, `play_turn()`, `play_step()`, `over_run()`; `game_frame_end()`, `game_wait_key()`; `game_over()` sets a flag; the states, `game_set_state()` and both switches go. game.h's write-up describes the new flow |
| `title_bank.c`, `title.h` | `title_run()` replaces `title_enter()` and `title_update()` |
| `ui_bank.c`, `ui.h` | `ui_choose_item()`, `ui_show_inventory()` and `ui_item_key()` replace the enter, update and key functions |
| `player_bank.c`, `player.h` | commands call the choosers; `player_ask_direction()` and `player_direction_key()` replace the prompt state's functions |
| `target_bank.c` | waits through `game_wait_key()`; the cursor is overlay sprite 0 |
| `screen.c`, `screen.h` | the overlay sprite pool: `SCREEN_OVERLAY_COUNT` records in common memory, `screen_overlay_set()`, `screen_overlay_hide()`, uploaded by `game_frame_start()` |
| tests | `test_ui` drives the item screens and the direction prompt through the commands, with a fake `game_wait_key()`; `test_target` likewise |
| guidelines | G-FLOW-2, P-STATE-1 and P-UI-1 are withdrawn (IDs keep their meaning); new G-FLOW-4 (a game ends only through `game_over()`), P-LOOP-1 (loops that run frames) and P-CHOOSE-1 (choosers) |

### 6.8 Game over and the end of a game

- **Died:** `death_on_event()` calls `game_over(GAME_OVER_DIED)` while the
  frame's events are handled. The loop ends after that `play_step()`.
- **Quit:** Q's command calls `game_over(GAME_OVER_QUIT)`.
- **Escaped:** `change_level()` with `LEVEL_DEPTH_SURFACE` calls
  `game_over(GAME_OVER_ESCAPED)`.
- **Mid-advance:** the clock is stopped, so no further timer fires in that
  advance, and `world_can_act()` stops the next one (R6).

### 6.9 Call depth and stack

The deepest banked chain in steps 1 to 4 is a command that asks for an
item:

```
game_run (common) → play_run (common) → player_command (114)
  → ui_choose_item (116) → game_wait_key (common) → nge_render_update (52)
```

That is 3 banked levels of the 8 (`player_command`, `ui_choose_item`,
`nge_render_update`). Targeting's chain is 4, as now. On the stack, the
deepest is the item chooser's list (25 bytes) and an `act_t` in the
command. Both are well inside 512 bytes.

### 6.10 Later: animations (R18)

Built in step 7: [animations.md](animations.md) has the design as agreed
and built. Its D1 changes one point below: a finished sequence returns to
the rest animation in the engine, and still is a rest animation of one
pattern, not a second case.

- **Actions stay instant.** The tile, the damage and the events happen at
  once. The rules, the AI and the tests never wait for the screen.
- **Events start the animations.** An `anim` module's `on_event()` (in
  `game_on_event()`) starts:
  - a slide from the old tile on `EV_MOVED` (which then carries dx and dy
    in its payloads);
  - a lunge towards the target and back on `EV_HIT` and `EV_MISSED`;
  - the action's pattern sequence.

  Which sequences a kind has (idle, move, attack) is in its kind record.
- **They advance in `game_frame_end()`.** A slide is the render system's
  draw offset moving to 0 over n frames, a per-frame batch job for the
  render system. A finished sequence returns to the kind's idle
  animation, or to still if the idle animation has one frame.
- **The world waits for them.** `world_can_act()` adds `!anim_holding()`,
  and the player's command waits too (keys stay queued). Only entities in
  view animate: anything off-screen moves at once and holds nothing up.
  Moves at the same tick slide together.
- **Transient visuals use overlay sprites (D6):** projectiles, hit
  flashes, deaths.
- **The camera** follows the player's slide a pixel at a time (R19, R20).

The play loop's frames carry on while animations play (the turn engine
just doesn't advance), and choosers' frames call `game_frame_end()` too,
so idle animations play under the item screens and during targeting.

### 6.11 Later: demo mode (R17)

- `play_run()` gains a mode; in demo mode it starts a new game with a
  fixed seed.
- On the player's turn, `demo_command()` (later: the monsters' AI applied
  to the player) does an action directly. It calls no chooser (D3).
- Any key, or the end of the game, ends it. The title's `title_run()`
  shows the title again. The player's name is untouched.

### 6.12 Later: the UI layer and pixel scrolling (R19, R20)

- **Layer 2:** 320 x 256 at 8 bits (80K, pages 16 to 25), above the
  sprites and the tilemap (layer priority register $15). It is transparent
  over the map (register $14) and opaque under the panels.
- **The tilemap holds only the map,** and scrolls by its offset registers.
  A camera move of a whole tile changes the offset and draws only the new
  column or row.
- **nge:** a Layer 2 module (clear and fill by DMA, draw a glyph, scroll a
  rectangle), and a Layer 2 text stream like `nge_tm_stream`.
- **The game:** `screen.h` keeps its interface. Only `screen.c` and the
  item screens' drawing change. The item screens no longer cover the map,
  so `game_play_redraw()` shrinks to the help bar.

## 7. Steps

Each step builds, passes `make test`, keeps the game as it plays now (R16),
and gets its own CSpect checks.

1. **Split the frame.** `game_frame_end()`; `play_step()` holds the world
   steps; the title and game-over states stop running them; the title
   clears the map window (R13). The state machine stays for now.
2. **One waiting function and the overlay pool.** `game_wait_key()`, the
   overlay sprites, and targeting moved onto both.
3. **Choosers.** The item screens and direction prompts become choosers,
   and the commands act on their answers. Seven states go.
4. **Structured flow.** `title_run()`, `play_run()`, `over_run()`;
   `game_over()` as a flag; the state machine goes. The name prompt draws
   through the game's frames.

Later, each with its own spec:

5. **Layer 2 UI prototype:** Layer 2 above the tilemap and sprites, text
   drawing speed, message scrolling by DMA, and pixel-scrolling the tilemap
   with sprites through a pixel camera. Decides D9 against its fallback.
6. **The UI on Layer 2, and pixel scrolling.** Specified and built in
   [ui_layer2.md](ui_layer2.md): the UI on Layer 2, and a pixel camera
   (the game still moves it a tile at a time until step 7).
7. **Animations.** Specified and built in [animations.md](animations.md) (2026-10-21):
   slides, lunges and one-shots in the render system, the world waiting
   for them, and the camera following the player's slide.
8. **Demo mode.**

## 8. Tests

PC tests:

- **Step 1:** the title and game-over states post no events and release
  nothing (a dying entity left by a test stays until a play frame).
- **Step 2:** `game_wait_key()` returns a waiting event at once, runs frames
  until one arrives, and returns `NGE_KEY_NONE` after its time-out.
- **Step 3:** `ui_item_key()` gives the item for a listed letter and
  `NGE_ENTITY_INVALID` for any other key. `ui_choose_item()` driven by
  scripted keys returns the right item, and says so when the list is empty.
  `player_direction_key()` gives each direction, and 0 for other keys. A
  command test: w then a letter equips that item.
- **Step 4:** `play_run()` with fake frames: a game over at a given frame
  ends the loop after that frame's events, and nothing acts after it.

CSpect checks, each step:

- every command, screen and prompt works as before (R16): move, get,
  inventory, drop, equip, take off, use, close, fire, look, stairs, quit;
- the title shows no level after a game over (from step 1);
- sprites keep animating on the item screens and during prompts (from
  step 2);
- a death shows its message, then "Game over" (step 4).

## 9. As built: differences from the proposal

- **No `play_run()` mode yet.** It has one kind of game until demo mode
  needs a second (6.11).
- **The name prompt changed no nge code.** The proposal had nge's blocking
  line reader call a frame function the game sets. The line editor's
  non-blocking form, in a loop of the game's frames, does the same with
  no pointer and no change to nge (6.2).
- **An open frame is ended before the next starts** (6.2): the proposal
  didn't say how a chooser inside a play frame keeps the sprites built
  once a frame and through the current camera.
- **No `test_game`.** The play loop waits for real frames
  (`nge_frame_wait_vblank()` polls the raster line), which the PC tests
  can't provide without faking `hw/nge_frame` for one test. Its order and
  game over are covered by the CSpect checks of section 8, and the
  choosers' and commands' logic by `test_ui` and `test_target`.
- **The step 1 PC test** (no events or releases outside play) isn't a
  separate test: `play_step()` is static and only `play_run()` calls it,
  which a reading of `game.c` shows.

## 10. Step 5: the Layer 2 UI prototype

Done: checked on CSpect and hardware, and D9 decided for Layer 2. Shift+L on the title runs it
(`game/proto/proto_bank.c`, debug builds only); ENTER returns to the title.
It uses a new nge module, `layer2/` (page 90): 320 x 256 Layer 2 as 8 x 8
cells, with fills, text from the tilemap's font, and scrolling up by a row.

What it shows, against the questions of D9:

1. **Layering:** Layer 2 above the sprites and the tilemap (order LSU),
   transparent (`$E3`) over the map area, with an opaque stats panel (blue)
   and message and help panels (grey, black) where the tilemap UI's windows
   are.
2. **Drawing speed:** at start it times a fill of the 4 message rows, one
   glyph, a 40-character line, and a scroll of the 4 message rows up one
   row. The times are shown in the panel and on the CSpect console
   (`layer2: ...`). M adds a message (a scroll and a line) and logs its
   time. All of it is done with the CPU (`memset`, `memmove`, a C glyph
   loop); DMA is the next step if the times are too long.
3. **Pixel scrolling:** the whole tilemap (a test pattern: a wall every 8
   tiles, row numbers at the left) scrolls a pixel a frame while an arrow
   key (or CAPS+5-8, or h j k l) is held, wrapping. Four overlay sprites
   stand on tiles, and are placed through the same scroll in the same
   blanking period, so they should stay on their tiles.

Results (to fill in after the CSpect check, and on hardware later):

| Check | Result |
|---|---|
| Panels opaque, map visible through the transparent area, sprites under the panels | |
| Fill, glyph, line, scroll times (us) | CSpect, 2026-10-20, first version (CPU loops, the 4-bit font read per pixel): fill 4 rows 24,320; glyph 1,742; 40-character line 69,696; scroll 4 rows 26,816; a message (scroll and line) 89,472. Hardware, 2026-10-20, the same version: fill 19,712; glyph 1,606; line 64,256; scroll 22,016 (so a message about 86,000). Hardware times are about 0.81 of CSpect's throughout |
| Second version: column-bitmask font, glyphs in assembly, DMA fills and scrolls (one transfer a pixel column, fully reprogrammed each time) | CSpect, 2026-10-20: glyph 363 (was 1,742; about 3,100 T of drawing and 3,800 of overhead: the banked call, the MMU calls, the timer read); 40-glyph line 14,528; fill CPU 25,408, DMA 23,360; scroll CPU 27,648, DMA 28,480; a message 41,664 (was 89,472). DMA gains nothing yet: each transfer is 32 bytes and its 16-write set-up costs about as much as the CPU copy. Hardware, 2026-10-20: glyph 297; line 11,904; fill CPU 20,608, DMA 18,304; scroll CPU 22,656, DMA 22,720; a message 33,472. About 0.8 of CSpect's, the DMA included, so CSpect times the DMA realistically |
| Third version: fill and scroll in assembly (CPU) or by DMA set up once a call; calls timed in halves | CSpect, 2026-10-20: fill CPU 10,112, DMA 9,728; scroll CPU 14,400, DMA 20,672; glyph 259; line 10,368; a message (DMA scroll) 29,952. A first run timed each call whole and read the DMA scroll as 512: it ran past a frame, so the raster line wrapped. Decided: scrolls by the CPU (the DMA's set-up for each 24-byte column costs more than it saves), fills by DMA |
| Fourth version: CPU scroll only, glyph rows unrolled (about 31 T a pixel), text maps a page once every 4 characters | CSpect, 2026-10-20: fill CPU 10,112, DMA 9,728; scroll 14,080; glyph 192 (was 1,742 in the first version); line 7,680; a message 20,928, so about 17,000 on hardware (0.8 of CSpect): within a frame. The scroll is now two-thirds of a message: when several messages come in one frame, step 6 can scroll once by that many rows and then draw each line. Hardware, 2026-10-20: fill CPU 7,680, DMA 7,936; scroll 11,008; glyph 155; line 6,208; a message 16,576, within a frame. On hardware the DMA fill is slower than the CPU's, so the DMA buys nothing for Layer 2 at this layout: step 6 drops it |
| Conclusion | Layer 2 is fast enough for the UI (a message 16.6 ms on hardware), and layering, pixel scrolling and the text are right on hardware: D9 is decided for Layer 2, not the Copper split (2026-10-20) |
| CPU speed (calibration loop) | CSpect: 18.9 T-states a microsecond at turbo setting 28MHz (register 7 reads `0x33`), about 2/3 of nominal. Hardware: 23.2, which is the 28MHz wait state (about one per memory access: the calibration loop's 26 T-state pass takes 31). Fill and scroll come to about 45 T a byte, the CPU's copying speed with the wait state; a glyph to about 580 T a pixel, which is the code. So the next steps are a column-bitmask font for glyphs (about 75% of a message's time), and DMA for fill and scroll |
| Tilemap scrolls smoothly; sprites stay on their tiles | Hardware: yes. A sprite going off the left or top edge vanished whole: the prototype wrapped its position at 320 x 256 like the tilemap, which put it at the far right or bottom, outside the sprite clip window. Fixed: a position that would run off the right or bottom is placed off the left or top instead, wrapped to 9 bits (512 - k), so the visible part shows (checked on hardware). Step 6 needs the same in the render system once the camera moves by pixels: today a sprite can't be partly off the view's left or top, as the camera moves by whole tiles from the screen's edge |
| Panels and layering | Hardware: correct |
| The title and a new game work after ENTER | |
