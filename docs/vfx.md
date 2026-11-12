# Visual effects (VFX): requirements and design

Status: implemented (2026-11-08). CSpect checks 1 to 9 in section 7 passed
(2026-11-08); hardware not tried yet, so check 10 (frame time on hardware) is
not measured. D1 to D11 agreed on 2026-11-08, with D5 (magic at 4 pixels a
frame, R11 changed to allow it), D6 (the colours as proposed), 5.4 (an orb for
magic), and a hit flash added to scope (R22, D11); the art agreed as drawn the
same day. Section 8 lists where the build differs from the design.

To-do item V1 ([todo.md](todo.md)). This is R14 of
[animations.md](animations.md) ("transient visuals on overlay sprites:
projectiles for fire, hit flashes, deaths") and D6 of
[game_loop.md](game_loop.md) (overlay sprites for them). It builds on the
action animations: the actions stay instant, the events they post start
what is shown, and the world waits while it plays.

What is shown:

- a shot (arrow, bolt, thrown item) flying from the shooter to the target;
- a magic bolt flying from the caster to the target, coloured by what it
  does;
- a glow on a creature that a spell, wand or potion takes effect on,
  coloured by the effect (healing, a curse, haste...);
- a splash on a creature that takes damage, coloured by the damage type,
  and the creature flashing white;
- a dying creature that stays until the blow lands, flashes, and goes.

## 1. Goal

- Ranged attacks and spells read as something travelling from one place
  to another, and their results read as happening to the target.
- Colour says what kind of thing happened: fire is orange, cold is ice
  blue, healing is green, and so on, with the palette banks the sprites
  already use.
- The rules, the AI and the tests never wait for the screen (animations.md
  R4). VFX only show what already happened.
- The engine (nge) does what every tile game needs: sprites that fly from
  one point to another or play once at a point, and a way to ask whether
  any are still playing. The game says which look goes with which event.

## 2. Terms

| Term | Meaning |
|---|---|
| VFX | A visual effect: a short-lived sprite that isn't an entity. Not an *effect* (effects.md: what a potion or poison does) |
| Slot | One of the engine's VFX records, with its own hardware sprite |
| Bolt | A VFX that moves in a straight line from one map pixel to another at a fixed speed, playing its patterns as it goes, then disappears |
| Burst | A VFX that plays its patterns once at a map pixel, then disappears: a glow, a splash, a puff |
| Delay | Frames a VFX waits, hidden, before it starts |
| Arrival | Frames until a bolt in flight reaches its end |
| Impact | A burst on the tile a bolt or a blow lands on |
| Stand-in | A burst showing a dying creature's own pattern, so its sprite doesn't vanish before the blow that killed it lands |
| Look | A row of the game's table: a pattern run, its rate and a palette bank (and for a bolt, a speed) |

## 3. Requirements

### What is shown

- **R1** A ranged attack (`EV_FIRED`), hit or miss, shows a bolt from the
  shooter's tile to the target's tile. Thrown items, arrows and bolts can
  have different looks.
- **R2** A spell cast at another creature (`EV_CAST` with target ≠
  source), or a wand or staff aimed at one (`EV_AIMED`), shows a bolt from
  the caster to the target, in the colour of the effect it carries.
  Direct spells (`SPELL_DIRECT`, such as Hold) fly too: they skip the
  creatures on the way in the rules, and the bolt just passes over them.
- **R3** An effect taking hold on a creature (`EV_EFFECT_STARTED`) shows a
  glow burst on it, in the effect's colour. Effects can have no glow:
  innate ones (regeneration, mana regeneration), and damaging ones whose
  splash already shows (R4).
- **R4** Damage (`EV_DAMAGED` with p1 > 0) shows a splash burst on the
  creature hurt, in the damage type's colour. This covers melee, shots,
  bolts, traps and poison's ticks. **Changed:** the request had the splash
  for melee only; tying it to the damage event gives every source of
  damage the same feedback for free.
- **R5** A miss shows the bolt (R1) and no impact. 0 damage (immune) shows
  no splash: the message says why.
- **R6** A creature killed (`EV_DIED`) doesn't vanish before the blow lands:
  a stand-in shows it at its tile until the impact, then it flashes white
  (the flash bank) and a short puff plays. **New:** without this, a
  creature shot dead disappears while the arrow is still flying to it,
  because the kill destroys it at once.
- **R7** A spell on the caster (`SPELL_SELF`), a potion or a staff on its
  user shows only the glow (R3), with no bolt.

### Timing

- **R8** An impact starts when the bolt to its tile arrives: a shot's
  splash, a bolt spell's glow or splash, and a stand-in's flash all wait
  for it.
- **R9** A melee splash starts at the peak of the attacker's lunge, not
  when the lunge starts.
- **R10** VFX hold the world like the action animations: the world doesn't
  act and the player's command waits (keys queued) while any VFX is
  playing or waiting to start (animations.md R5, R6).
- **R11** A bolt flies at a fixed speed in pixels a frame, so a longer
  shot takes longer. Over the longest range (8 tiles, `ACT_AIM_RANGE` and
  the bolt spells' range), a shot takes at most about a third of a second
  and a magic bolt about 0.6 s. **Changed** (2026-11-08, D5): a third of a
  second for both was proposed, and magic is allowed to be slower so it
  reads as heavier.

### Where

- **R12** Only VFX in view play: a VFX whose start and end are both
  outside the view is dropped at once and holds nothing up. A bolt that
  enters or leaves the view plays, clipped as the creature sprites are.
- **R13** VFX are drawn over the creatures and items, and under the UI.
- **R14** VFX are cleared on a change of level and on a new game.

### Constraints

- **R15** No function pointers to banked code, and no call back from nge
  into the game (animations.md R12, game_loop.md R15).
- **R16** A frame's work stays within a frame on hardware.
- **R17** Common memory grows by as little as possible: only the calls
  from the frame functions (it has about 4.7K free in debug builds).
- **R18** The art fits the Effects share of the pattern budget:
  14 patterns (sprite_style.md 6.8).

- **R22** A creature that takes damage (p1 > 0) and survives flashes
  white (the flash bank, `BANK_FLASH`) for a moment, when the splash
  starts (R8, R9). This includes the player. **New** (2026-11-08, from
  review): sprite_style.md D8 planned hits as the flash bank, and it
  wasn't built.

### Later

- **R19 (later)** True particles: several small sprites from one point
  with their own velocities (sparks, a fire burst). The slot is general
  enough to add a velocity burst kind.
- **R20 (later)** Area effects (a fireball's radius), a cast gesture for
  the caster, a breath cone, a beam that stays drawn the whole line.
  I3 built balls ([items_aimed.md](items_aimed.md) 4.8): `EV_AREA` flies
  a bolt to the centre and bursts there, and the creatures' impacts wait
  for it. On a change of level and a new game, `anim_vfx_clear()` clears
  the VFX and forgets the centre (R14).
- **R21 (later)** Floating damage numbers.

## 4. Decisions

- **D1 The name: VFX, in a module of that name.** "Effects" is taken
  (effects.md), and "particles" usually means many tiny dots moved by
  physics, which is one kind of VFX (R19) but not what most of these are.
  The engine module is `nge/vfx/`, and the game's mapping lives in the
  anim module. **Rejected:** "overlay visuals" (animations.md's working
  name): "overlay" already means the overlay screens (ui_screens.md).

- **D2 The engine owns a pool of slots, separate from the render system
  and the game's overlay sprites.** `nge/vfx/` has `NGE_VFX_SLOTS` (12)
  slots for hardware sprites `NGE_VFX_SPRITE` onwards, after the game's
  overlay sprites (64 to 67), so 68 to 79. With the default sprite
  priority, higher numbers draw on top, so VFX are over the creatures
  (R13). The slots, their sprite records and the code live in a new nge
  page pair, mapped by the banked calls, so common memory only gains the
  calls (R17). **Rejected:** VFX as entities in the render system: an
  arrow in flight would take an entity slot and a render sprite, and a
  stand-in must outlive its entity (game_loop.md D6). **Rejected:** the
  game's overlay pool (`screen_overlay_*`): its 4 records are in common
  memory, it has no movement or animation, and growing it costs common
  memory.

- **D3 Two kinds: bolt and burst, each with a delay.** A burst plays a
  pattern run once at a point. A bolt plays its pattern run looping while
  it moves from one point to another. Both wait their delay first,
  hidden. The game builds everything in section 1 out of these:
  a stand-in is three bursts (D9). **Rejected:** a general timeline or
  script per VFX: more code and data for shapes the game composes from
  two primitives and a delay.

- **D4 An impact waits for a bolt to its tile, by an engine rule.** All the
  events of one action are handled in one go: `EV_FIRED`, `EV_HIT`,
  `EV_DAMAGED` and `EV_DIED` arrive in the same frame. So a burst asked
  for while a bolt is flying to the same point starts at that bolt's
  arrival (or later, if its own delay is longer). The game doesn't track
  bolts. This is safe because the world holds while a bolt flies (R10),
  so the only bursts asked for during a flight come from the same action,
  or from actions at the same tick, which should land together anyway.
  **Rejected:** the game remembering the last bolt's target and arrival:
  state that must be cleared at the right time, in every handler.
  **Rejected:** `EV_FIRED` carrying the impact: the action doesn't know
  the outcome when it posts it, and the effect's events come from
  `effect_apply()`.

- **D5 Bolts move by speed, with a Bresenham step.** The longer axis moves
  `speed` pixels a frame and the shorter one follows by an error term, so
  any angle works (a knight's-move shot) with no division a frame. Speeds
  are 4 or 8 pixels a frame, so a bolt's arrival is a shift of its length
  (D4 needs it when a burst is asked for). Proposed: shots at 8 (2 frames a
  tile: 16 frames for 8 tiles), magic at 4 (4 frames a tile, it reads as
  slower and heavier: 32 frames for 8 tiles, about 0.6 s). **Agreed**
  (2026-11-08): R11 changed to allow magic's 0.6 s. The speeds can still
  be tuned after the CSpect check.

- **D6 Colour comes from the palette banks, not new patterns.** The
  sprites are 4-bit, and the 16 banks already have the colourways that
  are wanted (sprite_style.md 5.2): one glow pattern run shows green in
  bank 6, orange in 4 and ice in 9. Proposed colours:

  | What | Bank |
  |---|---|
  | Blunt, slash, pierce damage | 3 red |
  | Fire | 4 orange |
  | Cold | 9 ice |
  | Poison, acid | 7 lime |
  | Magic damage, magic bolt | 10 purple |
  | Healing, cure, renewal | 6 green |
  | Buffs: haste, might, stone skin, resists, raises | 5 gold |
  | Curses: slow, weakness, hold, blind, sleep | 15 dark |
  | Mana | 8 blue |

  **Agreed** (2026-11-08).

- **D7 Directions from the hardware's rotate and mirror flags.** A bolt's
  pattern is drawn pointing right, and its diagonal pointing up-right. The
  rotate and mirror flags give the other 6 directions. A bolt at another
  angle uses the nearest of the 8. The magic orb (5.4) is round and needs
  no direction: it is drawn with no rotate or mirror. **Rejected:** a
  pattern per direction: four times the patterns, for an 8-tile flight.

- **D8 The looks are a table in the game, keyed by what is known in the
  event.** In the anim module's data page:
  - by damage type (`DMG_*`): the splash;
  - by effect (`EF_*`): the bolt that carries it and the glow it shows,
    either of which can be none;
  - by ammo group (arrows, bolts, thrown): the shot's bolt.

  `EV_CAST` gives the spell, so the effect comes from the spell
  (`spell_get()`, or a copy of each spell's effect at start-up, as
  `anim_init()` copies the kinds' sequences). `EV_AIMED` gives the item,
  whose effect comes from its aim hook (`gen_hook_of()`). Instant
  damaging effects (`EF_BOLT_*`) have no glow: their `EV_DAMAGED` splash
  shows (R3, R4), so the target doesn't show both. **Rejected:** a look
  field in the spell and effect definitions: those tables are the rules'
  and are in other modules' pages, and the anim module would map them in
  for every event.

- **D9 A stand-in from three bursts.** On `EV_DIED`, the dying creature is
  still readable: it is released in the clean-up after the events
  (`level_release_dying()`), and the render system's record for it stays
  until then. The anim module asks the render system for its pattern and
  flags (`nge_render_get()`, new) and starts:
  1. its pattern in its own bank, shown at once, for as long as the
     impact's delay (D4, R9);
  2. its pattern in the flash bank (`BANK_FLASH`), 4 frames, after that
     delay;
  3. the death puff, after the flash.

  The bursts are asked for in the order of the events, so the splash (from
  `EV_DAMAGED`, which comes first) and the stand-in start together.
  Burst 1 is the one case where a VFX shows during its "delay", so a burst
  shown from the start is a burst with no delay whose rate is that
  delay. **Rejected:** keeping the entity alive until the animation ends:
  the actions stay instant (animations.md R4), and every rule that looks
  at the map would see a dead creature. **Rejected:** a dying entity's
  sprite fading in the render system: it is released at the end of the
  frame.

- **D10 A melee splash is delayed to the lunge's peak by the game.** The
  game starts an `EV_DAMAGED` splash with a delay of
  `ANIM_IMPACT_DELAY` (`ANIM_LUNGE_PIXELS / ANIM_LUNGE_SPEED` = 4
  frames). With a bolt in flight to the tile, D4 makes it later. Damage
  from a trap or a poison tick waits the same 4 frames (80 ms), which
  isn't seen. **Rejected:** working out whether the damage came from a
  melee attack: the event doesn't say, and the difference can't be seen.

- **D11 The hit flash is a VFX burst over the creature, as the
  stand-in's flash is.** On `EV_DAMAGED` with p1 > 0, if the target is
  still alive, the anim module reads its pattern and flags
  (`nge_render_get()`, D9) and starts a burst of that pattern in the
  flash bank, 4 frames, at the creature's point, with the splash's delay
  (D10). The flash is started before the splash, so it normally has the
  lower slot and the splash draws over it. A creature killed by the blow
  is already dying when its `EV_DAMAGED` is handled (`health_damage()`
  destroys it before the events are processed), so it gets no hit flash,
  only the stand-in's flash (D9), and never two. **Rejected:** a timed
  palette override in the render system (`nge_render_flash()`): it adds a
  field to every entity and new code to page 52, and it can't wait for a
  bolt's arrival (D4) without asking the VFX module for it. **Accepted
  risk:** if the creature's rest animation changes pattern during the 4
  frames, the flash shows the old pattern over the new one, a pixel or
  so out at most (rest animations move little, sprite_style.md D7).

## 5. Design

### 5.1 nge: the VFX module (`nge/vfx/`, a new nge page pair)

**Data,** in the pair's data page:

| Field | Meaning |
|---|---|
| `kind` | none, burst or bolt |
| `x`, `y` | map pixels now (16-bit) |
| `ex`, `ey` | a bolt's end |
| `sx`, `sy`, `err`, `major` | a bolt's step: signs, error term, pixels left on the long axis |
| `speed` | a bolt's pixels a frame (4 or 8) |
| `first`, `count`, `rate`, `tick`, `frame` | the pattern run, as the render system's one-shot |
| `flags` | palette bank, mirror and rotate (`NGE_SPRITE_*`) |
| `delay` | frames to wait, hidden |

About 20 bytes a slot, 240 bytes for 12, and 12 sprite records.

**Functions** (banked):

```c
#define NGE_VFX_SLOTS   12
#define NGE_VFX_SPRITE  (NGE_RENDER_SPRITES + 4)   /* after the game's overlay sprites */
#define NGE_VFX_NONE    0xFF                       /* no slot was free */

void nge_vfx_init(void) NGE_BANKED;                /* every slot free, the sprites hidden */
void nge_vfx_clear(void) NGE_BANKED;               /* end every VFX now (R14) */

/* Play patterns first..first+count-1 once at map pixel (x, y), each for
   rate frames, after delay frames, or after a bolt flying to (x, y)
   arrives if that is later (D4). */
uint8_t nge_vfx_burst(uint16_t x, uint16_t y, uint8_t first, uint8_t count, uint8_t rate,
                      uint8_t flags, uint8_t delay) NGE_BANKED;

/* Fly from (x, y) to (ex, ey) at speed (4 or 8) pixels a frame, looping the
   patterns, then disappear (D5). flags has the direction's rotate and
   mirror (nge_vfx_dir_flags()). */
uint8_t nge_vfx_bolt(uint16_t x, uint16_t y, uint16_t ex, uint16_t ey, uint8_t speed,
                     uint8_t first, uint8_t count, uint8_t rate, uint8_t flags) NGE_BANKED;

/* The rotate and mirror flags for a bolt going (dx, dy), and whether to use
   the diagonal pattern (D7). */
uint8_t nge_vfx_dir_flags(int16_t dx, int16_t dy, uint8_t *diagonal) NGE_BANKED;

uint8_t nge_vfx_busy(void) NGE_BANKED;    /* a VFX is playing or waiting (R10) */
void nge_vfx_update(void) NGE_BANKED;     /* advance and build the sprites: game_frame_end() */
void nge_vfx_upload(void) NGE_BANKED;     /* send them: game_frame_start(), in the blanking period */
```

- **Pool full:** the call returns `NGE_VFX_NONE` and nothing is shown. The
  rules don't care, and nothing waits for it.
- **In view (R12):** judged at the start through the camera as it is then,
  from the start and end points; a VFX out of view is never started. While
  one plays, it is placed through the camera each frame and hidden while
  outside the view, as the render system does.
- **`nge_vfx_update()`,** per slot in use:
  1. a delay counts down, hidden;
  2. a bolt moves `speed` pixels along the long axis, with the error term
     for the short axis; at the end, its slot is freed;
  3. the pattern run advances; a burst's last pattern frees its slot;
  4. the sprite is built at the position less the camera.
- **`nge_vfx_busy()`** is a count the last update worked out, or true if a
  VFX started since, as `nge_render_busy()` (animations.md D5).
- **Arrival for D4:** `nge_vfx_burst()` looks for a bolt in use whose end
  is (x, y) and takes `delay = max(delay, its delay + major >> shift)`.

### 5.2 The game: the anim module (`game/anim/`, pages 126/127)

`anim_on_event()` gains the cases below; a new `anim_vfx.c` holds them, in
page 126 with the rest of the module, and the looks table (D8) goes in
page 127.

| Event | Shows |
|---|---|
| `EV_FIRED` (target, source = shooter, p1 = launcher or thrown item) | a bolt from the shooter's tile to the target's, in the ammo group's look |
| `EV_CAST` (target, source, p1 = SP_*) | target ≠ source: a bolt in the spell's effect's bolt look, if it has one |
| `EV_AIMED` (target, source, p1 = item) | target ≠ source: a bolt in the item's aim effect's bolt look |
| `EV_EFFECT_STARTED` (target, p1 = EF_*) | the effect's glow, if it has one, with no delay (D4 adds one after a bolt) |
| `EV_DAMAGED` (target, p1 = taken, p2 = type) | taken > 0: the hit flash if the target is alive (D11), then the type's splash, both with `ANIM_IMPACT_DELAY` (D10) |
| `EV_DIED` (target) | the stand-in (D9), if the target is shown |

- A tile's point is where a 16 × 16 sprite on it is drawn: (tile × 16,
  tile × 16) in map pixels, the same place the render system draws the
  creature. The creatures' draw offset is 0 today; if one is
  set later, the stand-in reads it with the pattern.
- The existing lunge on `EV_HIT` and `EV_MISSED` still plays for a shot: it
  reads as the shooter's throw or draw, and plays its attack sequence.
- A target or source not on the current level (`LEVEL_AREA`) shows
  nothing.
- **`anim_holding()`** becomes `nge_render_busy() || nge_vfx_busy()`, the
  second only asked if the first is false.

### 5.3 The game: frames and resets (`game.c`, `level/`)

- `game_frame_start()` calls `nge_vfx_upload()` after
  `screen_overlay_upload()`; `game_frame_end()` calls `nge_vfx_update()`
  after `nge_render_update()`. Overlay screens pause it as they pause the
  render system.
- `nge_vfx_init()` at start-up with the render system; `nge_vfx_clear()`
  on a change of level and a new game (R14).

### 5.4 The art (`assets/Sprites.aseprite`)

In the Effects share (14 patterns, R18), drawn to sprite_style.md, in
slots that recolour by bank (the primary ramp 9 to 12):

| Tag | Patterns | Use |
|---|---|---|
| `fx.arrow` | 2 | straight and diagonal |
| `fx.thrown` | 1 | a spinning dagger or stone: mirror flags make the spin |
| `fx.orb` | 1 | magic: a round orb that needs no direction; the mirror flags can make it shimmer |
| `fx.glow` | 3 | rising sparkles, once |
| `fx.splash` | 3 | a burst outwards, once |
| `fx.puff` | 2 | the death puff |

12 patterns, 72 + 12 = 84 of 128. The hit flash and the stand-in use the
creatures' own patterns. **Agreed** (2026-11-08): an orb for magic, not a
directed bolt.

### 5.5 Call depth and cost

- `anim_on_event()` calls `nge_vfx_*()`: the same depth as its render
  calls today (animations.md 5.6). `EV_CAST` and `EV_AIMED` add one
  banked call to find the effect (D8).
- `nge_vfx_update()`: one banked call a frame. With no VFX in use it
  returns after a byte test. With 12 in use, a few additions and
  comparisons each: well under 1 ms (R16).
- `nge_vfx_upload()`: 12 sprite records a frame, or none if nothing
  changed since the last upload.
- Common memory: two calls in the frame functions and one in
  `anim_holding()`, about 20 bytes (R17).

## 6. Scope

In:

- R1 to R18 and R22, as in section 5;
- the art of 5.4, for review by the workflow of sprite_style.md section 7;
- PC tests (section 7);
- docs: `src/nge/README.md` (the VFX module), `src/game/README.md` (the
  anim module's VFX), `docs/memory_map.md` (the new pair, the sprite
  numbers), animations.md (R14 pointing here), sprite_style.md 6.8 (the
  tags in the Effects share).

Out, for later:

- particles, area effects, cast gestures, beams (R19, R20);
- damage numbers (R21).

## 7. Tests

PC tests:

- **`test_vfx`** (new):
  - a burst plays its patterns once at its rate, then frees its slot;
  - a delay hides it and holds it for that many frames;
  - a bolt reaches its end in (length / speed) updates, along each of
    the 8 directions and a knight's move, and frees its slot;
  - a burst at a bolt's end starts at its arrival (D4); a longer delay of
    its own wins; a burst elsewhere doesn't wait;
  - `nge_vfx_busy()`: true from a start until the update that frees the
    last slot, including delays;
  - out of view: a VFX with both points outside isn't started and isn't
    busy;
  - pool full: `NGE_VFX_NONE`, nothing else changes;
  - `nge_vfx_dir_flags()` for the 8 directions;
  - `nge_vfx_clear()` frees every slot.
- **`test_anim`:**
  - `EV_FIRED` starts a bolt from the shooter to the target;
  - `EV_CAST` at another creature starts the spell's bolt; at the caster,
    none;
  - `EV_DAMAGED` starts the hit flash (the target's pattern in the flash
    bank) and the type's splash, with the impact delay; with 0 taken,
    neither; for a target killed by the blow, the splash but no hit
    flash;
  - `EV_EFFECT_STARTED` starts the effect's glow, or none for an effect
    without one (an innate one, a damaging bolt);
  - `EV_DIED` starts the three bursts of a stand-in, timed to a bolt in
    flight;
  - `anim_holding()` is true while only a VFX plays.

CSpect and hardware checks:

1. Fire a bow, throw a dagger, at 1 tile and 8 tiles, straight,
   diagonal and at a knight's angle: the shot flies and points the right
   way; the splash shows as it lands; a miss has no splash.
2. Cast Magic Bolt, Fire Bolt and Frost Bolt at a monster: each flies in
   its colour, and the splash is purple, orange and ice.
3. Cast Cure Wounds and Haste on yourself, Hold and Enfeeble on a
   monster; drink a potion: each glows in its colour; Hold flies first.
4. Kill a monster with a shot and with a sword: it stays until the blow
   lands, flashes once and puffs.
5. Hit a monster without killing it, with a shot and a sword, and get hit
   by one: the creature hit flashes white as the splash starts, with no
   pixels of the old pattern showing at its edges.
6. A monster poisoned by the player shows a lime splash each tick.
7. Shoot at a monster at the edge of the view: the bolt is clipped at the
   edge, not drawn over the UI.
8. While a bolt flies, keys typed wait and are taken after it lands.
9. Take the stairs straight after a shot: nothing is left on the new
   level.
10. Frame time on hardware with several VFX and the most creatures in view.

## 8. As built: differences from the design

- **The first sprite is a parameter, not `NGE_VFX_SPRITE`:**
  `nge_vfx_init(first_sprite)`. The engine doesn't know the game's
  overlay sprites (nge never includes game code), so the game names it:
  `SCREEN_SPRITE_VFX` (68) in `game/ui/screen_main.h`, from `main()`.
- **A look is a struct** (`nge_vfx_look_t`: first, count, rate, flags,
  mode) passed by pointer, on the caller's stack, instead of seven
  arguments.
- **Directions are the engine's:** a look with `NGE_VFX_DIRECTED` gets
  its rotate and mirror flags, and its diagonal pattern (`first + count`),
  from `nge_vfx_bolt()`. There is no public `nge_vfx_dir_flags()`.
- **The stand-in's first burst is a hold** (`NGE_VFX_HOLD`): shown with
  its first pattern during its delay, ended when the delay ends. D9's "a
  burst with no delay whose rate is that delay" can't work beside D4,
  which delays every burst at a bolt's end, the stand-in's too.
- **`nge_vfx_arrival(x, y)` is public,** so the stand-in's puff can start
  `ANIM_FLASH_FRAMES` after the flash: D4's own rule gives the flash and
  the puff the same start.
- **Arrival is a division, not a shift** (D5): the speed can be any value,
  and the division is only made when a burst is asked for. A bolt is drawn
  until the update before it arrives, and freed on that update, when its
  impact first shows.
- **`ANIM_IMPACT_DELAY` is 3, not 4** (D10): with a delay of 3 the splash
  first shows on the 4th update, the one where the lunge reaches its peak.
- **The VFX's frame rate is the game's,** `ANIM_FX_RATE` (3 frames,
  60 ms), not the file's: `scripts/sprite_grids.py` writes 100 ms for
  every frame that isn't a creature's, which made a hit hold the world
  for 0.3 s.
- **Shots:** an arrow (red fletching) for bows and crossbows (`GROUP_ARROW`,
  `GROUP_BOLT`), the stone for everything else: slings, thrown items and a
  creature's own ranged attack.
- **The colours of D6, by effect,** are in `anim_ef_fx[]`
  (`game/anim/anim_vfx.c`): poison glows lime, mana blue; the pending
  effects (light, identify, map, teleport, uncurse, enchant, summon) have
  colours ready; innate (`EF_REGEN`, `EF_ROOT`, `EF_MANA_REGEN`), worn and
  food effects show nothing.
- **The art breaks the checker's item rules** (body box, edges, joined
  pixels): an exception in sprite_style.md section 8.
- **Common memory lost 60 bytes,** not 20 (5.5): the calls in
  `game_frame_start()`, `game_frame_end()`, `main()`, `play_start()`,
  `change_level()` and `anim_holding()`. 4,277 bytes are free in debug
  builds. Page 92 holds 3.1K, page 126 2.5K.
