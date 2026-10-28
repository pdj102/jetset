# Known bugs

Bugs seen in the game and not yet fixed. Each has a number that commits
and tests can name (B1...). When one is fixed, its status says so, with
the commit, and it stays listed so the number keeps its meaning.

| Field | Meaning |
|---|---|
| Seen | Date, and where: CSpect or hardware, build (branch and commit) |
| Steps | How it happened, as far as known |
| Seen / expected | What happened, and what should have |
| Status | open, investigating, fixed (commit), or can't reproduce |
| Notes | Causes ruled in or out, and what to try next |

## B1 A rat on a wall tile that the look command doesn't see

- **Seen:** 2026-10-21, CSpect, dev branch at 2f00b20 with step 7 (the
  animations) uncommitted.
- **Steps:** the player fell to a new level (the F development key, not
  the stairs), so the level was generated on arrival. A dead rat was on a
  wall tile next to an open doorway.
- **Seen:** the rat's sprite on the wall tile. Looking at that tile (x)
  said "You see nothing there."
- **Expected:** no entity on a wall tile. Anything drawn on a tile is
  something the look command reports.
- **Status:** open, not investigated.
- **Notes:** the sprite showing while look finds nothing suggests the
  drawn entity and the tile's entities disagree. Two things to check: is
  the drawn rat an entity on this level at all (render system and look
  both go by the location), or one left from the level above? And can
  the generator place a creature on a doorway tile or a wall? It may not
  come from the fall in particular: try the stairs as well.

## B2 An empty name keeps the previous game's name

- **Seen:** 2026-10-28, CSpect, dev branch at 8474fa9.
- **Steps:** play a game, typing a name at the title. When it ends, go back
  to the title and press ENTER at the name prompt without typing a name.
- **Seen:** the new game's player has the name from the previous game.
- **Expected:** the default name, "Rogue" (docs/player_character.md: "Rogue"
  if none).
- **Status:** open, cause found, not fixed.
- **Notes:** the code does this on purpose, against the spec. `game.c` keeps
  the name in a static buffer, `name[]` ("kept for the next game"), set to
  "Rogue" only once at start-up. `ask_name()` in `ui/screen_title_bank.c`
  copies the typed name into it only if it isn't empty ("An empty name
  keeps the current one"). To fix: an empty name should set "Rogue",
  either in `ask_name()` or by resetting the buffer before each title.
  Update both comments.
