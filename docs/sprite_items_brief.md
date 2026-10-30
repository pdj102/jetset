# Item sprites: working brief

Temporary: deleted when the 36 items in items.md 6.5 are drawn. All a batch
session needs: don't read sprite_style.md or items.md. Sprites are in
`assets/Sprites.aseprite`; the tool is `scripts/sprite_grids.py`.

## One batch

1. Write all the batch's grids in one Write to a file in the scratchpad.
2. `python3 -I scripts/sprite_grids.py check <file>`. Fix only what it names.
3. `... preview <file> <png>`. View it once, fix what is plainly wrong, once.
4. Stop and give the artist the PNG path. All their fixes come in one message.
5. Edit, `check`, `preview`. On their OK: `apply <file>`, `make`, `make test`,
   and add what they corrected to the review log below. Commit only when asked.

Header line: `dagger 13 6 7` (tag, drawn bank, other banks), adding `glow` if
the outline may be left off. Then 16 rows: `.` transparent, `1`-`9`, `a`-`f`.
`dump <tag>...` prints any existing sprite the same way.

Preview columns: 8x on floor, wall, flash, shadow; 4x in each other bank; 1x.

## The rules for an item

- 16 x 16, up to 12 x 12 with outline, centred on (7.5, 7.5), nothing on rows
  14-15 or columns 0 and 15. Items don't turn or animate: one frame.
- 3/4 view, matching the masters: chunky, read at a glance at 1x.
- Slot 1 outline, 1 pixel, all round, including inner holes. Diagonal steps are
  one pixel, with no thick "L" corners. Slot 1 inside only where two parts of
  one ramp overlap, and never closing a part off.
- Light from the top left. Base is the mid shade; lit is 1 pixel along top and
  left; shade along bottom and right and under overhangs; highlight is 1 to 3
  pixels, only on shiny surfaces (metal, glass), not cloth.
- No dithering, anti-aliasing or pillow shading. Each material 2 or 3 shades;
  only the primary uses 4. No two shades in parallel stripes of equal length.
- A detail that identifies the sprite is 2 pixels or more.
- Glows and fire leave out the outline on the bright side, using 6 and 2 at the
  hottest (mark the sprite `glow`).
- A bank changes only slots 9-15. If a sprite has several banks, whatever should
  change colour must be in the primary ramp (`P` below).
- Weak things small; a dagger is plainly smaller than a sword.

## Slots

| Slot | Use |
|---|---|
| 1 | Outline |
| 2, 3, 4 | White, light grey, dark grey: steel, glass, bone, paper |
| 5 | Red: blood, gems |
| 6 | Gold: hilts, buckles, flame tips |
| 7, 8 | Brown, dark brown: wood, leather |
| 9, 10, 11, 12 | Primary: highlight, light, mid, dark |
| 13, 14, 15 (d, e, f) | Secondary: light, mid, dark. Differs by bank; iron's is rust, so prefer the primary |

Banks: RED 3, ORANGE 4, GOLD 5, GREEN 6, LIME 7, BLUE 8, ICE 9, PURPLE 10,
EARTH 12, IRON 13, BONE 14, DARK 15.

## Masters

`potion` is the agreed master; `ring` shows a primary-ramp stone on fixed gold.
Note the size and the outline.

```
potion 3          ring 6
................  ................
................  ................
......1111......  ................
.....177781.....  ......1111......
......1881......  .....19aab1.....
......1341......  .....1abbc1.....
....11134111....  ....161cc161....
...1323333341...  ...1266116671...
...1aaaaaaab1...  ...1611..1171...
...12bbbbbbc1...  ...161....171...
...129bbbbbc1...  ...161....171...
...1bbbbbbcc1...  ...1711111181...
....1cccccc1....  ....17777881....
.....111111.....  .....111111.....
................  ................
................  ................
```

## Batches

Banks: drawn bank first. Tick each batch off when applied.

| Done | # | Sprites | Colour plan |
|---|---|---|---|
| x | 1 Blades (6) | sword (redraw, tag exists) 13 8 12 5 6 4 10; dagger 13 6 7; greatsword 13 10; spear 13 8 12; arrow 12 6 13; dart 13 | Blade or head `P`, hilt 6-7-8. Spear also stands for halberd and javelin, arrow for bolts: keep them generic |
| x | 2 Hafted (5) | axe 13 8 6 9; mace 13 8 12 6; staff 12 10 3; wand 12 14 5 9 4 10; torch 12 | Head `P`, haft 7-8. Mace also stands for war hammer and club. Torch: flame 6-5-2, `glow` |
| x | 3 Ranged, small tools (6) | bow 12 6 10; crossbow 13 12; sling 12; shot 13 8; key 13 14 15 5 4; lock.pick 13 5 | Limbs or metal `P`, string 3 |
| x | 4 Body (6) | armour.soft 12 8 6; armour.mail 13 12 6 10; armour.plate 13 5; cloak 12 4 10; shield 13 8 12; helmet 13 12 | All `P`. Soft, mail and plate must differ by surface as well as outline |
| x | 5 Limbs (4) | vambraces 13 12; gauntlets 13 12 4; greaves 13 12; boots 12 9 13 | All `P`. The riskiest batch: paired forearm, hand, tall shin and L-shaped foot must read apart at 1x |
| x | 6 Food, containers (5) | bread 12 5; meat 3 12; fruit 3; flask 12 5 4; lantern 12 9 | Potion's proportions. Flask `P`; lantern frame 7-8, glass `P` |
| | 7 Treasure, magic (5) | parchment 14 8 15 12 5 6 9 7 4 10; amulet 13 15 7 10 3; coins 5; gem 9 14 3; crown 10 | Parchment: paper 2-3-4, ribbon or seal `P`. Amulet: chain 6, stone `P`. Crown: gold 6, jewels `P` |

## Review log

What the artist corrected, batch by batch. Apply it to every later batch.

- Batch 1: no corrections.
- Batch 2: no corrections.
- Batch 3: key teeth longer. A detail that identifies a sprite wants to be bold.
- Batch 4: no corrections.
- Batch 5: no corrections.
- Batch 6: no corrections.
