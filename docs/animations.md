# Action animations: requirements and design

Status: implemented (2026-10-21). CSpect checks 1 to 6 in section 7
passed (2026-10-21); check 7 (frame time on hardware) is not measured yet:
it needs a timing readout, as it can't be judged by eye. D1 to D8 were agreed on 2026-10-21, with the notes in D5, D6 and
D8. Section 8 lists where the build differs from the design.

Step 7 of [game_loop.md](game_loop.md), from its R18, D8 and section 6.10.
An entity that acts plays the action's animation: a slide from its old
tile to the new, a lunge at what it attacks. The world waits for the
animations in view to finish before it acts again, but the actions
themselves stay instant. The camera follows the player's slide a pixel at
a time, through the pixel camera built in step 6
([ui_layer2.md](ui_layer2.md)).

## 1. Goal

- Movement and attacks read as movement, not as a sprite jumping a tile.
- The rules, the AI and the tests never wait for the screen: an action
  changes the world at once, and the animation shows what already happened.
- The engine (nge) does the animating that every tile game needs. The game
  says which animation goes with which event, and nothing more.

## 2. Terms

| Term | Meaning |
|---|---|
| Rest animation | An entity's looping animation, shown when it is doing nothing: `count` patterns from `first`, each for `rate` frames. With one pattern it is a still sprite. Today's `nge_render_set_anim()` and `nge_render_set_pattern()` set it |
| One-shot | A sequence of patterns played once over the rest animation. When it ends, the rest animation shows again |
| Motion | A temporary draw offset that moves back to 0 at a fixed speed: a slide (starting from the old tile) or a lunge (out towards a target, then back) |
| Draw offset | The offset set with `nge_render_set_offset()`, for example (0, -8) to stand on the tile. A motion is added to it and doesn't change it |
| Busy | An entity in view whose motion or one-shot is still playing |
| Holding | Any entity busy: the world doesn't act and the player's command waits |
| Sequences | A kind's patterns for each action: rest, move, attack (in its base record) |

## 3. Requirements

Numbered for this step. game_loop.md's R18 is the source of R1 to R4.

### What is shown

- **R1** An entity that moves one tile slides from its old tile to its new
  one, at a fixed speed, and plays its kind's move sequence if it has one.
- **R2** An entity that attacks in melee (hit or miss) lunges a few pixels
  towards its target and back, and plays its kind's attack sequence if it
  has one.
- **R3** An entity doing nothing shows its rest animation, as now. A
  one-shot that ends returns to the rest animation by itself: a still
  pattern if the rest animation has one pattern. **Changed:** game_loop.md
  6.10 has two cases ("the kind's idle animation, or still"); they are one
  case in the engine (D1).
- **R4** Actions stay instant (game_loop.md R18): the location, the damage
  and the events happen when the action does. No action and no event
  handler runs frames or waits.

### Waiting for animations

- **R5** The world doesn't act while an entity in view is busy: the turn
  engine's `world_can_act()` is false (game_loop.md D8).
- **R6** The player's command waits while an entity in view is busy. Keys
  stay queued, and are taken when the animations end.
- **R7** Only entities in view animate a motion or one-shot. One that is
  out of view when its animation would start shows at its new place at
  once, and holds nothing up. Rest animations play as now.
- **R8** Moves at the same tick slide together: everything that acts in
  one advance of the clock starts its animation before the world is
  checked again.
- **R9** The frames go on while animations play: the screen stays live,
  and the play loop's frame does everything but the turn engine
  (game_loop.md R1).

### The camera

- **R10** The camera follows the player's drawn position a pixel at a
  time while they slide, so the map scrolls smoothly and the player stays
  centred (clamped at the map's edges, as now). **New in this step:**
  game_loop.md R19 is met here.
- **R11** Nothing else moves the camera differently from now: targeting
  and the V key move it as before, and a new level centres it on the
  player.

### Constraints

- **R12** No function pointers to banked code, and no call back from nge
  into the game for an animation's end (game_loop.md R15).
- **R13** A frame's work stays within a frame on hardware with the
  number of entities a level has in view.

### Later

- **R14 (later)** Transient visuals on overlay sprites (game_loop.md D6):
  projectiles for fire, hit flashes, deaths. They need their own patterns
  and a pool manager; this step leaves room by making "holding" a
  question any visual can answer (D5).
- **R15 (later)** Sequences for the kinds: there is art only for the rest
  animations today (the player's and the goblin's 2-pattern figure, the
  rat's still). The table has room for move and attack sequences, and
  they are "none" until there is art.

## 4. Decisions

- **D1 The engine returns to the rest animation; the game never does.**
  This answers the question of who returns an entity to its idle
  animation, and what the base case is. The render system already keeps
  each sprite's animation. It keeps the rest animation separately from
  the one-shot playing over it, and when the one-shot's last pattern has
  been shown, it shows the rest animation again. "Still" is not a
  separate case: it is a rest animation of one pattern. The game decides
  what an entity's rest animation is (from its kind, or later a state
  such as asleep: it calls `nge_render_set_anim()` again) and which
  one-shot an event plays. This is the base case in every engine with
  sprite animation (a looping base state, and one-shot clips that fall
  back to it), it needs nothing from the game at the end (R12), and the
  game can't forget to do it. **Rejected:** the game restoring the rest
  animation when an animation ends. That needs an end event or a
  callback from nge (a function pointer, or polling every entity each
  frame), and every one-shot the game plays would need the matching
  restore.
- **D2 Motions are in the render system, as a second offset.** A motion
  is an offset added to the draw offset, which moves towards 0 by a
  speed in pixels a frame. A slide starts at the old tile's offset (-8 *
  dx, -8 * dy) and moves to 0. A lunge moves out to its offset, then back
  to 0. Both are generic, so they are nge's: `nge_render_slide()` and
  `nge_render_lunge()`. **Rejected:** moving the draw offset itself. It
  would lose an offset the game set (standing on the tile), and the game
  would have to put it back. **Rejected:** a general tween with a start,
  an end and a frame count. It needs a division a frame per entity, or a
  table, for two shapes the game uses.
- **D3 Speed, not frames.** A motion moves a whole number of pixels a
  frame, so no division is needed and diagonals finish with the straight
  moves. Proposed: slides at 2 pixels a frame (4 frames a tile), lunges 4
  pixels out at 2 a frame (4 frames there and back). 4 frames a tile is
  the key repeat rate (`NGE_KEY_REPEAT_RATE`), so a held movement key
  keeps pace with the slides and doesn't queue up moves. **To decide:**
  the speeds, after the CSpect check.
- **D4 The animations start from events, in a game module.** A new
  `game/anim/` module's `anim_on_event()`, called by `game_on_event()`,
  starts the slide on `EV_MOVED` and the lunge on `EV_HIT` and
  `EV_MISSED` (game_loop.md 6.10). `EV_MOVED` gains dx and dy in its
  payloads. The actions don't change otherwise (R4). **Rejected:** the
  actions starting the animations. The rules would depend on the render
  system, and the PC tests of the actions would need it.
- **D5 Holding is a question nge answers.** `nge_render_busy()` is true
  while an entity in view has a motion or one-shot playing. The game's
  `anim_holding()` asks it (and later its overlay visuals, R14), and the
  turn engine and the player's command ask `anim_holding()`. It is worked
  out from the entities themselves, so an entity released mid-animation
  can't leave the world waiting. Looping rest animations never hold.
  **Agreed, with when it is asked:** only at the two points that decide
  whether something may act, never as part of a frame's drawing:
  - the player's command: only when a key is waiting, so a player who is
    thinking costs nothing;
  - the world: in `world_can_act()`, after the cheaper conditions (so not
    on the player's turn, during a change of level or after a game over).
    Before each advance of the clock, so while an animation holds it, the
    world asks once a frame, and stops at the first "busy".

  Each ask is one banked call that returns a byte the last update worked
  out.
- **D6 The camera follows an entity's drawn position, in the render
  system.** `nge_render_follow(id)` names an entity. Each update, after
  the motions advance and before the sprites are placed, the camera is
  centred on that entity's drawn position if it has changed since the
  last update: a slide, or a jump (a new level). When the entity doesn't
  move (targeting, the V key, the item screens), the camera is left where
  the game put it (R11). `player_on_event()` stops centring the camera
  on `EV_MOVED`. **Rejected:** the game centring the camera each frame.
  `nge_render_update()` advances the motions and places the sprites
  through the camera in one call, so the game's camera would be a frame
  behind the slide, and the player would jitter by a slide's step. Splitting
  the update into two banked calls would fix that, at the cost of a call a
  frame and an order every loop that runs frames must keep.
  **Agreed, and it works with targeting:** targeting runs on the
  player's turn, which starts only once nothing is busy, so the player
  isn't sliding while the cursor moves the camera. The player's drawn
  position doesn't change during targeting, so the follow leaves the
  camera to the cursor, and targeting puts it back on the player at the
  end, as now. The V key works the same way.
- **D7 "In view" is judged when an animation starts.** A motion is started
  only if the entity's old or new tile is in the view, through the camera
  as it is then; otherwise it is dropped and the entity shows at its new
  place at once (R7). A one-shot likewise. An entity that leaves the
  view while it plays (the camera followed the player away) finishes its
  animation at once. **Rejected:** judging it every frame against the
  sprite's drawn rectangle only. An entity stepping into the view from
  outside would appear without a slide.
- **D8 Kinds' sequences are copied into a table by kind at start-up.** The
  base records (pages 107, 109, 123) are read only by the generators'
  pages. `gen_render_t` gains a move and an attack sequence (first,
  count, rate; count 0 for none), and the generator stores the kind's
  sequences for the entity where `anim_on_event()` can read them: the
  kind index is already in slot 0 (`kind_index`), so a table by class
  and kind in the anim module's page, filled from the base records once
  at start-up, costs 6 bytes a kind and nothing per entity.
  **Rejected:** a per-entity component. It costs 6 bytes for each of 100
  entities for data that is the same for every entity of a kind.
  **Agreed for now.** How it scales: 6 bytes a creature kind, so 100
  kinds is 600 bytes of page 127's 8K. Items and features get a table
  only if they come to animate. If more sequences a kind are added (hurt,
  die), each costs 3 bytes a kind. **If it outgrows a page:** drop the
  copy and look a kind's sequence up when an event needs it, by a banked
  call into the creature generator's page (106, which reads 107). That
  costs one more banked call for each animation started, and no memory.

## 5. Design

### 5.1 nge: the render system (`ecs/systems/`, pages 52/53)

**Data,** in page 53 (one table per field, as now):

| Field | Meaning |
|---|---|
| `rest_first`, `rest_count`, `rest_rate` | the rest animation (today's `first`, `anim_count`, `anim_rate`) |
| `first`, `anim_count`, `anim_rate`, `anim_tick`, `anim_frame` | what is playing: the rest animation, or a one-shot |
| `oneshot` | 1 while a one-shot plays |
| `mox`, `moy` | the motion's offset now, added to the draw offset |
| `mtx`, `mty` | a lunge's turning point (0, 0 for a slide or on the way back) |
| `mspeed` | pixels a frame, 0 when there is no motion |

About 7 bytes more an entity: 700 bytes, in page 53's free space (it has
about 6K free).

**Functions** (banked, page 52):

```c
/* The rest animation: as now, but a one-shot playing carries on, and returns to this */
void nge_render_set_pattern(nge_entity_id_t id, uint8_t pattern) NGE_BANKED;
void nge_render_set_anim(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED;

/* Play patterns first to first+count-1 once, each for rate frames, then the rest animation (D1) */
void nge_render_play(nge_entity_id_t id, uint8_t first, uint8_t count, uint8_t rate) NGE_BANKED;

/* Slide from (ox, oy) pixels away to the location, speed pixels a frame (D2, D3) */
void nge_render_slide(nge_entity_id_t id, int8_t ox, int8_t oy, uint8_t speed) NGE_BANKED;

/* Lunge out to (ox, oy) pixels away and back, speed pixels a frame */
void nge_render_lunge(nge_entity_id_t id, int8_t ox, int8_t oy, uint8_t speed) NGE_BANKED;

/* An entity in view has a motion or one-shot playing (D5) */
uint8_t nge_render_busy(void) NGE_BANKED;

/* Centre the camera on this entity's drawn position when it changes; NGE_ENTITY_INVALID for none (D6) */
void nge_render_follow(nge_entity_id_t id) NGE_BANKED;
```

- `nge_render_play()`, `_slide()` and `_lunge()` do nothing but show the
  end state if the entity isn't in view (D7): `_play()` leaves the rest
  animation, `_slide()` leaves no offset, `_lunge()` nothing.
- A new motion replaces one playing, from where it is now; likewise a
  one-shot.
- **`nge_render_update()`**, per entity it draws:
  1. advance the animation; at the end of a one-shot, back to the rest
     animation from its first pattern;
  2. advance the motion by its speed: towards the turning point, then
     towards 0; at 0, no motion;
  3. drawn position = location + draw offset + motion.

  Then, if a followed entity's drawn position changed, it centres the
  camera on it (`nge_map_centre_camera_px()`), and only then places the
  sprites through the camera. An entity not in view has its motion and
  one-shot finished at once (D7). The busy count is the number of drawn
  entities still busy after this.
- **`nge_render_busy()`** returns the count from the last update, or
  true if a motion or one-shot started since (the starting functions set
  it). So the advance that starts a slide stops the next advance at once
  (R8).
- `nge_render_init()` clears the follow.

### 5.2 nge: the map (`map/`)

```c
/* Centre the view on a map pixel, clamped to the map (fine offset as needed) */
void nge_map_centre_camera_px(uint16_t x, uint16_t y);
```

Like `nge_map_centre_camera()`, through `nge_map_set_camera_px()`. In
common memory with the other camera functions, so the render system
(banked) can call it.

### 5.3 The game: the anim module (`game/anim/`, new, pages 126/127)

The code in page 126, the kinds' sequences in page 127: the pair the
prototype freed in step 6. `anim_holding()` is in common memory.

```c
/* Start the animations for an event: slides, lunges, one-shots (D4) */
void anim_on_event(const nge_event_t *e) NGE_BANKED;

/* An animation the world waits for is playing (D5) */
uint8_t anim_holding(void);

/* Fill the kinds' sequences from the base records: once, at start-up (D8) */
void anim_init(void) NGE_BANKED;
```

| Event | Animates | How |
|---|---|---|
| `EV_MOVED` (p1 dx, p2 dy) | the target | `nge_render_slide(target, -8 * dx, -8 * dy, ANIM_SLIDE_SPEED)`, and its move sequence with `nge_render_play()` |
| `EV_HIT`, `EV_MISSED` | the source (the attacker) | `nge_render_lunge(source, 4 * sx, 4 * sy, ANIM_LUNGE_SPEED)`, where sx and sy are the signs of the target's position less the attacker's; and its attack sequence |

- `game_on_event()` calls `anim_on_event()` first, so the other handlers
  see the same frame.
- `anim_holding()` is `nge_render_busy()` until overlay visuals come
  (R14).
- A moved entity with no sprite or not in `LEVEL_AREA` is skipped.

### 5.4 The game: the turn engine and the player (`game.c`, `player/`)

```c
static uint8_t world_can_act(void)
{
    return !player_turn && !level_pending() && !ended && !anim_holding();
}
```

- `play_turn()` takes a key only if one is waiting and `!anim_holding()`
  (R6), in that order (D5). Seeing that a key is waiting without taking
  it needs `nge_key_waiting()` (new, `input/`, common memory: the number
  of key events queued), as `nge_key_get()` takes the key off the queue.
- `pass_time()` checks `world_can_act()` before each advance, after the
  last advance's events, which started its animations (R8).
- `play_start()` calls `nge_render_follow(actor_player)` after creating
  the player. `player_on_event()` no longer centres the camera on
  `EV_MOVED` (D6). A new level still centres it (`level_gen.c`).
- `act_move()` posts `EV_MOVED` with dx and dy (as `uint8_t`) in p1 and
  p2; `events.h` says so.

### 5.5 The game: sequences (`gen/`)

`gen_render_t` gains `move` and `attack`, each `{ first, count, rate }`
(count 0: none). All are 0 for now (R15). `anim_init()` copies each
creature kind's two sequences into a table in the anim module's page,
through a banked helper in the creature generator's page that reads
page 107 (D8). Items and features have none.

### 5.6 Call depth and cost

- The deepest chain doesn't change: `anim_on_event()` is called from
  `game_on_event()` (common) and calls the render system: two banked
  levels, inside `nge_event_process()`, which is called from common
  code.
- `nge_render_update()` gains a few comparisons and two additions an
  entity in view. With 20 entities in view, well under 1 ms.
- `nge_render_busy()`: one banked call an advance, at most
  `CLOCK_ADVANCES_MAX` a frame.

## 6. Scope

In:

- R1 to R13, as in section 5;
- PC tests (section 7);
- docs: `src/nge/README.md` (the render system's motions, one-shots and
  follow), `src/game/README.md` (the anim module), `docs/memory_map.md`
  (the anim module's page, page 53's size), game_loop.md (step 7, R18
  and R19 done, 6.10 pointing here).

Out, for later:

- overlay visuals: projectiles, hit flashes, deaths (R14);
- art for move and attack sequences (R15);
- monsters' moves: there is no monster AI yet (`TIMER_MONSTER` waits), so
  only the player slides and lunges in play. The engine's handling of
  several entities is checked by the PC tests.

## 7. Tests

PC tests:

- **`test_render`:**
  - a one-shot plays its patterns once at its rate, then the rest
    animation from its first pattern; with a still rest, the still
    pattern;
  - `nge_render_set_anim()` during a one-shot changes what follows it,
    not the one-shot;
  - a slide moves the drawn position by the speed each update and stops
    at the location; diagonals finish with straight moves; the draw
    offset is kept;
  - a lunge goes out to its offset and back to 0;
  - `nge_render_busy()`: true from the start of a motion or one-shot
    until the update that ends it; false for rest animations; false for
    an entity released mid-motion; a start out of view does nothing and
    isn't busy;
  - follow: the camera centres on the followed entity's drawn position
    as it slides, and isn't moved when the entity doesn't move.
- **`test_map`:** `nge_map_centre_camera_px()`, clamped at each edge.
- **`test_anim`** (new): `EV_MOVED` starts a slide from the old tile;
  `EV_HIT` and `EV_MISSED` lunge the attacker towards the target, in
  each of the 8 directions; an entity without a sprite is skipped.
- **`test_act`:** `EV_MOVED`'s dx and dy.
- **`test_input`:** `nge_key_waiting()` counts queued events and doesn't
  take them.

CSpect and hardware checks:

1. The player slides a tile at a time, in all 8 directions, and the map
   scrolls smoothly with them; at the map's edges the camera stops and the
   player slides on to the edge.
2. Holding a movement key walks steadily, and letting go stops within a
   tile or two (D3: no queued moves running on).
3. Bumping a monster lunges at it and back; a door bumped opens as now.
4. Targeting and the V key move the camera as before; ending them puts it
   back on the player. The item screens and the inventory work as before.
5. The stairs: the new level shows centred on the player, with no slide.
6. The idle animations play throughout, also under screens and prompts.
7. Frame time with the most entities in view (hardware): within a frame.

## 8. As built: differences from the design

- **The sequences are in the creature record, not `gen_render_t`** (5.5):
  `gen_creature_base_t` has `seqs[GEN_SEQ_COUNT]`, read by
  `gen_creature_seqs()` (page 106). Items share `gen_render_t` and have
  no sequences, so they don't carry the 6 empty bytes.
- **A one-shot shows each pattern for its full rate,** from the update
  after `nge_render_play()`. The rest animation's timing is as before (its
  first pattern shows one frame short of its rate the first time round).
- **An entity sliding into the view is busy while it is still out of
  it:** "out of view" for ending an animation (D7) means its place (the
  location, without the motion) is out of view, not just its drawn sprite,
  so a slide from outside the edge plays in.
- **The followed position is the location plus the motion,** without the
  draw offset, so following centres the camera exactly as
  `nge_map_centre_camera()` does on the entity's tile.
- **`nge_map_centre_camera_px()`** puts the pixel where
  `nge_map_centre_camera()` puts a tile's top-left, so the two agree on a
  tile.
- **The render system's page data grew by 9 bytes an entity,** not 7: the
  rest animation is three tables, the one-shot flag one, and the motion
  five. Page 53 now holds about 2.7K. Common memory lost 126 bytes
  (`anim_holding()`, `nge_key_waiting()`, `nge_map_centre_camera_px()`
  and the calls): 697 bytes are free in debug builds.
