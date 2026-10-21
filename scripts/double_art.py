#!/usr/bin/env python3
"""
Converts the 8 x 8 art to 16 x 16 placeholders (docs/cells_16.md), once.

Run on the 8 x 8 versions of the data files (the commit before the change),
from the repository root:

    git show 9ed3730:src/data/tile_defs_data.asm > /tmp/tiles8.asm
    git show 9ed3730:src/data/sprite_data.asm > /tmp/sprites8.asm
    python3 -I scripts/double_art.py /tmp/tiles8.asm /tmp/sprites8.asm

It writes:
- src/data/tile_defs_data.asm: the map's metatiles, the ASCII glyphs the map
  used, pixel-doubled to 2 x 2 hardware tiles each (metatile m is hardware
  tiles 4m to 4m+3);
- src/data/sprite_data.asm: the 8 x 8 sprites (patterns 2-9) pixel-doubled
  to fill their 16 x 16 patterns;
- src/nge/layer2/nge_layer2_font.c: Layer 2's font as column bitmasks, from
  the tile definitions' ASCII font, which the metatiles replace.

The output is committed and is the source from then on: new art replaces it.
"""
import re
import sys

# Metatile number -> the glyph it doubles (game/level/level_internal.h, level.h)
METATILES = [None, '.', ',', '#', '-', '|', '+', "'", '<', '>']
METATILE_COUNT = 64
METATILE_NAMES = ['rock (transparent)', 'floor', 'floor 2', 'corridor', 'wall, horizontal (and corners)',
                  'wall, vertical', 'door, shut', 'door, open', 'stairs up', 'stairs down']

FONT_FIRST = 32
FONT_COUNT = 96

# Sprite pixel values as letters in the pattern comments (sprite_data.asm's key)
PIXEL_LETTERS = '.WYCRG'


def read_tiles(path):
    """The 256 tile definitions: 64 pixel values (0-15) each, row by row."""
    text = open(path).read()
    tiles = {}
    for n, body in re.findall(r'^sprite(\d+):\n((?:\s*db .*\n)+)', text, re.M):
        data = [int(v, 16) for v in re.findall(r'\$([0-9a-fA-F]+)', body)]
        assert len(data) == 32, n
        tiles[int(n)] = [(data[i // 2] >> (4 if i % 2 == 0 else 0)) & 0x0F for i in range(64)]
    assert len(tiles) == 256
    return tiles


def doubled(pixels8):
    """An 8 x 8 grid of pixels as a 16 x 16 grid."""
    return [pixels8[(y // 2) * 8 + x // 2] for y in range(16) for x in range(16)]


def quarter(pixels16, qx, qy):
    """One 8 x 8 quarter of a 16 x 16 grid."""
    return [pixels16[(qy * 8 + y) * 16 + qx * 8 + x] for y in range(8) for x in range(8)]


def tile_db(pixels8):
    """A tile definition's two db lines: 4 bits a pixel, the left pixel high."""
    data = [(pixels8[i] << 4) | pixels8[i + 1] for i in range(0, 64, 2)]
    return ['\tdb ' + ', '.join('$%x' % b for b in data[half * 16:half * 16 + 16]) for half in range(2)]


def write_tiles(tiles, path):
    lines = [
        '; The map\'s graphics: metatiles of 2 x 2 hardware tiles, 4 bits a pixel',
        '; (map/nge_map.h). Metatile m is hardware tiles 4m to 4m+3: top left, top',
        '; right, bottom left, bottom right. Its number is the map tile ID',
        '; (game/level/level_internal.h, level.h). Placeholders for now: the ASCII',
        '; glyphs the map used, pixel-doubled (scripts/double_art.py). Pixel values',
        '; 0 transparent, 1-3 through the tile\'s palette bank (the ink colours).',
        '',
        'SECTION BANK_5',
        'org 0x4A00',
        '',
        'PUBLIC _tile_pattern',
        '',
        '._tile_pattern',
        '',
        'sprite_count EQU %d' % (METATILE_COUNT * 4),
    ]
    for m in range(METATILE_COUNT):
        glyph = METATILES[m] if m < len(METATILES) else None
        big = doubled(tiles[ord(glyph)]) if glyph is not None else [0] * 256
        name = METATILE_NAMES[m] if m < len(METATILE_NAMES) else 'unused'
        lines.append('')
        lines.append('; metatile %d: %s' % (m, name))
        for q in range(4):
            lines.append('sprite%d:' % (m * 4 + q))
            lines.extend(tile_db(quarter(big, q % 2, q // 2)))
    open(path, 'w').write('\n'.join(lines) + '\n')


def write_font(tiles, path):
    lines = [
        '/*',
        ' * Layer 2\'s font: banked constants in page 90, with the drawing that reads',
        ' * them (nge_layer2_draw.c). Generated from the tilemap\'s 8 x 8 ASCII font',
        ' * by scripts/double_art.py, when the tile definitions became metatiles.',
        ' */',
        '#ifdef __SDCC',
        '#pragma codeseg PAGE_90',
        '#pragma constseg PAGE_90',
        '#endif',
        '',
        '#include "layer2/nge_layer2_internal.h"',
        '',
        '/* Character 32 (space) to 127; one byte a pixel column, left to right, bit 7 the top pixel */',
        'const uint8_t nge_layer2_font[NGE_LAYER2_FONT_COUNT][NGE_LAYER2_CELL] = {',
    ]
    for c in range(FONT_COUNT):
        pixels = tiles[FONT_FIRST + c]
        cols = []
        for x in range(8):
            mask = 0
            for y in range(8):
                if pixels[y * 8 + x]:
                    mask |= 0x80 >> y
            cols.append(mask)
        label = 'DEL' if FONT_FIRST + c == 127 else repr(chr(FONT_FIRST + c))
        lines.append('    { %s },   /* %s */' % (', '.join('0x%02X' % v for v in cols), label))
    lines.append('};')
    open(path, 'w').write('\n'.join(lines) + '\n')


def write_sprites(src, path):
    """Patterns 2-9 (8 x 8 figures in the top-left of the pattern) doubled to fill it."""
    out = []
    pattern = None
    rows = []
    for line in open(src).read().split('\n'):
        header = re.match(r'^; pattern (\d+)(.*)$', line)
        if header:
            pattern = int(header.group(1))
            line = '; pattern %d%s' % (pattern, header.group(2).replace('(8x8, top-left)', '(doubled)')
                                       .replace('(8x8 corners, top-left)', '(corners, doubled)'))
        elif line.startswith('._demo_sprite_palette'):
            pattern = None
        if pattern is not None and pattern >= 2 and line.lstrip().startswith('db '):
            rows.append([int(v, 16) for v in re.findall(r'\$([0-9a-fA-F]+)', line.split(';')[0])])
            if len(rows) == 16:
                assert all(len(r) == 16 for r in rows), pattern
                assert all(rows[y][x] == 0 for y in range(16) for x in range(16) if x >= 8 or y >= 8), pattern
                for y in range(16):
                    r = [rows[y // 2][x // 2] for x in range(16)]
                    out.append('\tdb ' + ', '.join('$%02x' % v for v in r) + '  ; ' +
                               ''.join(PIXEL_LETTERS[v] for v in r))
                rows = []
            continue
        out.append(line)
    text = '\n'.join(out)
    old_key = (';   2-3  the player, two frames; 4, 5 and 7 objects (a potion, a sword, a\n'
               ';        ring); 6 a creature (a rat): 8x8 figures in the top-left of the\n'
               ';        pattern, the rest transparent, so they cover exactly one 8x8 map tile\n'
               ';   8-9  the targeting cursor, green (in range) and red (out of range): the\n'
               ';        corners of an 8x8 tile, so what is on the tile still shows\n')
    new_key = (';   2-3  the player, two frames; 4, 5 and 7 objects (a potion, a sword, a\n'
               ';        ring); 6 a creature (a rat): 16x16, covering one map tile.\n'
               ';        Placeholders: the 8x8 originals pixel-doubled (scripts/double_art.py)\n'
               ';   8-9  the targeting cursor, green (in range) and red (out of range): the\n'
               ';        corners of a map tile, so what is on the tile still shows\n')
    assert text.count(old_key) == 1
    open(path, 'w').write(text.replace(old_key, new_key))


def main():
    tiles = read_tiles(sys.argv[1])
    write_tiles(tiles, 'src/data/tile_defs_data.asm')
    write_font(tiles, 'src/nge/layer2/nge_layer2_font.c')
    write_sprites(sys.argv[2], 'src/data/sprite_data.asm')


if __name__ == '__main__':
    main()
