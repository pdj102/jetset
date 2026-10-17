# game

Game code: the basis of a classic roguelike.

| File | Contents |
|---|---|
| `game.{h,c}` | The main loop (pre-update, the current state's update, post-update), the game states (title, new game, play, the item screens, targeting, dead; entering play redraws the map, sprites, stats and help), time passing, and the event and timer dispatch functions, which pass each event to every system's `on_event()` and each timer to its handler |
| `ui/` | Screens shown in the map window: the item screens (inventory, drop, equip, take off), each a row of a table of descriptions (title, help bar text, which items, what choosing one does). Banked in page 116 |
| `player/` | The player: what their keys do (`player_command()`: move, pick up, inventory, drop, equip, take off, fire, quit), `player_act()` for actions on their behalf, the firing screen and why a command was refused, their turn (`player_on_turn()` stops the clock) and the camera following them (`player_on_event()`). Banked: `player.c` (common: the handlers), `player_bank.c` (page 114: commands and screens) |
| `events.h` | The game's event IDs (things that have happened) |
| `dev.h` | Development aids, on in debug builds (`GAME_DEV_KIT`): a starting kit and resistances, and X to hurt the player |
| `timers.h` | The game's timer kinds: whose handler runs when an entity's timer fires (an actor's every `speed` ticks) |
| `messages/` | The message window's reactions to events and to the player's refused commands. Banked in page 118 |
| `title/` | The title state: the title message, the name prompt, then play. Banked: `title_bank.c` (page 104), no common part |
| `screen.{h,c}` | Screen layout: the map view, the stats window and the message window |
| `level/` | The current level: one area, its map in slot 1, and the level generator (rooms in a 4x3 grid joined by corridors; random items and monsters from the generators; the player). Banked: `level.c` (common), `level_bank.c` and `level_gen.c` (page 102), `level_pdata.c` (page 103: the room list) |
| `components/` | The game's components, their tables in slot 0 (`*_cdata.c`): `kind` (which base entity an entity was generated from), `stats` (speed), `health` (hit points, damage types it is immune to, resists or is vulnerable to; damage, healing, death), `item` (can be carried; its equipment slot), `inventory` (can hold items: capacity, equipment slots; moving items between the floor and holders); functions banked in page 110 |
| `actions/` | Actions: what entities do, with one API (`act_t` context, `ACT_DO` or `ACT_CHECK`, success or failure with a reason, whether the turn was used): `act_move()`, `act_pickup()`, `act_drop()`, `act_equip()`, `act_unequip()`; `act_do()` calls one by number. The pattern is described in `act.h`. Banked in page 112 |
| `gen/` | Entity generators: `gen_creature()`, `gen_item()` and their `_random()` versions create an entity with all its components from its base record, a `const` table in each class's odd page (`gen_<class>_pdata.c`, kinds in `gen_kinds.h`); `gen_name()` copies a kind's name. Pages 106/107 (creatures), 108/109 (items), 110 (shared helpers) |
| `actors.{h,c}` | The player (generated, placed by the level generator) and a demo effect on the player |

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
