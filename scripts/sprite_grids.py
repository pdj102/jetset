#!/usr/bin/env python3
"""
Sprites as text grids (docs/sprite_style.md section 7). From the repository
root:

    python3 -I scripts/sprite_grids.py dump potion sword
    python3 -I scripts/sprite_grids.py check grids.txt
    python3 -I scripts/sprite_grids.py preview grids.txt sheet.png
    python3 -I scripts/sprite_grids.py apply grids.txt

A grid file holds frames. A header line names the tag, the bank it is drawn
in, any other banks it shows in, and "glow" if it may leave its outline off
(6.6). A creature adds its size class (tiny, small, medium, large) and "air"
if it flies. 16 rows follow: . transparent, 1-9 and a-f the slot. Blank lines
and lines starting # are skipped. Grids in a row with one tag are its frames:

    dagger 13 6 7
    ................
    ...
    goblin.idle 6 13 10 medium
    ................
    ...

The .aseprite file stays the source of the art: apply writes the sprites into
it with Aseprite's command line, appending new tags and replacing the frame
or frames of a tag that has them. Add --file to work on another file. Set
ASEPRITE to use another Aseprite. check knows the Item and creature rules
(6.2).
"""
import argparse
import itertools
import os
import re
import struct
import subprocess
import sys
import tempfile
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import aseprite   # noqa: E402

SIZE = 16
SPRITE_FILE = 'assets/Sprites.aseprite'
SPRITE_LAYER = 'Sprites'
ASEPRITE = '/mnt/d/SteamLibrary/steamapps/common/Aseprite/Aseprite.exe'
ITEM_DURATION = 0.1           # seconds, the frames' duration in the file
CREATURE_DURATION = 0.5       # a rest animation, rate 25 (6.7)

ITEM_BOX = 12                 # the largest body box, each way
ITEM_CENTRE = 7.5
CENTRE_SLACK = 1              # how far a body box's centre may be from ITEM_CENTRE
ITEM_LAST_ROW = 13
CREATURE_BOX = {'tiny': (8, 7), 'small': (12, 9), 'medium': (12, 14), 'large': (14, 14)}   # the largest, w x h
FEET_ROW = 14
AIR_ROW = 11                  # the bottom of a flying creature, give or take its bob
AIR_SLACK = 1
NEAR_SILHOUETTES = 10         # pixels: fewer differing looks like the same shape

BANK_FLASH = 1
BANK_SHADOW = 2
FLOOR = (182, 182, 182)       # the preview's backgrounds: the map's light floor
WALL = (73, 73, 73)           # and dark wall
PAGE = (40, 40, 48)
BIG = 8                       # the preview's scales
SMALL = 4
GAP = 4


class GridError(Exception):
    pass


class Grid:
    def __init__(self, tag, bank, others=(), glow=False, rows=(), size=None, air=False):
        self.tag = tag
        self.bank = bank
        self.others = list(others)
        self.glow = glow
        self.rows = list(rows)        # strings of . and hex digits
        self.size = size              # a creature's class in CREATURE_BOX, None for an item
        self.air = air


def parse(text):
    """The sprites of a grid file, in order."""
    grids = []
    lines = [line.strip() for line in text.splitlines()]
    lines = [line for line in lines if line and not line.startswith('#')]
    for i in range(0, len(lines), SIZE + 1):
        parts = lines[i].split()
        words = [p for p in parts[1:] if not p.isdigit()]
        numbers = [p for p in parts[1:] if p.isdigit()]
        sizes = [w for w in words if w in CREATURE_BOX]
        if (not numbers or not all(int(n) < SIZE for n in numbers) or len(sizes) > 1
                or set(words) - set(sizes) - {'glow', 'air'}):
            raise GridError(f'"{lines[i]}" is not a header: tag, bank, other banks, glow, size, air')
        rows = lines[i + 1:i + 1 + SIZE]
        if len(rows) != SIZE:
            raise GridError(f'{parts[0]}: {len(rows)} rows, not {SIZE}')
        for y, row in enumerate(rows):
            if not re.fullmatch(r'[.1-9a-f]{%d}' % SIZE, row):
                raise GridError(f'{parts[0]} row {y}: "{row}" is not {SIZE} of . 1-9 a-f')
        grids.append(Grid(parts[0], int(numbers[0]), [int(n) for n in numbers[1:]], 'glow' in words, rows,
                          sizes[0] if sizes else None, 'air' in words))
    return grids


def format_grid(grid):
    header = [grid.tag, str(grid.bank)] + [str(b) for b in grid.others]
    words = ['glow'] * grid.glow + [grid.size] * bool(grid.size) + ['air'] * grid.air
    return '\n'.join([' '.join(header + words)] + grid.rows) + '\n'


def frame_grid(sprite, frame, tag):
    """Frame of an indexed sprite as a Grid: its bank is its pixels' palette row."""
    indices = sprite.frame_indices(frame)
    banks = {v >> 4 for row in indices for v in row if v}
    rows = [''.join('.' if not v else '%x' % (v & 15) for v in row) for row in indices]
    return Grid(tag, min(banks) if banks else 0, rows=rows)


def dump(sprite, tags):
    """The Grids of the tags named. A tag of several frames gives tag#0, tag#1..."""
    found = {tag.name: tag for tag in sprite.tags}
    grids = []
    for name in tags:
        if name not in found:
            raise GridError(f'no tag "{name}"')
        first, last = found[name].first, found[name].last
        for frame in range(first, last + 1):
            grids.append(frame_grid(sprite, frame, name if first == last else f'{name}#{frame - first}'))
    return grids


def _opaque(grid):
    return {(x, y) for y, row in enumerate(grid.rows) for x, c in enumerate(row) if c != '.'}


def _neighbours(x, y, reach):
    for dy in range(-1, 2):
        for dx in range(-1, 2):
            if (dx or dy) and abs(dx) + abs(dy) <= reach:
                yield x + dx, y + dy


def _joined(opaque):
    """Whether the pixels are one shape, corners touching."""
    todo = [min(opaque)]
    seen = {todo[0]}
    while todo:
        x, y = todo.pop()
        for p in _neighbours(x, y, 2):
            if p in opaque and p not in seen:
                seen.add(p)
                todo.append(p)
    return seen == opaque


def check_item(grid):
    """What breaks the Item rules (sprite_style.md 6.1-6.3, D16), as messages."""
    opaque = _opaque(grid)
    if not opaque:
        return ['is empty']
    problems = []
    xs = [x for x, _ in opaque]
    ys = [y for _, y in opaque]
    left, right, top, bottom = min(xs), max(xs), min(ys), max(ys)
    width, height = right - left + 1, bottom - top + 1
    if width > ITEM_BOX or height > ITEM_BOX:
        problems.append(f'body box {width} x {height}, the most is {ITEM_BOX} x {ITEM_BOX}')
    if left == 0 or right == SIZE - 1:
        problems.append('reaches column 0 or 15')
    if bottom > ITEM_LAST_ROW:
        problems.append(f'reaches row {bottom}, the last is {ITEM_LAST_ROW}')
    for name, low, high in (('columns', left, right), ('rows', top, bottom)):
        if abs((low + high) / 2 - ITEM_CENTRE) > CENTRE_SLACK:
            problems.append(f'{name} {low}-{high} are off centre')
    return problems + _shape_problems(grid, opaque)


def _shape_problems(grid, opaque):
    """What breaks the rules items and creatures share (6.1, 6.3, 6.6)."""
    problems = []
    if not _joined(opaque):
        problems.append('has pixels not joined to the shape')
    if not grid.glow:
        for x, y in sorted(opaque, key=lambda p: (p[1], p[0])):
            if grid.rows[y][x] != '1' and any(p not in opaque for p in _neighbours(x, y, 1)):
                problems.append(f'row {y} column {x}: an edge pixel that is not outline')
    if grid.others and not any(c in '9abcdef' for row in grid.rows for c in row):
        problems.append('shows in several banks but has no slot 9-15 to recolour')
    return problems


def check_creature(grid):
    """What breaks the creature rules (sprite_style.md D2, 6.1-6.3), as messages."""
    opaque = _opaque(grid)
    if not opaque:
        return ['is empty']
    problems = []
    xs = [x for x, _ in opaque]
    ys = [y for _, y in opaque]
    left, right, bottom = min(xs), max(xs), max(ys)
    width, height = right - left + 1, bottom - min(ys) + 1
    most_wide, most_high = CREATURE_BOX[grid.size]
    if width > most_wide or height > most_high:
        problems.append(f'body box {width} x {height}, the most for {grid.size} is {most_wide} x {most_high}')
    if left == 0 or right == SIZE - 1:
        problems.append('reaches column 0 or 15')
    if grid.air:
        if abs(bottom - AIR_ROW) > AIR_SLACK:
            problems.append(f'in the air its bottom is on row {bottom}, not about {AIR_ROW}')
    elif bottom != FEET_ROW:
        problems.append(f'its feet are on row {bottom}, not {FEET_ROW}')
    return problems + _shape_problems(grid, opaque)


def near_silhouettes(grids, masters=()):
    """Pairs that look alike: every pair of tags, and each grid against the masters."""
    pairs = []
    for i, a in enumerate(grids):
        others = [g for g in grids[i + 1:] if g.tag != a.tag] + [m for m in masters if m.tag != a.tag]
        for b in others:
            different = len(_opaque(a) ^ _opaque(b))
            if different < NEAR_SILHOUETTES:
                pairs.append(f'{a.tag} and {b.tag}: silhouettes differ by only {different} pixels')
    return pairs


def _png(width, height, pixels):
    """A PNG of rows of (r, g, b)."""
    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
    raw = b''.join(b'\0' + bytes(v for colour in row for v in colour) for row in pixels)
    return (b'\x89PNG\r\n\x1a\n'
            + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw, 9))
            + chunk(b'IEND', b''))


def _paste(page, x, y, grid, bank, palette, scale, background):
    """Draws grid at (x, y) on page, in bank, over a square of background."""
    for gy in range(SIZE):
        for gx in range(SIZE):
            c = grid.rows[gy][gx]
            colour = background if c == '.' else palette[bank * SIZE + int(c, SIZE)][:3]
            for py in range(y + gy * scale, y + (gy + 1) * scale):
                page[py][x + gx * scale:x + (gx + 1) * scale] = [colour] * scale


def preview(grids, palette):
    """
    A contact sheet as PNG bytes, one row a sprite: at 8x on the floor, on the
    wall, in the flash and shadow banks, then at 4x in its other banks, then
    at 1x on both backgrounds.
    """
    big, small = SIZE * BIG + GAP, SIZE * SMALL + GAP
    most = max(len(g.others) for g in grids)
    width = GAP + 4 * big + most * small + SIZE + GAP
    height = GAP + len(grids) * big
    page = [[PAGE] * width for _ in range(height)]
    for row, grid in enumerate(grids):
        y = GAP + row * big
        for col, (bank, background) in enumerate(((grid.bank, FLOOR), (grid.bank, WALL),
                                                  (BANK_FLASH, WALL), (BANK_SHADOW, FLOOR))):
            _paste(page, GAP + col * big, y, grid, bank, palette, BIG, background)
        x = GAP + 4 * big
        for col, bank in enumerate(grid.others):
            _paste(page, x + col * small, y, grid, bank, palette, SMALL, FLOOR)
        x += most * small
        _paste(page, x, y, grid, grid.bank, palette, 1, FLOOR)
        _paste(page, x, y + SIZE, grid, grid.bank, palette, 1, WALL)
    return _png(width, height, page)


def lua_script(grids):
    """The script that puts the grids in the active sprite and saves it."""
    data = []
    for tag, frames in itertools.groupby(grids, lambda g: g.tag):
        frames = list(frames)
        pixels = [','.join(str(0 if c == '.' else g.bank * SIZE + int(c, SIZE)) for row in g.rows for c in row)
                  for g in frames]
        duration = CREATURE_DURATION if frames[0].size else ITEM_DURATION
        data.append(f'  {{ tag = "{tag}", duration = {duration}, frames = {{ '
                    + ', '.join(f'{{ {p} }}' for p in pixels) + ' } },')
    return f"""local spr = app.activeSprite
local layer
for _, l in ipairs(spr.layers) do
  if l.name == "{SPRITE_LAYER}" then layer = l end
end
local tags = {{}}
for _, t in ipairs(spr.tags) do tags[t.name] = t end
local grids = {{
{chr(10).join(data)}
}}
for _, g in ipairs(grids) do
  local t = tags[g.tag]
  local first
  if t then
    assert(t.frames == #g.frames, g.tag .. " has another number of frames")
  end
  for n, pixels in ipairs(g.frames) do
    local img = Image(spr.spec)
    for i, v in ipairs(pixels) do
      img:drawPixel((i - 1) % {SIZE}, (i - 1) // {SIZE}, v)
    end
    if t then
      local number = t.fromFrame.frameNumber + n - 1
      local cel = layer:cel(number)
      if cel then
        cel.image = img
        cel.position = Point(0, 0)
      else
        spr:newCel(layer, number, img, Point(0, 0))
      end
    else
      local ends = {{}}
      for _, t2 in ipairs(spr.tags) do ends[t2] = t2.toFrame.frameNumber end
      local frame = spr:newEmptyFrame()
      for t2, last in pairs(ends) do t2.toFrame = last end
      frame.duration = g.duration
      spr:newCel(layer, frame.frameNumber, img, Point(0, 0))
      first = first or frame.frameNumber
    end
  end
  if not t then
    local tag = spr:newTag(first, first + #g.frames - 1)
    tag.name = g.tag
  end
end
spr:saveAs(spr.filename)
"""


def apply_grids(grids, path):
    """Writes the grids into the .aseprite file at path with Aseprite."""
    def windows(p):
        return subprocess.run(['wslpath', '-w', p], check=True, capture_output=True, text=True).stdout.strip()
    tool = os.environ.get('ASEPRITE', ASEPRITE)
    with tempfile.NamedTemporaryFile('w', suffix='.lua', dir=os.path.dirname(os.path.abspath(path))) as script:
        script.write(lua_script(grids))
        script.flush()
        done = subprocess.run([tool, '-b', windows(os.path.abspath(path)), '--script', windows(script.name)],
                              capture_output=True, text=True)
    if done.returncode or 'rror' in done.stdout + done.stderr:
        raise GridError(f'Aseprite failed: {done.stdout}{done.stderr}')


def main(argv=None):
    parser = argparse.ArgumentParser(description='Sprites as text grids.')
    parser.add_argument('--file', default=SPRITE_FILE, help='the .aseprite file')
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('dump').add_argument('tags', nargs='+')
    commands.add_parser('check').add_argument('grids')
    command = commands.add_parser('preview')
    command.add_argument('grids')
    command.add_argument('png')
    commands.add_parser('apply').add_argument('grids')
    args = parser.parse_args(argv)
    try:
        sprite = aseprite.read(args.file)
        if args.command == 'dump':
            print('\n'.join(format_grid(g) for g in dump(sprite, args.tags)), end='')
            return 0
        with open(args.grids) as f:
            grids = parse(f.read())
        if args.command == 'check':
            problems = [f'{g.tag}: {p}' for g in grids for p in (check_creature(g) if g.size else check_item(g))]
            problems += near_silhouettes(grids, dump(sprite, ['potion', 'sword', 'ring', 'player.idle', 'rat.idle']))
            print('\n'.join(problems or [f'{len(grids)} sprites: clean']))
            return 1 if problems else 0
        if args.command == 'preview':
            for row, grid in enumerate(grids):
                print(f'row {row}: {grid.tag}')
            with open(args.png, 'wb') as f:
                f.write(preview(grids, sprite.palette))
            return 0
        apply_grids(grids, args.file)
        return 0
    except (OSError, aseprite.AsepriteError, GridError) as e:
        print(f'sprite_grids: error: {e}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
