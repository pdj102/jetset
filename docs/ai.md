# AI: requirements and design

Status: implemented (2026-10-29); the CSpect checks in section 8 passed
(2026-10-29); not yet tried on hardware.
D1 to D13 agreed the same day, after a revision (types for the AI's
numbers, capabilities worked out from the entity each turn, D3, and one
brain to start, D2). Q1 to Q4 answered in section 10. Section 11 lists
where the build differs from the design.

The next step, [ai_default_brain.md](ai_default_brain.md), builds the
default brain on this framework: hearing and sight, the hunt state, a
target and goal per entity, the pattern for states and their options,
and `ACT_WAIT`. It changes three things here: hurt while asleep goes to
hunt, not idle (R11); a stimulus from sensing changes the state before
the entity acts (D6); and an AI turn always ends in one action, waiting
if nothing else applies (R9). Of R18, hunt, a target and noticing the
player (sight, and hearing with Wisdom) are done there.

The AI module decides what an entity with the AI component does on its
turn, through the same actions as the player's commands
([game/actions/act.h](../src/game/actions/act.h)). This step builds the
framework (the module, its page pair, the state machine, capabilities,
brains, the turn and event hooks) with the first state, **asleep**, and
the one state it wakes into, **idle**, which waits as every monster does
today. Behaviour that moves or fights (wander, hunt, attack, flee) is
later work on this framework.

Builds on [ranged_ai_components.md](ranged_ai_components.md) (the AI
component as a marker, D4 to D6 there) and [game_loop.md](game_loop.md)
(the turn engine).

## 1. Goal

- An entity with the AI component is controlled by the game: on each of
  its turns the AI module decides what it does.
- What an AI entity can do follows from what it is: its components (an
  inventory to pick things up, a melee attack or a weapon to fight, mana
  to cast) and its attributes (a low-intelligence creature never opens a
  door). It changes when they change.
- How it chooses between what it can do can differ between creatures
  (an animal flees when hurt, a goblin fights on): data, not code, and a
  new behaviour is one new state.
- The AI acts only through actions (`act_do()`), with the player's
  rules: it checks its options with `ACT_CHECK` and does one with
  `ACT_DO`.

## 2. Answers to the questions

### Is a state machine a good choice?

Yes, for this game. A turn-based roguelike monster is in one mode at a
time (asleep, wandering, hunting, fleeing), and what it does on a turn
depends mostly on that mode. A state machine fits the Z80 well: one byte
of state per entity, a `switch` per turn, transitions as a small table,
and everything testable on the PC. Events and the monster's own
observations are the obvious inputs to its transitions.

The alternatives, and why not:

- **Behaviour trees:** flexible, but they need an interpreter, per-node
  running state for each entity, and trees as data in a banked page. Too
  much code and memory for what a dungeon monster does, and harder to
  debug on the hardware.
- **Utility AI** (score every option each turn, take the best): every
  option is an `ACT_CHECK` call into the actions page, many banked calls
  per monster per turn. It is a good fit *inside* one state later (a
  hunting monster choosing between moving, attacking and using a potion),
  not as the whole design.
- **Goal planning (GOAP):** a search each turn: far too heavy.

So: a state machine, where **states choose actions** and **transitions
choose states**. A state's handler can still weigh options inside it.

### One state machine per class, or one shared one?

One shared machine, with three separate concerns:

| Concern | Question | What it is | Where |
|---|---|---|---|
| Capabilities | What can it do? | `ai_can()`: worked out from its components and attributes at the start of each turn | code, page 130 (D3) |
| Brain | How does it choose? | A transition table (state and stimulus to next state); later, temperament (when it flees, whether it chases) | data, one per brain (D2) |
| State | What is it doing now? | One byte per entity; one handler per state, shared by every brain | code and page 131 |

The states never ask "is this an animal?": they ask `ai_can(id) &
AI_CAN_OPEN`. So a rat and a goblin with their intelligence drained
behave alike at a door, and a goblin that picks up a sword can attack
with it.

The brain is what is left when the capabilities are taken out: two
creatures that can do the same things may still choose differently.
This step has one brain, `AI_BRAIN_DEFAULT`. A second is a new row of
transition data, added when a creature really behaves differently (an
animal that flees when hurt, undead that never flee). A creature that
needs behaviour no shared state gives (a mimic waiting to be touched)
gets a new state, which only its brain's table reaches.

- One machine per class (animal, monster, humanoid) would repeat most of
  each state's code per class in one 8K page, and every fix with it.
- One machine with every difference coded into it, `if (smart) ...` in
  every state, is the complexity to avoid: capabilities and brains keep
  those tests out of the states.
- Brains are also a development tool: a dev-kit key switching every
  monster to another brain compares two models in CSpect (R21, later:
  with a second brain to switch to).

## 3. Terms

| Term | Meaning |
|---|---|
| AI entity | An entity with the AI component (`AI_IS(id)`): every monster |
| State | What an AI entity is doing now (`ai_state_t`, `AI_STATE_*`): one byte per entity; each state has a handler that runs on its turn |
| Stimulus | Something that may change its state (`ai_stim_t`, `AI_STIM_*`): from an event (it was attacked) or from its own turn (it saw the player, later) |
| Capability | Something an AI entity can do this turn (`ai_can_t`, a mask of `AI_CAN_*`), worked out by `ai_can()` from its components and attributes |
| Brain | How an AI entity chooses (`ai_brain_t`, `AI_BRAIN_*`): a transition table, constant data (`ai_brain_def_t`). A creature kind names one |
| Think | One turn of an AI entity: its state's handler runs, may do one action, and may return a stimulus |

## 4. Requirements

Marked **Changed** where it differs from the request, **New** where it
adds to it, with the reason.

### Framework

- **R1** The AI is a state machine: each AI entity is in one state
  (`AI_STATE_*`), and each state has one handler, shared by every brain.
- **R2** Each AI entity has a brain (`AI_BRAIN_*`), named by its kind's
  record: its transition table. A new brain is a new row of data, with no
  new code. This step has one, `AI_BRAIN_DEFAULT`.
- **R3** The state changes only through the brain's transition table,
  given a stimulus: `next = table[state][stimulus]`, or no change.
- **R4** Stimuli come from events (R6) and from a state's handler on the
  entity's turn (R8).
- **R5** States act only through actions (`act_do()`), checking first
  with `ACT_CHECK` where they have a choice.
- **R5a (New)** What an AI entity can do (`ai_can()`) is worked out from
  its components and attributes when it is asked, at the start of each
  turn: never stored, so it is right after a change (an item equipped, an
  ability drained). States test it before choosing an action that needs
  it. Reason: the review of the first draft, where capabilities were the
  brain's.
- **R5b (New)** Rules that are the AI's alone live in `ai_can()` only
  (intelligence to open doors: the player can always open a door, so
  `act_open()` has no such rule). Rules every actor shares stay in the
  actions' checks, which `ai_can()` mirrors without replacing.
- **R5c (New)** The AI's numbers have types of their own: `ai_state_t`,
  `ai_stim_t`, `ai_brain_t`, `ai_can_t`, each a `uint8_t`. Reason: a
  field or argument says what it holds.

### Events

- **R6** The AI reacts to game events: an event about an alive AI entity
  can be turned into a stimulus for it. In this step: being attacked
  (`EV_HIT`, `EV_MISSED`, or `EV_DAMAGED`, target an AI entity) is
  `AI_STIM_HURT`.
- **R7** Reacting to an event only changes the AI's state; it never acts.
  An AI entity acts only on its own turn. (Otherwise an event would give
  it a free action, outside the timer's time.)
- **R7a (New)** A change of state posts `EV_AI_CHANGED` (target: the
  entity; source: what caused it, or none; p1: the old state; p2: the new
  one), so the messages and later systems can react to it. Reason: the
  wake-up is otherwise invisible, so it can't be checked in CSpect, and
  the player should be told.

### Turns

- **R8** The timer system decides when it is an AI entity's turn: its
  timer fires every `speed` ticks (as now), and `game_on_timer()` calls
  the AI's turn handler, `ai_on_turn()`, for an entity on the current
  level (`level_holds()`) with the AI component.
- **R9** On its turn an AI entity does at most one action that uses the
  turn. A refused action costs nothing (the handler may try another); not
  acting is waiting. The timer starts its next period whatever it did.
- **R10** `TIMER_MONSTER` is renamed `TIMER_AI`: the timer kind says who
  decides the turns, and an AI entity needn't be a monster (a charmed
  ally later).

### This step's states

- **R11 (Changed)** Two states: `AI_STATE_ASLEEP`, which does nothing on
  its turn and wakes on `AI_STIM_HURT`, and `AI_STATE_IDLE`, awake with
  nothing to do, which waits (today's behaviour). Reason: a state machine
  with one state can't change state, so the transition and its event, the
  point of the framework, couldn't be built or tested. Idle is a state the
  game keeps: it later wanders or looks for the player.
- **R12** A creature kind's record names its start state: rats and
  goblins start asleep.
- **R13** The messages tell the player when a monster they can see wakes
  up ("The goblin wakes up."): `EV_AI_CHANGED` from asleep.
- **R14** A monster killed by the attack that would have woken it doesn't
  wake up: nothing is said.

### Module and memory

- **R15** The AI is a module of its own, `src/game/ai/`, with a page pair
  of its own: code (and its constants) in page 130, its data in page 131.
- **R16 (New)** The AI component moves into the module: its header,
  `ai_add()` and `ai_remove()` go from `game/components/ai.{h,c}` (page
  110) to `game/ai/` (page 130). Reason: `ai_add()` now sets the entity's
  state and brain, which are the module's data in page 131, mapped only
  while page 130's code runs; and page 110 has only 119 bytes left.
- **R17** AI entities parked on other levels keep their state, and don't
  think (R8).

### Later

- **R18 (later)** More states: wander, hunt, attack, flee, and others a
  brain needs; a target per entity; perception (noticing the player:
  `AI_STIM_SEEN`, with Wisdom); noise (`EV_OPENED`, combat nearby);
  morale (`AI_STIM_LOW_HP`); a brain's temperament (a flee threshold).
- **R19 (later)** Conditions that stop an entity thinking: unconscious
  (`STATUS_UNCONSCIOUS`, a sleep spell), whatever its state. The magic
  sleep is an effect and a status; asleep is a behaviour. The two are
  different (D11).
- **R20 (later)** Saving the game saves page 131 with the ECS data page.
- **R21 (later)** A dev-kit key that switches every AI entity to the next
  brain (and back), to compare models in CSpect: once there is a second
  brain.
- **R22 (later)** `AI_CAN_RANGED` and `AI_CAN_CAST`, when a ranged attack
  and spells can be used (their bits reserved now).

## 5. Decisions

- **D1 A state machine (Q1).** As in section 2. **Rejected:** behaviour
  trees, utility AI and goal planning as the whole design; utility
  scoring is kept in mind for choices inside a state.

- **D2 Shared states; brains as data, one to start (Q2).** One handler
  per state, in page 130; one brain record per way of choosing, constant
  data:

  ```c
  typedef struct ai_brain_def {
      ai_state_t next[AI_STATE_COUNT][AI_STIM_COUNT];  /* AI_STATE_*, or AI_STATE_SAME */
  } ai_brain_def_t;
  ```

  One brain in this step, `AI_BRAIN_DEFAULT`, for the rat and the
  goblin: the brain number is in the kind record from the start (one
  byte), so a second brain is data, not a change to the generator. A
  later brain may add temperament fields to the record (R18).
  **Rejected:** a state machine per class (the code repeated per class);
  one machine with the differences in code (`if` in every state); no
  brain at all, one table for everything (every difference between
  creatures would then go into the states as code); function pointers per
  brain (a pointer into a banked page is fragile, N-BANK-6, and `switch`
  on a number is the project's way, G-ARCH-7); two brains now (animal,
  humanoid) with the same table: nothing would tell them apart until a
  state that uses the difference exists.

- **D3 Capabilities are worked out each turn from the entity (R5a,
  R5b).** `ai_can(id)` returns this turn's mask:

  | Bit | When | Mirrors |
  |---|---|---|
  | `AI_CAN_MELEE` | it has a melee attack, or a weapon with one in its weapon slot (`attack_weapon()`) | `act_melee()` |
  | `AI_CAN_PICKUP` | it has an inventory | `act_pickup()` |
  | `AI_CAN_EQUIP` | it has an inventory with an equipment slot | `act_equip()` |
  | `AI_CAN_USE` | it has an inventory and Int `AI_INT_USE` (5) or more | `act_use()`, plus the AI's rule |
  | `AI_CAN_OPEN` | it has stats and Int `AI_INT_OPEN` (6) or more | `act_open()`, plus the AI's rule |

  The scores come from the current ability values (`stats_ability[]`,
  slot 0), so effects count. The rat (Int 2, no inventory) can melee; the
  goblin (Int 10, an inventory with a weapon slot) can do all five. Each
  bit is a few byte tests in slot 0, and `attack_weapon()` one banked
  call, once a turn. The mask says what the AI would consider; the
  action's own check still decides (a door may be locked). **Rejected:**
  capabilities in the brain (the first draft): an intelligence drain
  wouldn't change them, and every brain would repeat what the components
  already say. **Rejected:** a mask computed when the entity is
  generated: it goes stale when equipment, components or abilities
  change, and the rules would live twice. **Rejected:** keeping it up to
  date on events (`EV_ATTRS_CHANGED`, `EV_EQUIPPED`...): more code and
  100 bytes, to save a few tests a turn.

- **D4 The AI's data is page data, in page 131**: per entity, its state
  (`ai_state[]`) and brain (`ai_brain[]`), 200 bytes at 100 entities.
  Only the AI module reads them, from its own banked code, which is
  "used by one system" in docs/memory_map.md. Others ask through
  `ai_get_state()` (banked). **Option:** slot 0 (page 50 has 1.6K left):
  readable from common code without a banked call, and saved with the
  ECS page, but nothing outside the AI needs it each frame. **Rejected:**
  slot 0, to keep its last 1.6K for hot shared data.

- **D5 The events go in through a filter in common memory.**
  `ai_on_event()` (common, in `ai.c`) is called by `game_on_event()` for
  every event; it maps the event to a stimulus and only for an AI entity
  that is alive, and only then calls the banked `ai_stimulus()`. So the
  events that don't concern the AI cost no banked call. The same pattern
  as `player_on_event()` and its banked part. Checking alive gives R14:
  an attack that kills posts `EV_DAMAGED` and `EV_DIED` after the
  creature is already dying.

- **D6 State changes from an event happen at once; from a turn, the new
  state acts next turn.** An event changes the state when handled, so
  the next turn is the new state's. A state's handler that returns a
  stimulus has already used (or not) its turn: the new state acts next
  time. Waking up therefore costs the creature its next turn, not this
  one: it slept through this turn, then wakes. **Rejected:** running the
  new state's handler at once (two thinks in one turn, and a loop to
  guard against).

- **D7 `EV_AI_CHANGED`, one event for every change of state (R7a).**
  The messages switch on old and new state (asleep to anything: "wakes
  up"). **Rejected:** an event per transition (`EV_WOKE`, `EV_FLED`...):
  an event ID each, and code in `ai` for each.

- **D8 `TIMER_AI` (R10)**, and the handler is skipped for an entity
  without the AI component (it was removed: later, a creature the player
  controls by a charm). The comment example in
  [nge_timer.h](../src/nge/ecs/systems/nge_timer.h) changes to
  `TIMER_AI` too. **Rejected:** keeping `TIMER_MONSTER` (as for D6 in
  ranged_ai_components.md).

- **D9 The kind record names the AI: an `ai_kind_t` part**, as the other
  components' kind parts:

  ```c
  typedef struct ai_kind {
      ai_brain_t brain;   /* AI_BRAIN_*; AI_BRAIN_NONE (0) for no AI */
      ai_state_t state;   /* AI_STATE_* it starts in */
  } ai_kind_t;
  ```

  The generator adds the AI component when the brain isn't
  `AI_BRAIN_NONE` (`AI_KIND_HAS()`), instead of to every creature but the
  player, and gives a creature with AI the `TIMER_AI` timer. The player's
  record has none; `test_gen` checks every other creature has one.
  **Rejected:** keeping "every creature but the player" with the brain
  chosen elsewhere: the kind record is where kind data goes, and a statue
  later is a creature with no brain.

- **D10 The module and its pages (R15, R16).** `src/game/ai/`:

  | File | Contents | Where |
  |---|---|---|
  | `ai.h` | The types; the component (`AI_IS()`, `ai_kind_t`, `ai_add()`, `ai_remove()`) and the module (`ai_on_turn()`, `ai_on_event()`, `ai_get_state()`, `ai_can()`); states, stimuli, brains, capabilities | |
  | `ai.c` | `ai_on_event()`: the filter (D5) | common |
  | `ai_bank.c` | `ai_add()`, `ai_remove()`, `ai_on_turn()`, `ai_stimulus()`, `ai_get_state()`, `ai_can()`, the states' handlers, `ai_try()` | page 130 |
  | `ai_brains.c` | The brain table, `const` | page 130 (constseg) |
  | `ai_pdata.c` | `ai_state[]`, `ai_brain[]` | page 131 |
  | `ai_internal.h` | Shared by the module's files | |

  `game/components/ai.{h,c}` are removed, and `components.h` points to
  `game/ai/ai.h` for `COMPONENT_AI`, as it does to effects.h for
  `COMPONENT_EFFECT`. 130/131 is the **last game code pair** (102-131):
  the next module needs the range moved or extended (Q2).

- **D11 Asleep is a state, not the sleep effect.** A monster asleep
  because the level found it so is the AI's concern and wakes when hurt.
  A sleep spell (later) is an effect granting `STATUS_UNCONSCIOUS`, which
  stops any state thinking while it lasts (R19). **Rejected:** asleep as
  an effect with a timer: an entity and a timer per sleeping monster, for
  what one byte says.

- **D12 `ai_try()`, the framework's way to act (R5, R9):**

  ```c
  /* Do action (ACT_*) for a->actor if a check passes: 1 if it used the turn */
  uint8_t ai_try(uint8_t action, act_t *a);
  ```

  It checks with `ACT_CHECK`, does it with `ACT_DO` if the check passed,
  and returns `a->used`. The states call actions only through it. Banked
  calls from page 130 to the actions (page 112) map page 112/113 over
  slots 6 and 7 for the call: the context is on the stack, and nothing in
  page 131 is passed by pointer (the generator's rule, gen_internal.h).
  No state uses it in this step (asleep and idle don't act); it is built
  and tested now so the first acting state starts from it.

- **D13 Types for the AI's numbers (R5c):** `typedef uint8_t
  ai_state_t;`, `ai_stim_t`, `ai_brain_t`, `ai_can_t`, as nge's
  `nge_entity_id_t` and `nge_block_t`. The brain's record is therefore
  `ai_brain_def_t`, not `ai_brain_t`: the type without a suffix is the
  number, as for entities. They document; C still lets one be passed for
  another.

## 6. Design

### 6.1 ai.h

```c
typedef uint8_t ai_state_t;   /**< AI_STATE_*. */
typedef uint8_t ai_stim_t;    /**< AI_STIM_*. */
typedef uint8_t ai_brain_t;   /**< AI_BRAIN_*. */
typedef uint8_t ai_can_t;     /**< A mask of AI_CAN_*. */

/* States */
#define AI_STATE_ASLEEP  0   /**< Does nothing; wakes when hurt. */
#define AI_STATE_IDLE    1   /**< Awake with nothing to do: waits (later: wanders, looks). */
#define AI_STATE_COUNT   2
#define AI_STATE_SAME    0xFF  /**< In a brain's table: no change. */

/* Stimuli */
#define AI_STIM_HURT     0   /**< Attacked: hit, missed or damaged. */
#define AI_STIM_COUNT    1
#define AI_STIM_NONE     0xFF  /**< From a state's handler: nothing new. */

/* Brains */
#define AI_BRAIN_NONE     0   /**< In a kind record: no AI. */
#define AI_BRAIN_DEFAULT  1
#define AI_BRAIN_COUNT    2

/* Capabilities: bits of ai_can() (D3) */
#define AI_CAN_MELEE   0x01   /**< Attacks in melee: its own attack or a weapon's. */
#define AI_CAN_PICKUP  0x02   /**< Picks up items: an inventory. */
#define AI_CAN_EQUIP   0x04   /**< Wields and wears them: an inventory with a slot. */
#define AI_CAN_USE     0x08   /**< Uses them (drinks a potion): an inventory, Int AI_INT_USE. */
#define AI_CAN_OPEN    0x10   /**< Opens and closes doors: Int AI_INT_OPEN. */
#define AI_CAN_RANGED  0x20   /**< Reserved: a ranged attack (later). */
#define AI_CAN_CAST    0x40   /**< Reserved: casts spells, with mana (later). */

/* The least Intelligence for the AI's own rules */
#define AI_INT_USE   5
#define AI_INT_OPEN  6

#define AI_IS(id)          NGE_ENTITY_HAS_COMPONENT(id, COMPONENT_AI)
#define AI_KIND_HAS(k)     ((k)->brain != AI_BRAIN_NONE)

void       ai_add(nge_entity_id_t id, const ai_kind_t *k) NGE_BANKED;   /* k in common memory */
void       ai_remove(nge_entity_id_t id) NGE_BANKED;
void       ai_on_turn(nge_entity_id_t id) NGE_BANKED;
void       ai_on_event(const nge_event_t *e);                           /* common */
ai_state_t ai_get_state(nge_entity_id_t id) NGE_BANKED;
ai_can_t   ai_can(nge_entity_id_t id) NGE_BANKED;
```

`ai_add()` asserts the entity is in use, the brain is in range and not
`AI_BRAIN_NONE`, and the state in range. `ai_can()` asserts the entity
is alive; an entity without stats has Int 0 for the rules (it can't open
or use).

### 6.2 A turn

```c
void ai_on_turn(nge_entity_id_t id) NGE_BANKED
{
    ai_stim_t stim;

    switch (ai_state[id]) {
    case AI_STATE_ASLEEP: stim = AI_STIM_NONE; break;   /* sleeps */
    case AI_STATE_IDLE:   stim = AI_STIM_NONE; break;   /* waits */
    default: nge_util_abort("bad ai state");
    }
    if (stim != AI_STIM_NONE) {
        change(id, stim, NGE_EVENT_NONE);
    }
}
```

Each state with more than a line gets its own static function,
`think_<state>(id, can)`, returning a stimulus; `ai_on_turn()` calls
`ai_can()` once, before the `switch`, when the first state needs it
(asleep and idle don't). `change()` looks up the brain's table and, if
the state changes, sets it and posts `EV_AI_CHANGED`. `ai_stimulus()` is
`change()` for an event.

In [game.c](../src/game/game.c):

```c
case TIMER_AI:
    if (level_holds(id) && AI_IS(id)) {
        ai_on_turn(id);
    }
    break;
```

and `ai_on_event(e)` in `game_on_event()`, after `quest_on_event()` and
before `messages_on_event()`. Its `EV_AI_CHANGED` is handled later in
the same `nge_event_process()` call, so the order only matters for the
event that caused it.

### 6.3 Events

`ai_on_event()`, in common memory:

```c
void ai_on_event(const nge_event_t *e)
{
    nge_entity_id_t target = e->target;   /* a local: see messages_on_event() */

    switch (e->id) {
    case EV_HIT:
    case EV_MISSED:
    case EV_DAMAGED:
        if (target != NGE_EVENT_NONE && NGE_ENTITY_IS_ALIVE(target) && AI_IS(target)) {
            ai_stimulus(target, AI_STIM_HURT, e->source);
        }
        break;
    }
}
```

A hit gives `EV_HIT` then `EV_DAMAGED`: the first wakes it, the second
finds it idle, where hurt is `AI_STATE_SAME`. One `EV_AI_CHANGED`.

`EV_AI_CHANGED` joins [events.h](../src/game/events.h):

| Event | Target | Source | Payloads |
|---|---|---|---|
| `EV_AI_CHANGED` | the AI entity | the cause, or NGE_EVENT_NONE | p1: old state, p2: new state |

### 6.4 The brains (`ai_brains.c`)

```c
const ai_brain_def_t ai_brains[AI_BRAIN_COUNT] = {
    [AI_BRAIN_NONE] = { .next = { ...every entry AI_STATE_SAME } },   /* never used: ai_add() refuses it */
    [AI_BRAIN_DEFAULT] = {
        .next = {
            [AI_STATE_ASLEEP] = { [AI_STIM_HURT] = AI_STATE_IDLE },
            [AI_STATE_IDLE]   = { [AI_STIM_HURT] = AI_STATE_SAME },
        },
    },
};
```

A missing entry is 0, which is `AI_STATE_ASLEEP`, not "no change": a
test checks every entry of every brain is set (section 8), and the
tables are written out in full.

### 6.5 The generator and the kinds

`gen_creature_kind_t` gets `ai_kind_t ai;`. In `create()`
(gen_creature_bank.c):

```c
if (AI_KIND_HAS(&b.ai)) {
    ai_add(id, &b.ai);   /* the game decides its turns */
}
...
nge_timer_add(id, (b.flags & CREATURE_PLAYER) ? TIMER_PLAYER : TIMER_AI, stats_speed[id], 0);
```

with an assert that a creature is either the player or has a brain.
Kinds: the rat and the goblin `{ AI_BRAIN_DEFAULT, AI_STATE_ASLEEP }`,
the player none. The dev kit's persistent goblin on depth 2 is a goblin,
so asleep. Nothing about capabilities goes in the kind record: they
follow from the rest of it (D3).

### 6.6 Messages

`messages_on_event()` gets `EV_AI_CHANGED` with p1 `AI_STATE_ASLEEP`:
"The <name> wakes up.", worded and shown as its other messages about
monsters are.

### 6.7 Memory and banks

- [banks.inc](../src/banks.inc): `#pragma define CRT_ORG_PAGE_130 =
  0x82C000`, and 130/131 in the page list; docs/memory_map.md the same.
- Page 130: the module's code and the brains, about 1K to start.
- Page 131: 200 bytes.
- Page 110: `ai_add()` and `ai_remove()` leave it (a few dozen bytes back).
- Common memory: `ai_on_event()`, a few dozen bytes; `game_on_timer()`
  a few bytes more.
- Slot 0: nothing.

## 7. Scope

- **In:** the module and its page pair (R15, R16, D10); the types (R5c);
  the state machine, stimuli and the default brain (R1 to R4);
  `ai_can()` (R5a, R5b); `ai_try()` (D12); the event filter and
  `EV_AI_CHANGED` (R6, R7, R7a); the turn handler from
  `game_on_timer()`, `TIMER_AI` (R8 to R10); asleep and idle (R11, R12);
  the wake-up message (R13); the generator and kind records (D9);
  updates to `game/README.md`, docs/memory_map.md, timers.h's table,
  components.h, nge_timer.h's example; tests.
- **Later:** R18 to R22: every state that moves or fights, targets,
  perception, noise, morale, temperament, unconsciousness, saving, the
  dev-kit brain switch, ranged and casting capabilities.

## 8. Tests

PC tests:

- **`test_ai`** (now linked like `test_act`, for `ai_can()`, `ai_try()`
  and the events):
  - `ai_add()`: the component, the state and brain set, read back with
    `ai_get_state()`; asserts: a free entity, `AI_BRAIN_NONE`, a brain or
    state out of range. `ai_remove()`.
  - The brains: every entry of every brain's table is a state or
    `AI_STATE_SAME`.
  - `ai_can()`: nothing for an entity with no components; melee from its
    own attack, and from a weapon equipped (and gone when the weapon is
    taken off); pick up with an inventory; equip only with a slot; use and
    open at Int 5 and 6, not at 4 and 5; open lost when Int is lowered
    below 6 (the ability's value, as an effect would change it); the rat
    and the goblin as generated (melee; all five).
  - Events: `EV_HIT` on an asleep AI entity makes it idle and posts one
    `EV_AI_CHANGED` (target, source, p1 asleep, p2 idle); the following
    `EV_DAMAGED` posts nothing; `EV_MISSED` wakes it too; an event on a
    dying AI entity, on an entity without AI, and with target none,
    changes nothing and posts nothing; other events are ignored.
  - Turns: `ai_on_turn()` on asleep and idle entities moves nothing,
    changes no state, posts no event.
  - `ai_try()`: a move into a free tile succeeds and uses the turn; into
    a wall it is refused with no events and the entity unmoved.
- **`test_gen`:** the rat and the goblin have the default brain, both
  asleep; the player has no AI; every creature but the player has a
  brain; timer kinds `TIMER_AI` and `TIMER_PLAYER`.
- **`test_timer`**, unchanged (nge); the rename touches only the game.

CSpect checks:

1. A new game: the monsters sit still, as before; the player moves and
   fights as before.
2. Attack a goblin and miss: "The goblin wakes up." Attack it again:
   no second message.
3. Hit a rat without killing it: one "wakes up" message, after the hit's
   messages.
4. Kill a monster with one blow (with the dev kit's sword): no "wakes up".
5. The dev kit's persistent goblin on depth 2 (the same entity on every
   visit): wake it, go up to depth 1 and back down, and attack it again:
   no second "wakes up". Other monsters are new on each visit (levels are
   generated again), so a depth 1 goblin wakes again after a return.

`ai_can()` has nothing to show in CSpect until a state acts on it: the
PC tests cover it.

## 9. Risks

- **The last game code pair** (D10): the next banked game module has no
  pair in 102-131. Q2.
- **Banked calls from page 130** to the actions, the components and nge
  map other pages over slot 7: code must not hold pointers into page 131
  across them (D12; `ai_can()` calling `attack_weapon()` too). The tests
  on the PC can't catch it (one flat memory), so the rule goes in
  `ai_internal.h` and the CSpect checks exercise it once acting states
  exist.
- **zsdcc miscompiles** (memory notes): reading `ai_brains[b].next[s][t]`
  and struct fields of a copied record; the mask built from several
  tests in `ai_can()` (constant bits are below `0x80`, so no signed-char
  shift); read the asm of `change()`, `ai_add()` and `ai_can()`.
- **Cost per turn:** one banked call per AI entity turn, plus one per
  event that concerns an AI entity, and later `ai_can()`'s tests and one
  banked call: no more than an effect's timer today.

## 10. Questions

All answered (2026-10-29):

- **Q1 (D3):** Int 5 to use items and Int 6 to open doors: agreed.
- **Q2 (D10):** the AI takes 130/131, the last pair in 102-131; the game
  code range is moved or extended when the next module needs a pair.
- **Q3 (R11):** idle is the second state in this step: agreed.
- **Q4 (R12):** every monster starts asleep for now; a chance per kind
  may come later.

## 11. As built: differences from the design

- **"None" values are 0, members of their groups** (H-CONST-5), not
  `0xFF`: `AI_STATE_NONE` 0 (in a brain's table: no change; never an
  entity's state, `ai_add()` asserts), so `AI_STATE_ASLEEP` is 1 and
  `AI_STATE_IDLE` 2, and `AI_STATE_SAME` is gone; `AI_STIM_NONE` 0, so
  `AI_STIM_HURT` is 1. A brain's table has a row for `AI_STATE_NONE` and
  a column for `AI_STIM_NONE`, all no change (6 bytes a brain). An entry
  left out of a table is therefore "no change", not asleep: the risk in
  6.4 is gone, though the tables are still written out in full. The
  tables and `change()` name it `AI_STATE_NO_CHANGE`, defined as
  `AI_STATE_NONE`, so an entry reads as "stay" rather than as a state.
- **`ai_try()` is in a file of its own, `ai_act.c`** (page 130 too), so
  the tests linked with the generators (test_gen, test_inventory...) need
  the AI's component code but not the actions. `test_ai` links with the
  actions (as `test_act`).
- **`ai_on_turn()` doesn't pass a stimulus on yet:** neither state
  returns one, and zsdcc warned that the code doing it was unreachable.
  Its comment says how the first acting state's handler,
  `think_<state>(id, can)`, feeds `ai_stimulus()`.
- **Memory:** page 130 $456 bytes (about 1.1K); page 131 $C8 (200);
  page 110 down from $1F89 to $1EB2, 215 bytes back. **Common memory grew
  by 135 bytes, from 244 to 109 bytes free below the stack**: the event
  filter `ai_on_event()` (about 80) and the `TIMER_AI` case and the call
  in `game.c`. The next common-memory addition will need room found
  first.
- **The wake-up message is said for any monster** on the level, not
  only one the player can see: there is no field of view yet, and the
  other messages about monsters ("The rat dies.") do the same.
- **components.h** now says that a component whose tables are its
  module's page data has its functions with that module (the AI), and
  lists the AI's tables beside the effects'.
- **The asm was read** for `ai_on_event()`, `ai_stimulus()` and
  `ai_add()`: the fields are copied into registers before the branches
  and calls, the brain table's stride is 6, and `EV_AI_CHANGED`'s
  arguments are in order.
- **Tests:** `test_ai` 53 checks (also: a creature with Int 6 and no
  inventory opens but uses nothing; an inventory with no slots picks up
  but doesn't equip; a carried, unwielded sword gives no melee; damage
  with no source wakes; a move refused by a creature names it);
  `test_gen` 56, with the goblin's and the rat's `ai_can()`.
