# game

Game code: the basis of a classic roguelike.

| File | Contents |
|---|---|
| `game.{h,c}` | The main loop (pre-update, the current state's update, post-update), the game states (title, play, inventory, targeting, dead) and the event dispatch function, which passes each event to every system's `on_event()` |
| `events.h` | The game's event IDs (things that have happened) |
| `timers.h` | The game's timer kinds (whose handler runs when an entity's timer fires) and action costs in ticks |
| `messages.{h,c}` | The message window's reactions to events |
| `title/` | The title state: the title message, the name prompt, then play. Banked: `title_bank.c` (page 104), no common part |
| `screen.{h,c}` | Screen layout: the map view, the stats window and the message window |
| `level/` | The current level: one area, its map in slot 1, and the level generator (rooms in a 4x3 grid joined by corridors, items, the player). Banked: `level.c` (common), `level_bank.c` and `level_gen.c` (page 102), `level_pdata.c` (page 103: the room list) |
| `items.h`, `items_cdata.c` | The item component: each item's kind (placeholder potion and sword), its table in slot 0 |
| `actors.{h,c}` | The player (placed by the level generator) and a demo effect on the player |

Code here uses the nge library (`src/nge`); nge never includes anything from
here. New `.c` files are built automatically (see the Makefile). Includes are
written from `src`: `#include "game/level/level.h"`.

## Banked game modules

A module with code that runs rarely is banked: it moves into its own folder,
`src/game/<module>/`, with `<module>.c` (common: per-frame and per-turn code
and data), `<module>_bank.c` (its even page: rarely run code with its
literals) and, if needed, `<module>_pdata.c` (its odd page: large private
data). Its page pair comes from the game code range in
`src/nge/banks.inc` (102-131). See "Banked game modules" in
`docs/memory_map.md` for the rules. Modules not yet banked stay single files
here.
