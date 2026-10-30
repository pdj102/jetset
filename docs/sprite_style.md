# Sprite style guide

Status: agreed (2026-10-29): D1 to D9 as proposed, and Q1 to Q5 answered
as proposed. No masters are drawn yet (6.9).

The rules every sprite in `assets/Sprites.aseprite` follows, so sprites
drawn at different times, by the artist or by Claude, look like one set.
How the art gets into the game, and the palette's slot layout, are
docs/sprite_import.md; this guide says how to draw with them.

A request for art names this guide: "draw `skeleton.idle` and
`skeleton.attack` in bank 14, following docs/sprite_style.md".

## 1. Goal

- One look across the player, monsters, items, features and effects:
  the same view, size, outline, light and shading.
- Sprites read at a glance on the dungeon's light grey floors and dark
  grey walls, at 16 x 16, on a TV.
- Sprites recolour cleanly through the banks (sprite_import.md D4), and
  show as clean silhouettes in the flash and shadow banks.
- Few patterns per thing: 128 patterns hold the whole game.

## 2. Terms

| Term | Meaning |
|---|---|
| Cell | One 16 x 16 square of the map, the size of a sprite |
| Baseline | The row a standing sprite's feet touch |
| Body box | The rectangle a sprite's opaque pixels fill, outline included |
| Ramp | The shades of one material, light to dark: primary 9-12, secondary 13-15, greys 2-3-4, browns 7-8 |
| Master | An agreed sprite that new sprites are drawn to match (section 6.9) |
| Grid | A sprite written as 16 rows of 16 characters, one a pixel: `.` transparent, `1`-`9` and `a`-`f` the slot |

## 3. Requirements

- **R1** Every sprite follows the view, size, outline, light, shading and
  animation rules of section 6. A sprite that breaks one says why in its
  layer name or in section 8.
- **R2** Sprites match the tileset (`assets/Tilesheet.aseprite`): the
  same 3/4 view, and the same light from the top left as the bricks'
  highlights.
- **R3** A sprite uses the slots for their roles (sprite_import.md 5.2):
  it recolours by bank, and its outline, metal, wood and gold stay the
  same in every colourway.
- **R4** A sprite reads on both the light floor and the dark wall, and
  in the flash (1) and shadow (2) banks as a silhouette.
- **R5** Patterns are budgeted (6.8): a thing's animations fit its share.
- **R6** New art is checked against the masters (6.9) before it is
  committed: side by side, in Aseprite, at 1x and zoomed.
- **R7** Claude draws to this guide by the workflow of section 7: grids
  and a preview for review, then the file, then `make`.

## 4. Decisions

- **D1 A 3/4 view, facing right.** The tileset is 3/4 (walls show their
  face, dungeon_decoration.md), so creatures show their front and a
  little of their top, turned to the right. The game can face them left
  with `NGE_SPRITE_MIRROR_X`, with no extra patterns. **Rejected:** front
  view only (simplest, but a creature can't face what it attacks, and a
  lunge sideways looks odd); four directions (four times the patterns:
  the 128 wouldn't hold the bestiary). Whether the game turns sprites at
  all is Q1.
- **D2 Feet on row 14, columns 1-14.** A standing sprite's lowest pixels
  are on row 14 whatever its size, so creatures of all sizes stand on the
  same line in a cell; small ones are shorter, not floating. Row 15 and
  columns 0 and 15 stay empty, so neighbouring sprites never touch and a
  lunge's few pixels don't merge outlines. Items and effects are
  centred instead (6.2). **Rejected:** filling the whole 16 x 16: sprites
  in adjacent cells would join into one shape.
- **D3 A full outline in slot 1, outside only.** The floors are light grey
  and the walls dark grey: a near-black outline separates a sprite from
  both, and slot 1 is the same in every colourway, so recolouring never
  touches it. In the flash and shadow banks the outline is part of the
  silhouette, as it should be. Inside the shape, parts are separated by
  shade (slot 12 or 15 against a lighter one), and slot 1 only where two
  parts of the same colour overlap (an arm across the body).
  **Rejected:** selective outlines (the outline takes a dark shade of the
  part it borders): they read better on dark backgrounds, but on the light
  floor the lit edges disappear, and every edge pixel would depend on the
  bank.
- **D4 Light from the top left.** As the tileset's bricks. Lit edges are
  up and left, shade down and right, and on a 3/4 creature the tops of
  the head and shoulders catch the light.
- **D5 Hard pixels: no dithering, no anti-aliasing.** At 16 x 16 a dither
  pattern reads as noise, and anti-aliasing against an outline that is
  always there does nothing but blur. Each material takes 2 or 3 shades;
  only the primary uses 4 (the highlight 9 is a few pixels of glint, not
  a band). **Rejected:** pillow shading (dark all round the edge, light in
  the middle): it flattens the light from D4.
- **D6 Chunky proportions.** A humanoid's head is about 5 of its 13 or 14
  rows. Faces, hands and weapons are what a player reads at this size,
  so they are bigger than real proportions; legs are short. The style is
  close to the classic 16 x 16 roguelike look, not realistic.
  **Rejected:** realistic proportions: the head and face shrink to 3
  pixels and the creature loses its character.
- **D7 Little movement per frame.** A rest animation changes few pixels
  (a 1-pixel bob, a blink, a flicker), keeping the feet still, so it adds
  life without distraction. The slide and the lunge (animations.md) move
  the sprite; the frames only add the pose. **Rejected:** big idle loops:
  a screen of them is busy, and costs patterns.
- **D8 The standard set: rest (2), attack (1 or 2), dead (1).** Hits are
  the flash bank and walking is the slide, so neither needs patterns. A
  monster is 4 or 5 patterns; recolours (goblin, orc) share them (6.8).
  Move frames only for the player, if any (Q4).
- **D9 Masters, once agreed.** Rules alone don't fix a look; a few sprites
  everyone agrees on do. Today's sprites are placeholders and aren't
  masters. The first job under this guide is a master for each class
  (6.9): the player, one monster, one item.

## 5. Questions answered

All answered as proposed (2026-10-29): creatures turn (mirror X), items
don't; the player is a medium humanoid about 10 x 13, a generic
adventurer in bank 0; rest 2, attack 2, dead 1 and no move frames; no
ground shadow.

- **Q1 Facing.** Does the game turn sprites to face where they move and
  attack (mirror X, D1)? Proposed: yes, for creatures; items don't turn.
  If not, D1's "facing right" becomes "facing the viewer, turned slightly
  right", which still looks right without mirroring.
- **Q2 The player's size.** Proposed: a medium humanoid, body box about
  10 x 13. Larger (12 x 14) gives a more detailed hero but makes them as
  big as an ogre.
- **Q3 The player's look.** A class isn't chosen yet (player_character.md
  R5). Proposed: a generic adventurer: a blue tunic (bank 0 primary),
  skin and hair from bank 0's secondary, a short sword in the shared
  greys and gold. Hair colour is slot 8 (dark brown) unless a different
  one is wanted.
- **Q4 Player animations.** Proposed: rest 2, attack 2, dead 1, and no
  move frames (the slide is short, 4 frames). A 2-frame step at 2 frames
  each (rate 2) is possible later.
- **Q5 Ground shadow.** A 1-row slot-1 shadow under the feet, on row 15,
  grounds a sprite on the light floor, but breaks D2's empty row and
  shows as part of the silhouette in the flash bank. Proposed: no.

## 6. The rules

### 6.1 Canvas

- 16 x 16, indexed, the sprite palette (sprite_import.md 5.1). One palette
  row a frame.
- Columns 0 and 15 and row 15 are empty, except in attack frames
  (section 8).
- No stray pixels: every opaque pixel is joined to the shape, unless it is
  an effect's particle.

### 6.2 Size classes

| Class | Body box (w x h, with outline) | Placement | Examples |
|---|---|---|---|
| Tiny | up to 8 x 7 | feet on row 14, centred | bat (in the air: bottom on row 11), spider, slime |
| Small | up to 12 x 9 | feet on row 14 | rat, dog, snake |
| Medium | 9-12 x 12-14 | feet on row 14 | player, goblin, skeleton, zombie |
| Large | 13-14 x 14 | feet on row 14, top on row 1 | ogre, troll, golem |
| Item | up to 12 x 12 | centred on (7.5, 7.5), bottom no lower than row 13 | potion, sword, ring, scroll, coins |
| Feature | up to 14 x 14 | on row 14, as a creature | chest, torch, fountain |
| Effect | anything inside 14 x 14 | centred | sparks, fire, frost, poison cloud |
| UI | the whole 16 x 16 | as needed | target cursor |

A creature's height says how strong it looks: keep weak monsters small.

### 6.3 Outline

- Slot 1, 1 pixel, all round the outside, including the feet.
- Corners: diagonal steps are one pixel; no doubled pixels making a
  corner thick (an "L" of three outline pixels where two would join).
- Inside: shade separates parts. Slot 1 inside only where two parts of
  the same ramp overlap, and never closing a part off completely.
- Items and effects are outlined too. A glow or a flame's tip may leave
  the outline off where it should look bright (6.6).
- The UI cursor isn't outlined.

### 6.4 Light and shading

- Light from the top left (D4).
- For each material, from its ramp:
  - **Base**: the mid shade (11, 14, 3 for steel, 7 for wood) on most of
    the part.
  - **Lit**: the light shade (10, 13, 2, 6...) along the top and left
    edges, 1 pixel wide, wider on big surfaces facing up.
  - **Shade**: the dark shade (12, 15, 4, 8) along the bottom and right
    edges, and under overhangs (under the head, under the arm, the far
    leg).
  - **Highlight** (9, or 2 on metal and glass): 1 to 3 pixels where a
    shiny surface faces the light: a helmet's top left, a blade, a potion's
    glass. Not on cloth or fur.
- The far leg and arm (the right ones, facing right) are a shade darker
  than the near ones.
- No dithering, no anti-aliasing, no pillow shading (D5). No two shades
  in parallel stripes the same length ("banding").

### 6.5 Faces and details

- Eyes: one pixel. Slot 2 (white) on dark faces and for undead, slot 1
  (a dark pupil) on light skin, slot 5 (red) for monsters. Facing right,
  a humanoid shows one or both eyes in the right half of the head.
- Mouths and noses only on medium and large faces, as one shade pixel.
- Hair, hats and helmets cover the top of the head and show in the
  silhouette: they make creatures recognisable.
- A weapon is held in the hand on the right, the side the sprite faces,
  and points up or forward, so a lunge to the right reads as a strike.
  Leave a transparent column between it and the body, so the two outlines
  don't merge into a thick line.
- 1-pixel details that nothing depends on (a buckle) are fine; details
  that identify the sprite are 2 pixels or more.

### 6.6 Colour use

The slot roles are sprite_import.md 5.2. In short:

| Part | Slots |
|---|---|
| Outline | 1 |
| A creature's body (fur, cloth, scales, slime) | primary 10-11-12, 9 only if shiny |
| Belly, markings, skin, a second material | secondary 13-14-15 |
| Steel, glass, bone, teeth, eye whites | 2-3-4 |
| Wood, leather, hair | 7-8 |
| Gold, buckles, flame tips | 6 |
| Blood, eyes, gems | 5 |

- Draw a creature in its default bank (sprite_import.md 5.2: a rat in
  12, a goblin in 6), so the preview shows its usual colours.
- Draw a recolourable thing (a potion's liquid, a gem, a monster family)
  in the primary ramp, so each bank gives a colourway.
- Glows and fire leave out the outline on the bright side and use 6 and
  2 at their hottest.
- Check every sprite in bank 1 (flash) and bank 2 (shadow): its outline
  should be a clear, recognisable silhouette.

### 6.7 Animation

| Animation | Tag | Frames | Duration | Notes |
|---|---|---|---|---|
| Rest | `<name>.idle` | 2 (1 for still things) | 500 ms (rate 25) | A 1-pixel bob or breath: the body moves, the feet don't. Frame 2 can shift the head and body down a row |
| Attack | `<name>.attack` | 1 or 2 | 80 ms each, 160 ms at most (the lunge) | The strike pose: weapon or jaws forward to the right. The lunge moves it; the frames are the pose at its peak |
| Move | `<name>.move` | 0 or 2 | 40 ms each | Only if wanted (Q4): the slide is 80 ms |
| Dead | `<name>.dead` | 1 | any | A corpse lying on row 14: the body on its side, at most 8 rows high |
| Effect | `<name>` | 2-4 | 40-100 ms | A burst, then gone; or a loop for a lasting effect |

- Durations within a tag are all the same (sprite_import.md R8), and play
  forward (R9).
- Animation frames are drawn from the tag's first frame, changing as few
  pixels as possible: the outline stays continuous and the feet stay
  still.
- Tags are lower case, `<thing>.<action>`: `rat.idle`, `skeleton.attack`.
  Items and effects are one word: `potion`, `spark`.

### 6.8 Pattern budget

128 patterns in all. Proposed shares:

| Use | Patterns |
|---|---|
| Player | 6 (rest 2, attack 2, dead 1, spare 1) |
| Monsters | 50 (10 families of up to 5; a family's recolours are free) |
| Items | 40 (one each, recolours free) |
| Features | 12 (chest open and shut, torch 2, fountain 2...) |
| Effects | 14 (sparks, fire, frost, poison, magic) |
| UI | 6 (cursor, markers) |

### 6.9 Masters

None yet (D9). Once agreed, each is listed here by tag and frame, and new
art is compared with them:

| Class | Master |
|---|---|
| Medium creature | `player.idle` frame 0 (agreed 2026-10-29) |
| Small creature | `rat.idle` frame 0 (agreed 2026-10-29) |
| Item | `potion` (agreed 2026-10-29) |

## 7. Workflow with Claude

1. **The request** names the sprite, its tags, its bank, and this guide;
   it can name a master to match ("as the goblin, but bigger and in
   iron").
2. **Grids.** Claude writes each frame as a grid (section 2) in the reply,
   with notes on which rules decide each choice. Changing a grid is cheap:
   "make the skull 1 pixel wider".
3. **Preview.** Claude renders the frames at 8x, next to the masters, on
   the floor and wall colours, and in the flash and shadow banks.
4. **Into the file.** Once the artist agrees, Claude adds the frames and
   tags to `assets/Sprites.aseprite` with Aseprite's command line
   (`Aseprite.exe -b --script`, a Lua script), which keeps the layers,
   palette and the other frames as they are. New frames go at the end
   unless asked otherwise; existing frames change only when asked.
5. **Check.** `make` (the importer checks the hardware rules,
   sprite_import.md R1-R14), then the artist opens the file in Aseprite
   and runs the game in CSpect.
6. **Commit** the `.aseprite` file and the generated files together
   (sprite_import.md section 9).

## 8. Exceptions

- **Attack frames may reach column 15** in the direction of the lunge
  (agreed 2026-10-29). A medium creature's body leaves 3 or 4 columns for
  a weapon held forward, and a 2-pixel blade doesn't read as a strike. An
  attack frame shows only during a lunge, when the sprite is meant to
  reach into its target's cell, so touching the neighbour there is
  intended. Column 0 and row 15 stay empty. First used by
  `player.attack` frame 1.
