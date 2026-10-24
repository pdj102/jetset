"""
Look names from an Aseprite tileset's slices, and the decorator's rules made
from them (docs/dungeon_decoration.md R17-R25, 5.2, 5.4).

A slice over a placed tile on the tilemap layer's canvas names that tile:

    <base>[_<code>][_<diagonal>][_<variant>][_w<weight>]

- base: one or more words (wall, floor_lip, door_shut);
- code: four of x and o, north, east, south, west: x if that neighbour
  joins the cell, by the base's join set (game/level/level_deco.c);
- diagonal: ne, se, sw or nw, after a code: that neighbour doesn't join;
- variant: a number, for interchangeable looks of one name;
- weight: w and 1-255, a variant's chances (1 if left out).

The game's bases and looks are listed in a names file (assets/tile_names.txt):

    base wall  required      the wall's fallback (wall, no code) must exist
    base floor_lip           drawn or not
    look door_shut required  a look the game names

Checking collects every problem: errors (the import stops) and warnings
(the slice is ignored), each naming the slice and its canvas cell.
"""
import re

# Signature bits of the 8 neighbours (game/level/level_deco.c)
SIDE_BITS = (0x01, 0x04, 0x10, 0x40)                    # N, E, S, W: a code's letters in order
DIAGONAL_BITS = {'ne': 0x02, 'se': 0x08, 'sw': 0x20, 'nw': 0x80}
CARDINALS = 0x01 | 0x04 | 0x10 | 0x40

WEIGHT_MAX = 255
TABLE_MAX = 255                                          # uint8_t indices

UNNAMED = re.compile(r'(Slice \d+)?')                    # Aseprite's default names, or none
NAME_CHARS = re.compile(r'[a-z0-9_]+')
CODE = re.compile(r'[xo]{4}')
WEIGHT = re.compile(r'w(\d+)')


class NamesFileError(Exception):
    """The names file can't be read."""


class Problem:
    def __init__(self, error, text):
        self.error = error      # True: an error; False: a warning
        self.text = text

    def __str__(self):
        return f'{"error" if self.error else "warning"}: {self.text}'


class Look:
    """One name's tiles: its variants, (variant, tile, weight), variant None first."""

    def __init__(self, base, code, diagonal):
        self.base = base
        self.code = code
        self.diagonal = diagonal
        self.variants = {}      # variant -> (tile, weight, cell)

    @property
    def name(self):
        return '_'.join(p for p in (self.base, self.code, self.diagonal) if p)

    def ordered(self):
        """Variants in order: the unnumbered one, then by number."""
        return [self.variants[v] for v in sorted(self.variants, key=lambda v: -1 if v is None else v)]

    def care_want(self):
        """The rule's pattern on the signature (only for a look with a code)."""
        care = CARDINALS
        want = sum(bit for bit, letter in zip(SIDE_BITS, self.code) if letter == 'x')
        if self.diagonal:
            care |= DIAGONAL_BITS[self.diagonal]
        return care, want


def read_names(path):
    """The names file: {name: required}, in order; raises NamesFileError naming the line."""
    names = {}
    with open(path) as f:
        for number, line in enumerate(f, 1):
            words = line.split('#', 1)[0].split()
            if not words:
                continue
            if words[0] not in ('base', 'look') or len(words) not in (2, 3) or \
                    (len(words) == 3 and words[2] != 'required'):
                raise NamesFileError(f'{path}:{number}: expected "base|look <name> [required]"')
            if not NAME_CHARS.fullmatch(words[1]):
                raise NamesFileError(f'{path}:{number}: "{words[1]}": lower case letters, digits and _ only')
            if words[1] in names:
                raise NamesFileError(f'{path}:{number}: "{words[1]}" is listed twice')
            names[words[1]] = len(words) == 3
    return names


def parse(name):
    """
    (base, code, diagonal, variant, weight) of a name, reading from its end.
    Raises ValueError with what is wrong.
    """
    if not NAME_CHARS.fullmatch(name):
        raise ValueError('lower case letters, digits and "_" only')
    parts = name.split('_')
    if '' in parts:
        raise ValueError('an empty part (a "_" at an end, or two together)')
    weight = 1
    variant = code = diagonal = None
    match = WEIGHT.fullmatch(parts[-1])
    if match and len(parts) > 1:
        weight = int(match.group(1))
        if not 1 <= weight <= WEIGHT_MAX:
            raise ValueError(f'weight {weight}: 1-{WEIGHT_MAX}')
        parts.pop()
    if parts[-1].isdigit() and len(parts) > 1:
        variant = int(parts.pop())
    if parts[-1] in DIAGONAL_BITS and len(parts) > 1:
        diagonal = parts.pop()
    if CODE.fullmatch(parts[-1]) and len(parts) > 1:
        code = parts.pop()
    if diagonal and not code:
        raise ValueError(f'the diagonal "{diagonal}" needs a code before it')
    return '_'.join(parts), code, diagonal, variant, weight


def _known_prefix(name, known):
    """The longest known name that name begins with, followed by "_"; None if none."""
    best = None
    for k in known:
        if name.startswith(k + '_') and (best is None or len(k) > len(best)):
            best = k
    return best


def _first_misfit(parts):
    """The first of the parts after a base that isn't, in order, a code, a diagonal, a variant or a weight."""
    i = 0
    for fits in (CODE.fullmatch, DIAGONAL_BITS.__contains__, str.isdigit, WEIGHT.fullmatch):
        if i < len(parts) and fits(parts[i]):
            i += 1
    return parts[i] if i < len(parts) else parts[-1]


def collect(sprite, names, layer=None):
    """
    The looks named by the sprite's slices.

    names: the names file's {name: required}, or None to accept every base
    and require none. Returns (looks, problems): looks {(base, code,
    diagonal): Look}, problems a list of Problem, errors and warnings in
    the order found.
    """
    tileset = sprite.tilemap_tileset(layer)
    tw, th = tileset.width, tileset.height
    looks = {}
    problems = []

    for s in sprite.slices:
        col, row = s.x // tw, s.y // th
        where = f'slice "{s.name}" at ({col}, {row})'
        if UNNAMED.fullmatch(s.name):
            problems.append(Problem(False, f'{where}: no name: ignored'))
            continue
        if s.width > tw or s.height > th:
            problems.append(Problem(True, f'{where}: {s.width} x {s.height}, larger than one tile ({tw} x {th})'))
            continue
        if s.x < 0 or s.y < 0 or s.x + s.width > sprite.width or s.y + s.height > sprite.height:
            problems.append(Problem(True, f'{where}: off the canvas'))
            continue
        if (s.x + s.width - 1) // tw != col or (s.y + s.height - 1) // th != row:
            problems.append(Problem(True, f'{where}: crosses from one tile into the next'))
            continue
        try:
            base, code, diagonal, variant, weight = parse(s.name)
        except ValueError as e:
            problems.append(Problem(True, f'{where}: {e}'))
            continue
        if names is not None and base not in names:
            known = _known_prefix(s.name, names)
            if known is not None:
                part = _first_misfit(s.name[len(known) + 1:].split('_'))
                problems.append(Problem(True, f'{where}: "{part}" is not a code, a diagonal, a variant or '
                                              f'a weight of "{known}"'))
            else:
                problems.append(Problem(False, f'{where}: "{base}" is not a name the game uses '
                                               '(assets/tile_names.txt): ignored'))
            continue
        tile = sprite.tile_at(s.x, s.y, layer)
        look = looks.setdefault((base, code, diagonal), Look(base, code, diagonal))
        if variant in look.variants:
            other_tile, _, other_cell = look.variants[variant]
            if other_tile == tile:
                problems.append(Problem(False, f'{where}: repeats the slice at {other_cell}: ignored'))
            else:
                problems.append(Problem(True, f'{where}: the same name is on tile {other_tile} at {other_cell}'))
            continue
        look.variants[variant] = (tile, weight, (col, row))

    for name, required in (names or {}).items():
        if required and (name, None, None) not in looks:
            problems.append(Problem(True, f'no slice named "{name}": a look the game needs '
                                          '(assets/tile_names.txt)'))
    return looks, problems


def base_order(looks, names):
    """The bases with looks, in the names file's order (or by name with no file)."""
    bases = {look.base for look in looks.values()}
    order = list(names) if names is not None else sorted(bases)
    return [b for b in order if b in bases]


def tables(looks, names, banks_of):
    """
    The decorator's tables: (bases, rules, entries).

    bases: [(base, first rule, rules, fallback's first entry, its entries)];
    rules: [(care, want, first entry, entries, look name)];
    entries: [(tile, bank, weight, look name)]. A base's rules: those with a
    diagonal first, so a corner is found before the wall it is part of.
    banks_of: each tile's palette bank.
    """
    entries = []
    rules = []
    bases = []

    def add_entries(look):
        first = len(entries)
        for tile, weight, _ in look.ordered():
            entries.append((tile, banks_of[tile], weight, look.name))
        return first, len(entries) - first

    for base in base_order(looks, names):
        coded = sorted((look for look in looks.values() if look.base == base and look.code),
                       key=lambda look: (look.diagonal is None, look.code, look.diagonal or ''))
        first_rule = len(rules)
        for look in coded:
            care, want = look.care_want()
            first, count = add_entries(look)
            rules.append((care, want, first, count, look.name))
        fallback = looks.get((base, None, None))
        fb_first, fb_count = add_entries(fallback) if fallback else (0, 0)
        bases.append((base, first_rule, len(rules) - first_rule, fb_first, fb_count))
    if len(entries) > TABLE_MAX or len(rules) > TABLE_MAX or len(bases) > TABLE_MAX:
        raise ValueError(f'{len(entries)} looks, {len(rules)} rules, {len(bases)} bases: '
                         f'at most {TABLE_MAX} each')
    return bases, rules, entries
