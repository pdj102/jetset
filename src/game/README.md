# game

Game code: the basis of a classic roguelike.

| File | Contents |
|---|---|
| `screen.{h,c}` | Screen layout: the map view, the stats window and the message window |
| `level.{h,c}` | The current level: one area, its map in slot 1, a test layout |
| `actors.{h,c}` | The level's entities: the player and a test object |

Code here uses the nge library (`src/nge`); nge never includes anything from
here. New `.c` files are built automatically (see the Makefile). Includes are
written from `src`: `#include "game/level.h"`.
