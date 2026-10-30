# game

Game code: the basis of a classic roguelike.

| File | Contents |
|---|---|
| `game.{h,c}` | The main loop (`docs/game_loop.md`): the title, a game, then why it ended, one after another. The frames every loop runs (`game_frame_start()`, `game_frame_end()`) and waiting for a key (`game_wait_key()`). The play loop: the turn engine (time passing until the player's turn, then their command, each waiting while an animation plays in view), and the world's step (the frame's events, changing level when asked, `level_request()`, the clean-up). `game_over()` with its reason (died, quit, won), `game_play_redraw()` for a new game or level, the paused frames of overlay screens (`game_pause_frame()`), and the event and timer dispatch functions, which pass each event to every system's `on_event()` and each timer to its handler (skipping effects and AI entities off the current level) |
| `ui/` | The screens (`docs/ui_screens.md`), prefix `screen_`: the framework (`screen.h`: overlay screens drawn on Layer 2 buffer 1 over everything, the game paused, and the help line on row 31 under them all), the main screen of play (`screen_main.h`: the map view, the stats and the messages on buffer 0, and the overlay sprites), the inventory (`screen_inv.h`: browse, or choose an item for drop, equip, take off or use; a chooser, returning the action and the item, which the command does) the title (`screen_title.h`: the game's name, then the name prompt) and the player character (`screen_pc.h`: the character, attributes against their bases, defences, statuses and active effects; `docs/pc_screen.md`). Banked: `screen.c` and `screen_main.c` (common: the help line, the messages, the overlay sprites), `screen_pc_bank.c` in page 104, the rest in page 116, `screen_pdata.c` (page 117: the inventory's list) |
| `player/` | The player: what their keys do (`player_command()`: move or bump to attack or open, pick up, inventory, drop, equip, take off, use, fire, close, look, `<` and `>` stairs, the character screen (`C`), quit), `player_act()` for actions on their behalf, the direction prompt (a chooser: fire, close) and why a command was refused, their turn (`player_on_turn()` sets `player_turn`: no time passes until a command uses it), their stats on events (`player_on_event()`), and the XP for what they kill (`player_on_pc_event()`). Banked: `player.c` (common: the handlers), `player_bank.c` (page 114: commands and screens) |
| `anim/` | Action animations (`docs/animations.md`): `anim_on_event()` starts a slide after a move and a lunge after a melee attack, with the kind's sequence for the action if it has one (copied from the creature table by `anim_init()`); `anim_holding()` tells the turn engine to wait while one plays in view. Banked: `anim.c` (common: `anim_holding()`), `anim_bank.c` (page 126), `anim_pdata.c` (page 127: the sequences) |
| `events.h` | The game's event IDs (things that have happened) |
| `dev.h` | Development aids, on in debug builds (`GAME_DEV_KIT`): a starting kit (one of each item) and resistances, X to hurt the player, F to fall to the next level, a persistent goblin on depth 2, each depth's level the same every run (`level_dev_seed`), V to pan the view a pixel at a time, M to spend 1 of the player's MP (all back at 0), and E for a level and 10 gold |
| `timers.h` | The game's timer kinds: whose handler runs when an entity's timer fires (an actor's every `speed` ticks, the player's or the AI's; an active effect's every period) |
| `messages/` | The message window's reactions to events and to the player's refused commands. Banked in page 118 |
| `level/` | The current level and going between levels (`docs/level_transitions.md`): depths 1 to `LEVEL_DEPTH_BOTTOM`, each with its own area, the current one's map in slot 1; the level generator (rooms in a 4x3 grid joined by corridors; up and down stairs in different rooms; doors in about a third of the doorways; random items and monsters from the generators; the depth's parked entities; the player, on the stairs they came by); leaving a level (kept entities, the persistent ones and what they own, parked in its area, the rest released); requests to change level, carried out by the game at the end of the frame; entities sent to other levels; and the per-frame clean-up (`level_release_dying()`). Banked: `level.c` (common), `level_bank.c`, `level_gen.c` and `level_travel.c` (page 102), `level_pdata.c` (page 103: the room list, the stairs) |
| `components/` | The game's components: their numbers in one list and the shape each follows (`components.h`), their tables in slot 0 (`*_cdata.c`): `kind` (which kind an entity was generated from), `stats` (speed and the six abilities, Str to Cha, 1-30, with `STATS_MOD()`), `health` (hit points, armour class, damage types it is immune to, resists or is vulnerable to; damage, healing, death), `attack` (melee, later ranged: to hit, damage dice, bonus and type, range; the hit rule), `item` (can be carried; its equipment slot), `inventory` (can hold items: capacity, equipment slots; moving items between the floor and holders), `feature` (a fixed, interactive part of a level, doors and stairs so far: its state; placing, opening and closing it keep its blocking and its map tile in step), `mana` (mana points, 0 to a maximum from its kind record: spending, which the caller checks first, and restoring; `docs/mana.md`), `pc` (the player character, a single component with one variable per field: name, class placeholder, level, XP, gold, deepest depth; XP for kills and gold, new levels with their hit points; `PC_IS()` tells the player; `docs/player_character.md`); `attrs.h`: the attributes' numbers and how they work (kind value, base, derived value), their tables in health and stats; the statuses in stats; functions banked in page 110 |
| `effects/` | Effects (`effects.h`): definitions (`EF_*`) bundling modifiers, statuses (conditions among them) and ops (damage, heal, cure, raise a base), instant, timed (an active effect: an entity owned by its target, with a timer, and maybe a saving throw that ends it early) or worn (while an item is equipped); `effect_apply()`, `effect_end()`, `effect_cure()`, and `effects_recompute()`, which derives the attributes from the bases, equipment and active effects. Split into the mechanism, `effects_core.c`, and the game's content, `effects_game.c`, which the core reaches through a seam (`effects_core.h`). Banked: both in page 120, `effects_pdata.c` (page 121: the definitions) |
| `actions/` | Actions: what entities do, with one API (`act_t` context, `ACT_DO` or `ACT_CHECK`, success or failure with a reason, whether the turn was used): `act_move()`, `act_pickup()`, `act_drop()`, `act_equip()`, `act_unequip()`, `act_melee()`, `act_use()`, `act_open()`, `act_close()`, `act_ascend()`, `act_descend()`, `act_wait()`; `act_do()` calls one by number. The pattern is described in `act.h`. Banked in page 112 |
| `gen/` | Entity generators: `gen_creature()`, `gen_item()` (and their `_random()` versions) and `gen_feature()` create an entity with all its components from its kind record, a `const` table in each class's odd page (`gen_<class>_pdata.c`, kinds in `gen_kinds.h`); `gen_name()` copies a kind's name; `gen_hook_of()` gives the effect and power a kind names for a hook (used, a hit, worn, innate). Pages 106/107 (creatures), 108/109 (items), 122/123 (features), 110 (shared helpers) |
| `quest/` | The quest item (`docs/winning.md`): `quest_item`, the one item of kind `IT_QUEST`, made with the persistent entities and kept on one depth at all times; `quest_carried()` (the depth 1 up stairs let only the player out, and only with it), `EV_QUEST_FOUND` on the first pick-up, and `quest_check()` after each change of level in debug builds. Banked: `quest.c` (common: `quest_item`), `quest_bank.c` (page 128) |
| `ai/` | The AI (`docs/ai.md`, `docs/ai_default_brain.md`): the game decides the turns of an entity with the AI component, every monster. A state machine: each turn (`ai_on_turn()`, from `TIMER_AI`) it senses (hears the player while asleep, else looks: `ai_sense()`), then its state (asleep, idle, hunt) tries its options in order until one acts, else it waits: every turn is one action. Its brain, named by its kind's record, is a transition table from state and stimulus (hurt, from `ai_on_event()`; heard, seen, lost) to the next state, each change posting `EV_AI_CHANGED`; `ai_can()` works out what it can do from its components and abilities (a rat can't open doors, a goblin can); it acts through the actions (`ai_try()`). The AI component lives here (`AI_IS()`, `ai_add()`). Banked: `ai.c` (common: the event filter), `ai_bank.c`, `ai_sense.c`, `ai_think.c`, `ai_act.c` and `ai_brains.c` (page 130), `ai_pdata.c` (page 131: each entity's state, brain, target and goal) |
| `actors.{h,c}` | The player and the other persistent entities, from a table of {class, kind, depth}, made once a game (generated, placed by the level generator) |

## Architecture

The game is built on nge's ECS (entities, components, systems), with two
additions that suit a turn-based game: **actions** for what an actor does, and
**events** for what follows from it. Each kind of logic has one home:

| Layer | What it is | Where | Examples |
|---|---|---|---|
| Entities | IDs, nothing else | nge (`ecs/nge_entity.h`) | the player, a rat, a sword, an active effect |
| Components | Data, in tables by entity ID in slot 0, and small functions that keep it valid and hold the rules about it | `components/`, nge `ecs/components/` | `health_damage()`, `attack_hits()`, `inv_stow()` |
| Actions | One actor's intent, at one moment: context, validate, resolve, apply (`actions/act.h`). They choose and check, then call the components' rules | `actions/` | `act_move()`, `act_melee()`, `act_pickup()` |
| Events | Facts about what happened, posted after the change; handlers react to them in a fixed order (`game_on_event()` in `game.c`) | `events.h`, each module's `on_event()` | `EV_DAMAGED` → a message; `EV_DIED` on the player → game over |
| Effects | Data-defined consequences: modifiers, statuses and ops, instant, timed or worn, applied by actions and generators through `effect_apply()` | `effects/` | poison, a healing potion, a ring of health |
| Systems | Batch work: every entity with some components, every frame or as time passes | nge `ecs/systems/`, `game.c` | `nge_render` (sprites), `nge_timer` (turns, effects), `level_release_dying()` (clean-up) |
| Generators | Entities made from data: kind records by kind | `gen/` | `gen_creature()`, `gen_item()` |

Rules for new features:

1. **A rule lives in one place.** Put a game rule (how damage is taken, whether
   an attack hits) in its component's functions, and call it from every action
   and effect that needs it: a sword, a trap and a spell all damage through
   `health_damage()`.
2. **Actions orchestrate; they don't own rules.** An action finds and checks
   its target, then calls the components. Anything an actor chooses to do,
   the player or a monster, is an action, and costs a turn when it is done.
3. **Reactions go in event handlers, not in actions.** An action says what
   happened (`EV_DIED`); the game, the messages and the screen each decide what
   that means to them. A new consequence is a new handler, not a change to the
   action.
4. **Systems only for batch work.** Use a system when the question is "for every
   entity with these components, every frame or tick": drawing, timers,
   clean-up, and later field of view. Don't scan every entity to find the one
   that wants to act: that is an action, started by its timer or by a key.
5. **Effects over time are entities with timers** (poison, regeneration,
   sleep): the timer system drives them, owned by what they affect, so they
   go when it does. Add an effect as data (`effects/effects_pdata.c`) and
   name it from a kind's hook; only a new kind of op needs code.
6. **Derived data is kept right by the code that changes it, not by events.**
   An attribute's value is its base plus the modifiers of equipment and
   active effects, worked out from scratch by `effects_recompute()`, which
   whatever changes them calls directly (equipping, an effect starting or
   ending). Events can be dropped (a full queue) and arrive late (the item
   already used up), so they carry reactions only: messages, the screen.

Effects are game code built on nge's primitives (entities, ownership,
timers, events), not part of nge: most of what they do is this game's rules.
Their mechanism (applying, ending and curing effects, active effects'
lifecycle, recomputing derived values) is kept apart from the content (what
the attributes mean, the game's ops, equipment): `effects_core.c` includes
nothing of the game's components and reaches them only through the seam in
`effects_core.h` (`make test` checks it). If another game or system needs
effects, the core moves into nge and the seam becomes its callbacks.
7. **Behaviour is chosen by number, never by function pointer** (timer kinds,
   action IDs, event IDs): data stays editable and banked code stays callable.

Code here uses the nge library (`src/nge`); nge never includes anything from
here. New `.c` files are built automatically (see the Makefile). Includes are
written from `src`: `#include "game/level/level.h"`.

## Banked game modules

A module with code that runs rarely is banked: it moves into its own folder,
`src/game/<module>/`, with `<module>.c` (common: per-frame and per-turn code
and data), `<module>_bank.c` (its even page: rarely run code with its
literals) and, if needed, `<module>_pdata.c` (its odd page: large private
data). Its page pair comes from the game code range in
`src/banks.inc` (102-131). See "Banked game modules" in
`docs/memory_map.md` for the rules. Modules not yet banked stay single files
here.
