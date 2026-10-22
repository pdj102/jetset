#!/usr/bin/env python3
"""
Imports the game's sprites from an Aseprite file (docs/sprite_import.md).

From the repository root:

    python3 -I scripts/ase2sprites.py assets/Sprites.aseprite <out dir>

The file is indexed, 16 x 16, transparent index 0. Frame n is 4-bit pattern
n: its visible layers combined, each pixel its palette index & 15 (its
slot). The palette is the sprite palette, row b bank b. Tags name the
sprites and their runs: tag rat.walk is SPRITE_RAT_WALK (its first
pattern), with _COUNT, _RATE (frames at 50 Hz) and _BANK (the row its
frames were drawn in). Writes into the out dir:

- sprite_patterns_data.asm: the patterns, 64 a page, pages 137 and 139;
- sprite_palette_data.asm: the palette's banks, page 135;
- sprite_data.h: the pages and counts to load, and each tag's names.

On an error it writes nothing, prints the error and exits with status 1.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import aseprite      # noqa: E402
import asm_out       # noqa: E402
import next_colour   # noqa: E402

PATTERN_PIXELS = 16
PATTERNS_MAX = 128
PATTERNS_PER_PAGE = 64           # 4-bit patterns of 128 bytes in an 8K page
PATTERN_PAGES = (137, 139)
PALETTE_PAGE = 135
BANKS_MAX = 16
FRAME_MS = 20                    # a frame at 50 Hz
RATE_MAX = 255

PATTERNS_FILE = 'sprite_patterns_data.asm'
PALETTE_FILE = 'sprite_palette_data.asm'
HEADER_FILE = 'sprite_data.h'

# Names the header defines itself, which no tag's may repeat
FIXED_NAMES = {'SPRITE_PALETTE_PAGE', 'SPRITE_PALETTE_ENTRIES', 'SPRITE_PATTERNS'} | {
    f'SPRITE_PATTERNS_{kind}_{n}' for kind in ('PAGE', 'COUNT') for n in range(len(PATTERN_PAGES))}


class SpriteError(Exception):
    """The sprite file breaks one of the import's rules."""


class Run:
    """A tag as a run of patterns (nge_render_set_anim(), nge_render_play())."""

    def __init__(self, tag, name, first, count, rate, bank):
        self.tag = tag        # the Aseprite tag's name
        self.name = name      # SPRITE_<NAME>
        self.first = first
        self.count = count
        self.rate = rate
        self.bank = bank


def c_name(tag):
    """SPRITE_<NAME> for a tag's name, or None if it doesn't make a C identifier."""
    name = 'SPRITE_' + re.sub(r'[.\- ]', '_', tag).upper()
    return name if re.fullmatch(r'[A-Z_][A-Z0-9_]*', name) else None


def check_sprite(sprite):
    """R1, R5: the file's mode, size and layers."""
    if sprite.depth != aseprite.DEPTH_INDEXED:
        raise SpriteError('the sprite must be in indexed colour mode (docs/sprite_import.md D3)')
    if (sprite.width, sprite.height) != (PATTERN_PIXELS, PATTERN_PIXELS):
        raise SpriteError(f'the canvas is {sprite.width} x {sprite.height}, '
                          f'not {PATTERN_PIXELS} x {PATTERN_PIXELS}')
    if sprite.transparent_index != 0:
        raise SpriteError(f'the transparent colour is index {sprite.transparent_index}, not 0')
    if not sprite.frames:
        raise SpriteError('the sprite has no frames')
    if len(sprite.frames) > PATTERNS_MAX:
        raise SpriteError(f'{len(sprite.frames)} frames, at most {PATTERNS_MAX}')
    for i, layer in enumerate(sprite.layers):
        if not layer.shown:
            continue
        if layer.kind == aseprite.LAYER_TILEMAP:
            raise SpriteError(f'layer "{layer.name}" is a tilemap layer: draw sprites on normal layers')
        if layer.opacity != 255:
            raise SpriteError(f'layer "{layer.name}": opacity {layer.opacity}, only 255 is supported')
        if layer.kind != aseprite.LAYER_IMAGE:
            continue
        for n in range(len(sprite.frames)):
            cel = sprite.cel(n, i)
            if cel is None:
                continue
            if cel.kind not in (aseprite.CEL_RAW, aseprite.CEL_COMPRESSED):
                raise SpriteError(f'frame {n}, layer "{layer.name}": a cel of kind {cel.kind} '
                                  'is not supported')
            if cel.opacity != 255:
                raise SpriteError(f'frame {n}, layer "{layer.name}": cel opacity {cel.opacity}, '
                                  'only 255 is supported')


def palette_banks(sprite):
    """R12: the palette as banks of 16 Next colours (9-bit); slot 0 of each is black."""
    if len(sprite.palette) > BANKS_MAX * next_colour.BANK_SIZE:
        raise SpriteError(f'the palette has {len(sprite.palette)} entries, at most '
                          f'{BANKS_MAX * next_colour.BANK_SIZE} ({BANKS_MAX} banks)')
    count = -(-len(sprite.palette) // next_colour.BANK_SIZE)
    banks = []
    for b in range(count):
        bank = [0]
        for s in range(1, next_colour.BANK_SIZE):
            i = b * next_colour.BANK_SIZE + s
            if i >= len(sprite.palette):
                bank.append(0)
                continue
            colour = next_colour.to_next(sprite.palette[i])
            if colour is None:
                raise SpriteError(f'palette entry {i} (bank {b}, slot {s}): colour '
                                  f'{sprite.palette[i][:3]} is not a Next colour')
            bank.append(colour)
        banks.append(bank)
    return banks


def frame_patterns(sprite):
    """R3, R4: each frame's slots (rows of 0-15) and the palette row it was drawn in (None if empty)."""
    patterns = []
    rows = []
    for n in range(len(sprite.frames)):
        indices = sprite.frame_indices(n)
        used = set()
        slots = []
        for y, line in enumerate(indices):
            out = []
            for x, index in enumerate(line):
                if index is None:
                    out.append(0)
                    continue
                if index % next_colour.BANK_SIZE == 0:
                    raise SpriteError(f'frame {n}, pixel ({x}, {y}): palette index {index} is slot 0, '
                                      'transparent in the game')
                if index >= len(sprite.palette):
                    raise SpriteError(f'frame {n}, pixel ({x}, {y}): palette index {index} past the '
                                      f'palette ({len(sprite.palette)} entries)')
                used.add(index // next_colour.BANK_SIZE)
                out.append(index % next_colour.BANK_SIZE)
            slots.append(out)
        if len(used) > 1:
            raise SpriteError(f'frame {n} uses palette rows {", ".join(str(r) for r in sorted(used))}: '
                              'draw a frame with one row')
        patterns.append(slots)
        rows.append(used.pop() if used else None)
    return patterns, rows


def tag_runs(sprite, rows):
    """R6-R10: each tag as a run, in timeline order of their first frames."""
    runs = []
    names = {}
    tagged = set()
    for tag in sprite.tags:
        name = c_name(tag.name)
        if name is None:
            raise SpriteError(f'tag "{tag.name}": not a C name (letters, digits, ".", "-", " " and "_")')
        if name in FIXED_NAMES:
            raise SpriteError(f'tag "{tag.name}": its name {name} is one the header uses itself')
        if tag.direction != aseprite.TAG_FORWARD:
            direction = (aseprite.TAG_DIRECTIONS[tag.direction]
                         if tag.direction < len(aseprite.TAG_DIRECTIONS) else tag.direction)
            raise SpriteError(f'tag "{tag.name}": plays {direction}, only forward is supported')
        if not 0 <= tag.first <= tag.last < len(sprite.frames):
            raise SpriteError(f'tag "{tag.name}": frames {tag.first}-{tag.last} are not in the sprite')
        frames = range(tag.first, tag.last + 1)
        durations = {sprite.frames[n].duration for n in frames}
        if len(durations) > 1:
            raise SpriteError(f'tag "{tag.name}": its frames have different durations '
                              f'({", ".join(str(d) for d in sorted(durations))} ms)')
        rate = max(1, min(RATE_MAX, (durations.pop() + FRAME_MS // 2) // FRAME_MS))
        used = {rows[n] for n in frames} - {None}
        if len(used) > 1:
            raise SpriteError(f'tag "{tag.name}": its frames use palette rows '
                              f'{", ".join(str(r) for r in sorted(used))}: draw them with one row')
        run = Run(tag.name, name, tag.first, len(frames), rate, used.pop() if used else 0)
        for suffix in ('', '_COUNT', '_RATE', '_BANK'):
            full = name + suffix
            if full in names:
                raise SpriteError(f'tag "{tag.name}": its name {full} is also tag "{names[full]}"\'s')
            if full in FIXED_NAMES:
                raise SpriteError(f'tag "{tag.name}": its name {full} is one the header uses itself')
            names[full] = tag.name
        runs.append(run)
        tagged.update(frames)
    untagged = [n for n in range(len(sprite.frames)) if n not in tagged]
    if untagged:
        raise SpriteError(f'frame {untagged[0]} is in no tag: tag every frame '
                          f'({len(untagged)} untagged)')
    runs.sort(key=lambda run: (run.first, run.name))
    return runs


def convert(sprite):
    """Check the sprite and convert it: (patterns, rows, runs, banks)."""
    check_sprite(sprite)
    banks = palette_banks(sprite)
    patterns, rows = frame_patterns(sprite)
    for n, row in enumerate(rows):
        if row is not None and row >= len(banks):
            raise SpriteError(f'frame {n} uses palette row {row}, past the palette')
    runs = tag_runs(sprite, rows)
    return patterns, rows, runs, banks


def pattern_bytes(slots):
    """A pattern's 128 bytes: rows top to bottom, two pixels a byte, the left pixel in the high nibble."""
    return [row[x] << 4 | row[x + 1] for row in slots for x in range(0, PATTERN_PIXELS, 2)]


def pages(count):
    """The pattern pages used: (page, first pattern, count)."""
    return [(page, i * PATTERNS_PER_PAGE, min(PATTERNS_PER_PAGE, count - i * PATTERNS_PER_PAGE))
            for i, page in enumerate(PATTERN_PAGES) if count > i * PATTERNS_PER_PAGE]


def _tags_of(n, runs):
    return [run.tag for run in runs if run.first <= n < run.first + run.count]


def patterns_asm(source, patterns, rows, runs):
    lines = [
        "; The game's sprites: 4-bit patterns, 16 x 16, 128 bytes each, two pixels a",
        '; byte with the left pixel in the high nibble. Pixel values are slots: 0',
        "; transparent, 1-15 through the sprite's palette bank (sprite_palette_data.asm,",
        '; docs/sprite_import.md section 5.2). 64 patterns a page, assembled for slot',
        '; 7: load each page with nge_sprite_load_patterns4() (sprite_data.h).',
        ';',
        f'; Generated by scripts/ase2sprites.py from {source}: do not edit.',
    ]
    for p, (page, first, count) in enumerate(pages(len(patterns))):
        lines += ['', f'SECTION PAGE_{page}', '', f'PUBLIC _sprite_patterns_{p}', '', f'._sprite_patterns_{p}']
        for n in range(first, first + count):
            bank = '' if rows[n] is None else f', drawn in bank {rows[n]}'
            lines.append(f'; pattern {n}: {", ".join(_tags_of(n, runs))}{bank}')
            data = pattern_bytes(patterns[n])
            for y in range(PATTERN_PIXELS):
                picture = ''.join('.' if v == 0 else f'{v:X}' for v in patterns[n][y])
                lines.append(asm_out.db(data[y * 8:y * 8 + 8], picture))
    return '\n'.join(lines) + '\n'


def palette_asm(source, banks):
    lines = [
        "; The sprite palette: banks of 16 entries, from entry 0 of sprite palette 0,",
        '; as nge_palette_entry_9_t (RRRGGGBB, then the low blue bit). Every bank has',
        '; the slots of docs/sprite_import.md section 5.2; entry 0 of each is',
        '; transparent. In page 135, assembled for slot 7: load it with',
        '; nge_palette_load9() (hw/nge_palette.h, sprite_data.h).',
        ';',
        f'; Generated by scripts/ase2sprites.py from {source}: do not edit.',
        '',
        f'SECTION PAGE_{PALETTE_PAGE}',
        '',
        'PUBLIC _sprite_palette',
        '',
        '._sprite_palette',
    ]
    for b, bank in enumerate(banks):
        lines.append(f'; bank {b}')
        data = [byte for colour in bank for byte in next_colour.entry_bytes(colour)]
        lines.append(asm_out.db(data[:16]))
        lines.append(asm_out.db(data[16:]))
    return '\n'.join(lines) + '\n'


def header(source, patterns, runs, banks):
    def define(name, value, doc=None):
        line = f'#define {name:<28} {value}'
        return f'{line:<40} /**< {doc} */' if doc else line

    used = pages(len(patterns))
    lines = [
        '#ifndef SPRITE_DATA_H',
        '#define SPRITE_DATA_H',
        '',
        '/**',
        ' * @file sprite_data.h',
        " * @brief The game's sprites (sprite_patterns_data.asm) and their palette",
        ' *        (sprite_palette_data.asm): 4-bit patterns, from pattern 0.',
        ' *',
        f' * Generated by scripts/ase2sprites.py from {source}: do not edit.',
        ' *',
        ' * Each Aseprite tag is a run of patterns: SPRITE_<NAME> is its first',
        ' * pattern, _COUNT its patterns and _RATE the frames each shows',
        ' * (nge_render_set_anim(), nge_render_play()); _BANK is the palette bank',
        ' * it was drawn in, NGE_SPRITE_PALETTE(bank) (docs/sprite_import.md 5.2).',
        ' * Show its patterns with NGE_SPRITE_PATTERN4.',
        ' */',
        '',
        '#include <stdint.h>',
        '',
        '#include "hw/nge_palette.h"',
        '',
        define('SPRITE_PALETTE_PAGE', PALETTE_PAGE, f'SECTION PAGE_{PALETTE_PAGE} in sprite_palette_data.asm.'),
        define('SPRITE_PALETTE_ENTRIES', len(banks) * next_colour.BANK_SIZE,
               'Entries in sprite_palette: load them from entry 0.'),
        define('SPRITE_PATTERNS', len(patterns), '4-bit patterns, from pattern 0.'),
        '',
    ]
    for p, (page, first, count) in enumerate(used):
        lines.append(define(f'SPRITE_PATTERNS_PAGE_{p}', page,
                            f'Patterns {first}-{first + count - 1}: sprite_patterns_{p}.'))
        lines.append(define(f'SPRITE_PATTERNS_COUNT_{p}', count))
    for run in runs:
        last = run.first + run.count - 1
        frames = f'frame {run.first}' if run.count == 1 else f'frames {run.first}-{last}'
        lines += [
            '',
            define(run.name, run.first, f'Tag {run.tag}: {frames}.'),
            define(run.name + '_COUNT', run.count),
            define(run.name + '_RATE', run.rate),
            define(run.name + '_BANK', run.bank),
        ]
    lines.append('')
    lines += [f'extern const uint8_t sprite_patterns_{p}[];' for p in range(len(used))]
    lines += [
        'extern const nge_palette_entry_9_t sprite_palette[];',
        '',
        '#endif',
    ]
    return '\n'.join(lines) + '\n'


def run(source, out_dir):
    """Import source into out_dir; returns the summary lines. Raises on an error, writing nothing."""
    sprite = aseprite.read(source)
    patterns, rows, runs, banks = convert(sprite)
    asm_out.write_all(out_dir, {
        PATTERNS_FILE: patterns_asm(source, patterns, rows, runs),
        PALETTE_FILE: palette_asm(source, banks),
        HEADER_FILE: header(source, patterns, runs, banks),
    })
    used = sorted({run.bank for run in runs})
    page_count = len(pages(len(patterns)))
    return [f'{source}: {len(patterns)} of {PATTERNS_MAX} patterns, {page_count} '
            f'{"page" if page_count == 1 else "pages"}, {len(runs)} tags, {len(banks)} banks in the palette',
            f'  banks drawn in: {", ".join(str(b) for b in used)}']


def main(argv=None):
    parser = argparse.ArgumentParser(description="Import the game's sprites from an Aseprite file.")
    parser.add_argument('source', help='the .aseprite file')
    parser.add_argument('out_dir', help='where to write the generated files')
    args = parser.parse_args(argv)
    try:
        summary = run(args.source, args.out_dir)
    except (OSError, aseprite.AsepriteError, next_colour.ColourError, SpriteError) as e:
        print(f'ase2sprites: error: {e}', file=sys.stderr)
        return 1
    print('\n'.join(summary))
    return 0


if __name__ == '__main__':
    sys.exit(main())
