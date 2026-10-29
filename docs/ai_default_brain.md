# AI: the default brain

Status: implemented (2026-10-29). The CSpect checks in section 8 passed
(2026-10-29), check 3 with tracking (R9a) and the south-door case; check
5 needs the goblin idle, out of the player's sight (a hunting goblin
walks over items, D10: kept). Not yet tried on hardware. Q1 to Q5 answered in
section 10, D5 changed to line of sight in nge (Q2). Section 11 lists
where the build differs from the design.

The first brain that plays. `AI_BRAIN_DEFAULT` today sleeps until hurt,
then stands still (idle). This step gives it the behaviour of a basic
dungeon monster:

- it wakes when it hears the player nearby;
- it notices the player when they are in sight;
- it hunts them, opens doors on the way if it can, and fights;
- it gives up when it loses track of them;
- while idle it wanders, and picks up and wields what it finds;
- when hurt it drinks a healing potion, if it has one and the wit to.

It also fixes **the pattern every state follows**, and the API behind it,
so that later states (flee, ranged attack, cast) are new options on the
same frame. And it adds **a wait action**, for the AI and for the player.

Builds on [ai.md](ai.md): the module, the state machine, `ai_can()` and
`ai_try()` (its R and D numbers are written "ai.md R18" here). Uses the
actions in [act.h](../src/game/actions/act.h) and the range test in
[targeting.md](targeting.md) (`nge_map_within()`). Adds a line test to
nge's block component (`nge_block_line()`, D5).

## 1. Goal

- Monsters with the default brain are a challenge: an awake monster that
  sees the player comes for them and attacks, and a sleeping one can be
  sneaked past, but not for long.
- What a monster does depends on what it can do (`ai_can()`): the same
  brain makes a rat and a goblin behave differently. A rat (Int 2, no
  inventory) can't open a door, so a closed door stops it; it never picks
  anything up. A goblin (Int 10, an inventory with a weapon slot) opens
  doors, picks up a sword and wields it, and drinks a healing potion when
  hurt. An Intelligence drain makes a goblin stop at doors like a rat.
- Every state is written the same way: a context, then options tried in
  order of priority until one uses the turn, else wait. A new behaviour
  is a new option or a new state, with no change to the frame.

## 2. Answers to the questions

### Is there a standard pattern for a state?

Yes. Every state's turn has the same steps, which match your list. Steps
(a) and (f) are shared and run outside the state. Steps (b) to (e) are
the state itself:

| Step | What happens | Who does it |
|---|---|---|
| a. Context | Work out what the entity knows this turn: its tile, its capabilities, its target and whether it sees it. This may give a stimulus (it saw the player), which changes the state before it acts | `ai_sense()`, shared, once per turn |
| b. Evaluate | Each option checks cheaply, from the context alone, whether it applies (has the capability, is in reach, is hurt) | each option |
| c. Try | The first option that applies fills an `act_t` and calls `ai_try()`: check, then do | each option |
| d. Used: stop | `ai_try()` returned 1: the state returns | the state |
| e. Not used: next | The action was refused (a wall, a locked door): the state goes on to its next option | the state |
| f. Nothing: wait | No option used the turn: the entity waits (`ACT_WAIT`) | `ai_on_turn()`, shared |

A state is therefore a list of options in order of priority. The hunt
state is "attack if next to it, else drink a potion if badly hurt, else
move towards it". (b) is "evaluate", but not a scoring of every action:
the order is the evaluation. That is cheap on the Z80 (D3). An option can
still weigh choices inside itself later.

### Should a standard API be defined for these steps?

Yes, and a small one (D2, D4):

- `ai_ctx_t`: the turn's context, built by `ai_sense()` on the stack of
  `ai_on_turn()` and passed by pointer to the state and its options;
- `uint8_t think_<state>(ai_ctx_t *c)`: a state's handler, 1 if the turn
  was used;
- `uint8_t opt_<name>(ai_ctx_t *c)`: an option, 1 if the turn was used;
- `ai_try()` (exists), `ai_step_toward()` and `ai_in_sight()`: what the
  options are built from (`ai_in_sight()` is the range test and nge's
  line test, `nge_block_line()`);
- `c->stim`: what the turn noticed while acting (it reached where the
  player was and they are gone), passed to the brain after acting.

The handlers and options are functions in code, not data lists (D3).

### Should a common context struct be reused by the states?

Yes: one `ai_ctx_t` for every state (D4). It holds what more than one
state or option needs, worked out once per turn: the capabilities (one
banked call of `ai_can()`), the entity's tile and area, its target and
where it thinks the target is, and whether it sees it. A state that needs
something only it uses works that out itself, and the struct grows only
when a second user appears. A context per state was rejected: the states
share most of what they read, and `ai_on_turn()` would need one per state.

### A "do nothing" action?

Yes: `ACT_WAIT`, an action like the others (R13 to R16, D9). The AI
waits through it when no option applies, so every AI turn ends in exactly
one action. The player gets a wait key (`.`), which they don't have yet:
today a player who wants a monster to come to them has to walk into a
wall, and that is refused, so it doesn't use the turn.

## 3. Terms

As in ai.md section 3, and:

| Term | Meaning |
|---|---|
| Context | `ai_ctx_t`: what an AI entity knows on this turn, built by `ai_sense()` |
| Option | Something a state may do this turn (attack, drink, approach, wander): a function `opt_<name>()` that checks whether it applies and, if so, tries one action |
| Target | Who the entity is after (`ai_target[]`): the player, or whoever hurt it. Kept between turns |
| Goal | Where it thinks its target is (`ai_goal_x[]`, `ai_goal_y[]`): where it last saw or felt it. Kept between turns |
| In sight | Within `AI_SIGHT` of it (`nge_map_within()`), with no wall or closed door on the line between (`nge_block_line()`) |
| Hear | Notice the player within `AI_HEAR` while asleep, by a Wisdom roll each turn |

## 4. Requirements

Marked **Changed** where it differs from the request, **New** where it
adds to it, with the reason.

### The pattern

- **R1** Every AI turn runs the same frame (`ai_on_turn()`):
  1. sense: build the context (`ai_sense()`). A stimulus from sensing
     goes to the brain at once, before acting;
  2. think: the current state's handler tries its options in order of
     priority, until one uses the turn;
  3. wait: if none did, the entity waits (`ACT_WAIT`);
  4. a stimulus the handler set (`c->stim`) goes to the brain: the new
     state acts next turn.
- **R2** A state's handler is a list of options, each tried only if the
  previous ones didn't use the turn. An option first checks, from the
  context and with no banked call, whether it applies (capability bits,
  distance, hit points), and only then tries its action with `ai_try()`.
  A refused action costs nothing, and the next option is tried.
- **R3** One context struct (`ai_ctx_t`) for every state, on the stack of
  `ai_on_turn()`, never in page 131. Capabilities are worked out once per
  turn, in it.
- **R4** Options are shared: an option is written once and any state may
  list it. A state that needs a new behaviour adds an option; the frame
  doesn't change.
- **R5 (New)** Options and states only read the context and act through
  `ai_try()`. They never change the AI's page data directly: the state
  changes through stimuli (ai.md R3), and the target and goal through
  `ai_sense()` and `ai_stimulus()`. Reason: one place for each piece of
  state, which the PC tests can check.

### Senses

- **R6** A sleeping entity hears the player: each turn the player is on
  its level within `AI_HEAR` (4) tiles, it rolls 1d20 + its Wisdom
  modifier (`STATS_MOD()`), and on `AI_WAKE_DC` (15) or more gets
  `AI_STIM_NOISE`. Walls don't stop hearing. A rat (Wis 10) wakes on 30%
  of its turns near the player, a goblin (Wis 8) on 25%.
- **R7** An awake entity looks: the player is in sight if they are on its
  level, within `AI_SIGHT` (8) tiles (`nge_map_within()`), and no wall or
  closed door is on the line between them. In sight, its target becomes
  the player, its goal their tile, and it gets `AI_STIM_SEEN`.
- **R8** Being hurt (ai.md R6, `AI_STIM_HURT`) also sets the target to
  the source and the goal to the source's tile, if the source is on the
  same level. The entity turns on whoever hurt it, even unseen.
- **R9 (New)** A hunting entity that reaches its goal and doesn't see its
  target, or can't move towards its goal at all, gets `AI_STIM_LOST`.
  Reason: without it a monster that loses the player stays hunting a tile
  for ever.

- **R9a (New, after CSpect check 3)** A hunting entity that doesn't see
  its target goes first to where it last saw it, or was hurt by it.
  There, still not seeing it, for `AI_TRACK` (3) of its turns its goal
  follows the target's real tile: it hears where they went. Then the goal
  stays at the last tile, as R9. Reason: at speed 20 the player moved
  through a door and shut it between two of a goblin's looks; its goal
  was on its own side of the door, and it went idle there. Tracking from
  the moment sight was lost (the first build of this) sent a monster
  straight at the player through a wall, when they left by a south door
  and stepped aside: it never went to the doorway, and gave up.

### The default brain

- **R10 (Changed)** Three states: asleep, idle and hunt (new):

  | State | Senses | Options, in order |
  |---|---|---|
  | `AI_STATE_ASLEEP` | hears (R6) | none: it waits |
  | `AI_STATE_IDLE` | looks (R7) | pick up an item here; wield a weapon it carries; wander |
  | `AI_STATE_HUNT` | looks (R7) | attack the target next to it; drink a healing potion if at half hit points or less; step towards the goal |

  Reason (Changed): the request asked for basic actions. Flee and use of
  ranged attacks are left for later brains (section 7), so this step
  shows the pattern with as few states as can fight.
- **R11 (Changed)** The transitions:

  | From \ stimulus | `HURT` | `NOISE` | `SEEN` | `LOST` |
  |---|---|---|---|---|
  | `ASLEEP` | `HUNT` | `IDLE` | – | – |
  | `IDLE` | `HUNT` | – | `HUNT` | – |
  | `HUNT` | – | – | – | `IDLE` |

  – is `AI_STATE_NO_CHANGE`. Reason (Changed): hurt while asleep goes
  straight to hunt, not to idle as now: it knows who hurt it (R8), so
  standing still would be wrong. The message is still "wakes up" (from
  asleep).
- **R12** Capabilities decide which options apply (`ai_can()`, ai.md D3):

  | Option | Needs | Rat | Goblin |
  |---|---|---|---|
  | attack | `AI_CAN_MELEE` | yes | yes |
  | open a door on the way | `AI_CAN_OPEN` | no: stops at the door | yes |
  | pick up | `AI_CAN_PICKUP` | no | yes |
  | wield | `AI_CAN_EQUIP` | no | yes |
  | drink | `AI_CAN_USE` | no | yes |
  | wander, step towards | nothing | yes | yes |

### Waiting

- **R13 (New)** An action `ACT_WAIT`: the actor waits one turn. Check
  mode always succeeds; do mode uses the turn and posts no event. Reason:
  R1's step 3, and the player's wait.
- **R14 (New)** The AI never ends a turn without an action: what was
  "not acting is waiting" (ai.md R9) is now `ACT_WAIT`.
- **R15 (New)** The player's `.` key waits a turn (`player_act(ACT_WAIT)`).
- **R16 (New)** Waiting says nothing: no message.

### Movement

- **R17** Stepping towards a tile tries the direct direction (as built:
  the first tile of the line of sight to it, section 11), then the two
  directions either side of it (45° left, then right). Bumping into a closed door, with
  `AI_CAN_OPEN`, opens it instead (as the player's move does,
  `player_bank.c`'s `move()`). Bumping into another creature tries the
  next direction. No pathfinding.
- **R18** Wandering: on half of its idle turns the entity steps one tile
  in a random direction, if it can. It doesn't open doors while
  wandering: a goblin walks through an open door, but doesn't open one
  for nothing.

### Messages

- **R19** The player is told when a monster they can see (`ai_sees()`
  from the player) does something with its AI: "The goblin notices you."
  (idle to hunt), "The rat wakes up." (from asleep, as now), "The goblin
  opens the door.", "The goblin picks up a sword.", "The goblin wields a
  sword.", "The goblin drinks a potion of healing." A monster's attacks
  are said as now ("The rat hits you.").
- **R20 (Changed)** "wakes up" is now said only for a monster in sight.
  Reason: ai.md section 11 said it for any monster, because nothing could
  tell what the player sees. `ai_sees()` can now.

### Data and memory

- **R21** Per AI entity, in page 131: its target (`ai_target[]`) and goal
  (`ai_goal_x[]`, `ai_goal_y[]`), 300 bytes. `ai_add()` sets them to none
  (`NGE_ENTITY_INVALID`, `NGE_MAP_X_INVALID`).
- **R22** No common-memory growth beyond `ACT_WAIT`'s case in `act_do()`.
  Common memory has 109 bytes left (ai.md section 11).

## 5. Decisions

- **D1 Sense, then act (R1).** Sensing comes first and a stimulus from it
  changes the state before the handler runs, so an idle goblin that sees
  the player hunts on the same turn. A stimulus from acting (`LOST`)
  changes it after, for the next turn. This narrows ai.md D6 ("from a
  turn, the new state acts next turn"): there is still one handler per
  turn, so no double think and no loop. **Rejected:** keeping D6 for
  sensing too: an idle monster that has just seen the player would wander
  for a turn first. **Rejected:** sensing inside each state's handler:
  every state would repeat it, and the stimulus would arrive after the
  state had already chosen.

- **D2 The API (R1 to R5).** In `ai_internal.h`:

  ```c
  /* What an AI entity knows on this turn: ai_sense(), on the stack */
  typedef struct ai_ctx {
      nge_entity_id_t id;       /* the entity */
      ai_can_t        can;      /* ai_can(): this turn's capabilities */
      nge_owner_t     area;     /* its area, and its tile */
      uint8_t         x;
      uint8_t         y;
      nge_entity_id_t target;   /* ai_target[]: NGE_ENTITY_INVALID for none */
      uint8_t         gx;       /* ai_goal_x/y[]: NGE_MAP_X_INVALID for none */
      uint8_t         gy;
      uint8_t         sees;     /* 1: the target is in sight, at (gx, gy) */
      ai_stim_t       stim;     /* out: what acting noticed (R9), AI_STIM_NONE */
  } ai_ctx_t;

  /* Fill c for entity id; c->stim: what sensing noticed (R6, R7) */
  void ai_sense(nge_entity_id_t id, ai_ctx_t *c);

  /* Step c->id one tile towards (x, y): R17. 1 if the turn was used */
  uint8_t ai_step_toward(ai_ctx_t *c, uint8_t x, uint8_t y);

  /* 1 if (x1, y1) is in sight from (x0, y0) in area: within AI_SIGHT, and
     nge_block_line() with NGE_BLOCK_SOLID (R7, D5) */
  uint8_t ai_in_sight(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
  ```

  `ai_sense()`, `ai_step_toward()` and the options are not banked: they
  are in page 130 and called only from page 130, as the actions' helpers
  are in page 112 (`act_internal.h`). `ai_in_sight()` also gets a banked
  public wrapper, `ai_sees(viewer, id)`, for the messages (R19).
  **Rejected:** the context in page 131 (a pointer to it dies on the
  first banked call, `ai_internal.h`).

- **D3 States and options are code: ordered function calls.** A state is
  short:

  ```c
  static uint8_t think_hunt(ai_ctx_t *c)
  {
      return opt_attack(c) || opt_drink(c) || opt_approach(c);
  }
  ```

  An option has the same shape every time:

  ```c
  /* Attack the target if it is next to us: needs AI_CAN_MELEE */
  static uint8_t opt_attack(ai_ctx_t *c)
  {
      act_t a = { 0 };

      if (!(c->can & AI_CAN_MELEE) || !c->sees) {   /* b. applies? */
          return 0;
      }
      a.actor = c->id;                              /* c. try */
      a.target = c->target;
      return ai_try(ACT_MELEE, &a);                 /* d./e. used or not */
  }
  ```

  **Rejected:** each state's options as a constant list of option numbers
  (`AI_OPT_*`) run by a shared loop through a `switch`. It would let a
  brain carry its own order of options as data. But it adds a dispatcher
  and an indirection for a choice no brain needs yet. When a second brain
  needs a different order, a list per brain and state replaces the
  `return a || b || c;` line, and the options don't change. **Rejected:**
  scoring every option each turn (utility AI, ai.md D1): an `ACT_CHECK`
  banked call per option per turn, and the order already says what the
  default brain prefers. **Rejected:** function pointers to options
  (G-ARCH-7, as ai.md D2).

  The zsdcc note on `return (x = 1, 0)` (memory) doesn't apply to `||` of
  calls, but the asm of `think_hunt()` is read (section 9).

- **D4 One context for every state (R3).** As section 2. It holds only
  what at least two options or states use. `hp_low` is not in it: only
  `opt_drink()` uses it, and works it out. **Rejected:** a context per
  state; **rejected:** no context, each option looking everything up (one
  `ai_can()` banked call per option).

- **D5 Line of sight by Bresenham, in nge's block component (R7, Q2).**
  A line test is engine code (the map and blocking entities), and
  targeting's line of fire will want it, so it goes in nge now:

  ```c
  /**
   * @brief 1 if nothing blocks the line from (x0, y0) to (x1, y1) in area:
   *        no tile strictly between the two ends has, in the map's flags
   *        or those of an entity on it with this component, any of the
   *        flags in block. NGE_BLOCK_EDGE always blocks. The ends aren't
   *        tested (a monster in a doorway sees out). Bresenham's line,
   *        from (x0, y0).
   */
  uint8_t nge_block_line(nge_owner_t area, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                         nge_block_t block) NGE_BANKED;
  ```

  Sight is `nge_block_line(area, ..., NGE_BLOCK_SOLID)`: walls and closed
  doors (`block_shut` is `NGE_BLOCK_SOLID`) block it, creatures
  (`NGE_BLOCK_OCCUPIED`) don't. A line of fire later passes
  `NGE_BLOCK_SOLID | NGE_BLOCK_OCCUPIED`, or tests the first creature
  itself. The entity test is `nge_block_find()` with pass `~block` (it
  finds an entity whose flags have any of `block`), called only for a
  tile the map lets through.

  **Where:** a new file, `ecs/components/nge_block_line.c`, banked in a
  new nge ECS pair, **56/57** (the next free pair in 48-87,
  docs/memory_map.md): the block component's own code is in common
  memory, which has no room (109 bytes). Banked code there reads the map
  (page 51, slot 1) and the block tables (page 50, slot 0), which are
  always mapped, so it needs nothing from any other page. Page 56 has
  about 0.2K of code; page 57 is unused. **Rejected:** in the map's
  set-up page (44): the map module is below the ECS (`nge_block.h`
  includes `nge_map.h`), so it can't test entities. **Rejected:** in the
  AI's page (the first draft): engine code in a game module, moved again
  when targeting needs it.

  The AI calls it only after `nge_map_within()` passes, so a monster far
  away costs one range test; at range 8 the line has at most 7 tiles.
  Bresenham isn't symmetric: the player may see a monster that doesn't
  see them, on rare lines. Accepted for now. **Rejected:** sight through
  closed doors (map walls only): cheaper, but a monster would see the
  player through a door, and a rat would then wait at it for ever.

- **D6 Senses depend on the state (R6, R7).** `ai_sense()` hears when
  asleep and looks otherwise. Hearing uses Wisdom, so a sleeping monster
  is a gamble the player can take, and Wisdom has a use. **Rejected:**
  waking by distance alone (no roll): sneaking past a monster would be
  impossible. **Rejected:** a sight range from Wisdom now: one more number
  to tune before there is anything to tune it against (ai.md R18 keeps it
  for later).

- **D7 Target and goal in page 131 (R21).** 300 bytes, beside the state
  and brain (page 131 uses 200 of 8K). The target is kept, not assumed to
  be the player, so a monster can later fight a charmed ally or another
  monster with no change to the options. **Option:** no `ai_target[]`,
  the target always the player (100 bytes less). **Rejected:** the
  options would name `actor_player`, and every one would change when
  there is a second kind of target.

  `ai_sense()` drops a target that is no longer alive or on the level
  (target and goal to none). A hunting entity with no goal gets
  `AI_STIM_LOST`. A persistent monster parked with a goal on another
  level finds it stale when the player returns. It then fails to step
  (R9), or reaches it and sees nothing: both are `LOST`.

- **D8 Movement without pathfinding (R17).** The direct step and its two
  neighbours, which brings a monster round a pillar or along a corridor
  wall, but not round a long wall or a U-shaped room. In a dungeon of
  rooms and corridors that is enough for a monster that sees you (sight
  is a straight line, so the way is mostly open). **Rejected for now:** a
  distance map from the player (60 × 40 = 2400 bytes, rebuilt each player
  move), the usual roguelike answer, later if monsters get stuck too
  often in CSpect.

- **D9 `ACT_WAIT` is an action (R13 to R16).** In the actions page, with
  the four steps (it has none to do). It posts no event: no system needs
  one, and every monster would post one every turn it waits.
  **Rejected:** an `EV_WAITED` event (later, for a "rested" effect or the
  demo, it is one line). **Rejected:** waiting as "no action" (ai.md R9):
  the AI couldn't tell "nothing to do" from a forgotten case, and the
  player couldn't wait at all.

- **D10 Picking up and wielding (R10, R12).** `opt_pickup()`: idle, with
  `AI_CAN_PICKUP`, an item on its tile: `ACT_PICKUP` with the first item
  (the action refuses if it is full). `opt_wield()`: idle, with
  `AI_CAN_EQUIP`, no weapon in `SLOT_WEAPON` (`attack_weapon()`): tries
  `ACT_EQUIP` with `kind = SLOT_WEAPON` on each item it carries, the
  first the check accepts. No "better weapon" comparison: a goblin keeps
  the first weapon. The quest item can be picked up like any item; a
  monster that dies drops what it carries (`level.c`), so it is never
  lost. **Rejected:** monsters going to items they see: a goal kind, and a
  choice between the player and an item, for a later "greedy" brain.

- **D11 Drinking (R10, R12).** `opt_drink()`: hunting, with
  `AI_CAN_USE`, hit points at half or less (`health_hp * 2 <=
  health_hp_max`), carrying an item whose use effect (`gen_hook(item,
  GEN_HOOK_USE)`) is `EF_HEALING`: `ACT_USE`. Only healing: the AI
  doesn't yet know what the other effects are for. **Option:** a table of
  "AI uses" per effect (heal when hurt, cure when poisoned). Later, with a
  second use.

- **D12 Messages gated by sight (R19, R20).** The messages module calls
  `ai_sees(actor_player, id)` before saying an AI message. It is banked
  (page 130), called from page 118's banked code: it is one banked call
  per AI message, not per event. The other messages about monsters ("The
  rat dies.") stay ungated until there is a field of view. **Rejected:**
  gating nothing (the player hears a goblin across the level pick up a
  sword).

- **D13 Tracking after sight is lost (R9a).** One byte per AI entity in
  page 131, `ai_track[]`: set to `AI_TRACK` by seeing the target
  (`ai_sense()`) and by being hurt (`ai_stimulus()`), cleared by `LOST`
  and when the target is dropped. On a turn it doesn't see its target,
  with tracking left, and either at its goal (the last sighting) or
  already following (`ai_track[]` below `AI_TRACK`), `ai_sense()` sets the
  goal to the target's tile and counts down. So the count needs no flag
  of its own to say tracking started. Only the target is tracked, and
  only while awake, so an idle entity that lost its target stays lost.
  **Rejected:** tracking from the moment sight is lost (the first build):
  the line to the player's real tile can cross a wall the player went
  round, where the line to the last sighting goes through the doorway. **Rejected:** doors make
  noise (`EV_OPENED`, `EV_CLOSED` drawing monsters within `AI_HEAR`):
  it fixes a door, not a corner, and needs a hook in the common event
  filter, where 83 bytes are left. Kept for later (section 7).
  **Rejected:** leaving it to the speeds: at equal speeds it happens
  less, not never.

## 6. Design

### 6.1 ai.h

```c
/* States */
#define AI_STATE_NONE    0
#define AI_STATE_ASLEEP  1   /**< Does nothing; wakes when it hears the player, or is hurt. */
#define AI_STATE_IDLE    2   /**< Awake, no target: picks up, wields, wanders; looks. */
#define AI_STATE_HUNT    3   /**< After its target: attacks, drinks, steps towards it. */
#define AI_STATE_COUNT   4
#define AI_STATE_NO_CHANGE  AI_STATE_NONE

/* Stimuli */
#define AI_STIM_NONE   0
#define AI_STIM_HURT   1   /**< Attacked: hit, missed or damaged. */
#define AI_STIM_NOISE  2   /**< Heard the player (asleep). */
#define AI_STIM_SEEN   3   /**< Saw its target. */
#define AI_STIM_LOST   4   /**< Lost its target: at the goal and not seen, or can't get there. */
#define AI_STIM_COUNT  5

/* Senses */
#define AI_SIGHT    8    /* tiles, nge_map_within() */
#define AI_HEAR     4
#define AI_WAKE_DC  15   /* 1d20 + Wis modifier */

/** @brief 1 if viewer sees entity id: both on the current level, in sight (D5). Any viewer, AI or not. */
uint8_t ai_sees(nge_entity_id_t viewer, nge_entity_id_t id) NGE_BANKED;
```

### 6.2 A turn (`ai_bank.c`)

```c
void ai_on_turn(nge_entity_id_t id) NGE_BANKED
{
    ai_ctx_t c;
    uint8_t used = 0;

    check_ai(id);

    ai_sense(id, &c);                                 /* a. context */
    if (c.stim != AI_STIM_NONE) {
        ai_stimulus(id, c.stim, c.target);            /* the new state acts now (D1) */
        c.stim = AI_STIM_NONE;
    }
    switch (ai_state[id]) {                           /* b.-e. the state */
    case AI_STATE_ASLEEP: used = 0; break;            /* sleeps */
    case AI_STATE_IDLE:   used = think_idle(&c); break;
    case AI_STATE_HUNT:   used = think_hunt(&c); break;
    default: nge_util_abort("bad ai state");
    }
    if (!used) {
        ai_wait(&c);                                  /* f. nothing: wait */
    }
    if (c.stim != AI_STIM_NONE) {
        ai_stimulus(id, c.stim, NGE_EVENT_NONE);      /* acts next turn */
    }
}
```

`ai_stimulus()` grows R8: on `AI_STIM_HURT` with a source on the same
level, it sets the target and goal before the transition. On `SEEN`,
`ai_sense()` has already set them.

### 6.3 The options (`ai_think.c`, new)

| Option | Applies when | Action |
|---|---|---|
| `opt_attack` | `AI_CAN_MELEE`, sees its target | `ACT_MELEE` on the target (the check refuses it out of reach) |
| `opt_drink` | `AI_CAN_USE`, hp ≤ half, carries a healing item | `ACT_USE` |
| `opt_approach` | has a goal | at the goal and not seeing: `c->stim = LOST`, 0; else `ai_step_toward()`; nothing moved: `LOST` |
| `opt_pickup` | `AI_CAN_PICKUP` | `ACT_PICKUP`, first item (refused if none here) |
| `opt_wield` | `AI_CAN_EQUIP`, nothing in its weapon slot | `ACT_EQUIP`, `SLOT_WEAPON`, each item carried in turn |
| `opt_wander` | a 1 in 2 roll | `ACT_MOVE` in a random direction (no doors) |

```c
static uint8_t think_idle(ai_ctx_t *c) { return opt_pickup(c) || opt_wield(c) || opt_wander(c); }
static uint8_t think_hunt(ai_ctx_t *c) { return opt_attack(c) || opt_drink(c) || opt_approach(c); }
```

`opt_pickup()` costs a goblin one `ACT_CHECK` banked call each idle turn;
an item test on its tile first (`nge_location_find()`, common) avoids
it.

### 6.4 The brain (`ai_brains.c`)

```c
[AI_BRAIN_DEFAULT] = {
    .next = {
        /*                  NONE                HURT           NOISE               SEEN                LOST */
        [AI_STATE_ASLEEP] = { AI_STATE_NO_CHANGE, AI_STATE_HUNT, AI_STATE_IDLE,      AI_STATE_NO_CHANGE, AI_STATE_NO_CHANGE },
        [AI_STATE_IDLE]   = { AI_STATE_NO_CHANGE, AI_STATE_HUNT, AI_STATE_NO_CHANGE, AI_STATE_HUNT,      AI_STATE_NO_CHANGE },
        [AI_STATE_HUNT]   = { AI_STATE_NO_CHANGE, AI_STATE_NO_CHANGE, AI_STATE_NO_CHANGE, AI_STATE_NO_CHANGE, AI_STATE_IDLE },
    },
},
```

Written with designated initialisers per stimulus, as now. The stride is
5 bytes; the table grows from 6 to 20 bytes a brain.

### 6.5 The wait action

`act_wait.c` (page 112): context and validate assert an alive actor;
check mode returns `act_checked()`; do mode returns `act_succeed()`.
`ACT_WAIT` is 11 and `ACT_COUNT` 12; `act_do()` gets its case (common
memory, a few bytes). `player_command()` gets `case '.'`. The start of
game hints (`screen_title_bank.c`) don't list every key, and stay as
they are.

### 6.6 Messages (`messages_bank.c`)

`EV_AI_CHANGED`: from asleep, "wakes up"; idle to hunt, "notices you".
`EV_OPENED`, `EV_PICKED_UP`, `EV_EQUIPPED`, `EV_USED` with an AI source:
"The <name> opens the <door>.", "picks up a", "wields a", "drinks a".
Each only if `ai_sees(actor_player, id)`.

### 6.7 Files and memory

| File | Change | Page |
|---|---|---|
| `ai.h` | states, stimuli, senses, `ai_sees()` | |
| `ai_internal.h` | `ai_ctx_t`, `ai_sense()`, `ai_step_toward()`, `ai_in_sight()`, `ai_wait()`; the page data | |
| `ai_bank.c` | `ai_on_turn()` as 6.2; `ai_stimulus()` R8; `ai_add()` sets target and goal | 130 |
| `ai_sense.c` (new) | `ai_sense()`, `ai_in_sight()` (range, then `nge_block_line()`), `ai_sees()` | 130 |
| `ai_think.c` (new) | the states' handlers and the options | 130 |
| `ai_act.c` | `ai_step_toward()`, `ai_wait()` | 130 |
| `ai_brains.c` | the table, 6.4 | 130 |
| `ai_pdata.c` | `ai_target[]`, `ai_goal_x[]`, `ai_goal_y[]` | 131 |
| `act.h`, `act.c`, `act_wait.c` (new) | `ACT_WAIT` | common, 112 |
| `nge_block.h`, `nge_block_line.c` (new) | `nge_block_line()` (D5) | 56 |
| `banks.inc`, docs/memory_map.md | pair 56/57: nge `ecs` block (the line test / unused) | |
| `player_bank.c` | `.` | 114 |
| `messages_bank.c` | 6.6 | 118 |

Estimates: page 130 grows by 1.5K to 2K (about 3K in all, of 8K); page
131 by 300 bytes; page 56 about 0.2K; common memory by under 10 bytes; pages 112, 114 and 118
by a few dozen bytes to a few hundred.

## 7. Scope

- **In:** the pattern and its API (R1 to R5, D1 to D4); hearing, sight
  and the hurt target (R6 to R9); `nge_block_line()` in nge, page 56
  (D5); the hunt state and the transitions
  (R10, R11); the options and their capabilities (R12, D10, D11);
  `ACT_WAIT` and the player's wait (R13 to R16); movement (R17, R18);
  the messages (R19, R20); target and goal (R21); docs: ai.md's later
  list and `game/README.md`; the pattern written up at the top of
  `ai_think.c`, as act.h writes up the actions' steps, and offered as a
  pattern for docs/guidelines/patterns.md.
- **Later:** flee and morale (`AI_STIM_LOW_HP`, a second brain that
  flees, ai.md R18); ranged attacks and spells as options
  (`AI_CAN_RANGED`, `AI_CAN_CAST`); noise from events (a door opened,
  combat); going to items in sight; pathfinding (D8); sight from Wisdom;
  the player's stealth; a field of view for the other messages; monsters
  using stairs; option lists as data per brain (D3); the dev-kit brain
  switch (ai.md R21).

## 8. Tests

PC tests (`test_ai`, linked with the actions; `test_act` for the wait):

- **The wait:** check succeeds and uses nothing; do uses the turn and
  posts nothing.
- **`test_block`, the line (nge):** clear, horizontal, vertical,
  diagonal and steep lines; a solid map tile between blocks, and at
  either end doesn't; an entity with the flag between blocks, one without
  it (an occupied tile, testing `NGE_BLOCK_SOLID`) doesn't, an entity in
  another area doesn't; `NGE_BLOCK_EDGE` blocks whatever `block` is;
  the same tile and next-door tiles are always clear.
- **Sight (`test_ai`):** a clear line in range; out of range (9 tiles); a
  wall between; a closed door between, then open; a creature between
  doesn't block; another level.
- **Hearing:** with `rand()` stubbed, the roll at 14 and 15 with Wis 10,
  and with Wis 8 (modifier -1); out of `AI_HEAR` no roll; awake entities
  don't hear.
- **The frame:** an idle entity that sees the player changes to hunt and
  acts the same turn (moves towards them); a hunting entity at its goal
  without the player becomes idle after its turn; every turn ends with
  one action (a stub counts `act_do()` calls in do mode).
- **Transitions:** every entry of 6.4; hurt while asleep goes to hunt
  with the source as target and its tile as goal; hurt with no source
  hunts with no goal, then `LOST`.
- **Options and capabilities:** a rat and a goblin hunting the player
  behind a closed door: the goblin opens it, the rat waits; with the
  goblin's Int lowered to 5, it waits too. Next to the player: both
  attack. A goblin idle on a sword: picks it up, then wields it next turn;
  a rat on a sword: wanders or waits. A goblin at half hp with a healing
  potion: drinks; at more than half: doesn't; Int 4: doesn't (no
  `AI_CAN_USE`). A goblin with a potion of strength only: doesn't drink.
- **Movement:** the direct step blocked by a wall: one of the side steps;
  all three blocked: `LOST`; blocked by another monster: a side step.
- **Target:** a dead target is dropped; a target on another level is
  dropped.
- **`test_gen`**: unchanged (the kind records don't change).

CSpect checks:

1. A new game: sleeping monsters stay asleep while the player is far away.
   Walk up to a rat and wait (`.`) next to it: within a few turns "The rat
   wakes up.", and it attacks.
2. A goblin that sees the player from across a room: "The goblin notices
   you.", and it comes and attacks.
3. Close a door between the player and a hunting goblin: "The goblin
   opens the door." Do the same with a rat: it stays behind the door, and
   after a while gives up (wanders).
4. Run out of sight of a hunting monster: it goes to where it last saw the
   player, then wanders.
5. Drop the dev kit's sword where a goblin will wander over it: "The
   goblin picks up a sword.", then "The goblin wields a sword.", and its
   hits do the sword's damage. Kill it: the sword is on the floor.
6. Give a goblin a potion of healing the same way, then hurt it to half:
   "The goblin drinks a potion of healing."
7. Speed: a level with every monster awake and hunting has no visible
   slowdown (several monsters' sight lines a turn).
8. The persistent goblin on depth 2: wake it, go up and come back: it
   doesn't hunt a stale goal for ever.

## 9. Risks

- **Cost of sight.** `nge_block_find()` looks at entities by ID for each
  tile on the line the map lets through, and every awake monster looks
  every turn, each look a banked call to page 56. Bounded by the range
  test first and 7 tiles a line. CSpect check 7 measures it; if it
  shows, sight is tested every other turn.
- **A new nge pair (56/57)** for 0.2K: the ECS range has 20 pairs and
  uses 4, so it costs nothing that is short. The block component's
  header now has a banked function beside its common ones: its banking
  note says so.
- **Monsters stuck** (D8): round long walls they give up (`LOST`) rather
  than loop; pathfinding is the fix if it happens often.
- **zsdcc** (memory notes): the context struct's fields read after banked
  calls (`ai_try()`, `ai_can()`): copy into locals, read the asm of
  `ai_sense()`, `opt_approach()` and `think_hunt()`; the signed dx, dy in
  `ai_step_toward()` (`int8_t`, no shifts of signed chars).
- **Page 130** stays far under 8K; nothing else risks memory.
- **Balance:** a goblin with a sword (1d8, +1 to hit) hits harder than
  with its claws (1d4);
  monsters that hunt make the first levels harder than now, where nothing
  moves. Tuned in CSpect: `AI_HEAR`, `AI_WAKE_DC`, `AI_SIGHT` are the
  knobs.

## 10. Questions

All answered (2026-10-29):

- **Q1 (R11):** hurt while asleep goes straight to hunt: agreed.
- **Q2 (D5):** line of sight in nge now: `nge_block_line()`, page 56
  (D5 rewritten).
- **Q3 (D3):** options as ordered function calls, lists as data only when
  a second brain needs its own order: agreed.
- **Q4 (D10, D11):** goblins start with nothing; CSpect checks 5 and 6
  use the dev kit's items.
- **Q5 (R15):** `.` is the wait key: agreed.

## 11. As built: differences from the design

- **Files:** as 6.7. `ai_on_turn()` moved from `ai_bank.c` to
  `ai_think.c`, with the states and options; `ai_bank.c` keeps the
  component, `ai_can()` and `ai_stimulus()`, so the generators' tests
  still link without the actions. `ai_sense.c` and `ai_think.c` join
  `ai_act.c` in the tests' `ACT_GAME_SRCS`. The tests that link the AI's
  senses define `actor_player`, as test_ui and test_target do.
- **`ai_step(c, d, open)`** (`ai_act.c`): one step in direction d (0
  north, clockwise to 7), opening a closed door in the way when `open`
  and `AI_CAN_OPEN`. `ai_step_toward()` calls it for the direct step and
  its two neighbours, with `open`; `opt_wander()` for a random direction,
  without (R18), instead of its own `ACT_MOVE`.
- **The step towards the goal follows the line of sight (R17, D8;
  found by CSpect check 3).** The design's direct step was the sign of dx
  and dy, which goes diagonal first. A goblin that saw the player through
  a doorway off its diagonal then reached the wall beside the door, found
  the step and both beside it blocked, and gave up without opening it.
  The direct step is now the first tile of the line `nge_block_line()`
  draws from the entity to its goal: the goal was seen along that line,
  so it leads through the doorway. The two side steps are kept. test_ai
  checks it (a goblin at (0, 0), a door at (4, 2), the goal (8, 4)).
- **Tracking (R9a, D13), added after CSpect check 3:** at the last
  sighting, the goal follows the target for `AI_TRACK` turns. First built
  to follow from the moment sight was lost, which a CSpect run showed
  sent a monster at a wall (R9a). Page 131 600 bytes; page 130 $1287.
- **The directions are worked out, not tables** (`dir_x()`, `dir_y()`):
  zsdcc's code for a `const` table in page 130 made the assembler warn
  "integer range" on the table's address.
- **Lost clears the goal, keeps the target** (`ai_stimulus()`): an idle
  entity that lost its target still looks for it first. `ai_sense()`
  drops a target that isn't alive in its area (D7).
- **The noise has no source:** `EV_AI_CHANGED` from hearing has source
  `NGE_EVENT_NONE` (a sleeping entity has no target). From sight, the
  source is the player; from hurt, the attacker.
- **Messages** (R19, R20): as 6.6, and "puts on a" for an item equipped
  in a slot other than the weapon's, "eats the" for food. "notices you"
  needs the change's source to be the player. Each asks `ai_sees()` from
  the player.
- **`opt_wield()`** tries `ACT_EQUIP` with `SLOT_WEAPON` on each item it
  carries and hasn't equipped, until one is accepted (D10).
- **`ai_wait()`** asserts the wait used the turn.
- **Memory:** page 130 $1114 (4.3K, from 1.1K; more than the 3K
  estimated); page 131 500 bytes; page 56 $128 (296 bytes); page 112
  $1B72 (6.9K, 1.1K left); page 118 $F66. **Common memory grew by 26
  bytes, from 109 to 83 free below the stack**, not "a few": all of it is
  `act_do()`'s case for `ACT_WAIT`, as a banked call costs that much in
  zsdcc's switch. R22's estimate was wrong.
- **The asm was read** for `think_hunt()` (each option called with `c`
  reloaded, stopping at the first that acts), `opt_approach()` (the
  context's offsets; `&c->stim` kept across `ai_step_toward()`),
  `ai_on_turn()` (the context's `stim` and `target` read from the stack
  after each call) and `ai_sense()` (the context written from locals
  after `ai_can()`).
- **Tests:** `test_block` 54 checks (the line: directions, walls and
  entities between and at the ends, the flags asked for, other areas,
  the edge); `test_act` 184 (the wait); `test_ai` 124 (the brain's table
  in full; hurt to hunt with the attacker as target and goal; sight and
  `ai_sees()`; hearing at the DC with Wis 10 and 8, and no roll out of
  range; idle to hunt and moving the same turn; attacking; lost at the
  goal and with every step blocked; a door off the diagonal, reached
  along the line of sight; side steps round a creature and a
  wall; the door opened by a goblin, not by a rat or a goblin at Int 5;
  picking up then wielding, and a rat that doesn't; drinking at half hit
  points, not above, not at Int 4, not a potion of strength; a target on
  another level or dead dropped; hurt with no source, then lost; the
  door shut in its face, opened; seen on its side of a door, then the
  door shut behind the player: followed through it; out by a south door
  and left, out of sight: through the doorway after them; unseen on the
  way, the goal kept and tracking not started; at the goal, tracking for
  `AI_TRACK` turns, then not). "Every
  turn ends with one action" isn't counted by a stub, as section 8
  planned: the turns' events show it, and `ai_wait()` asserts it.
