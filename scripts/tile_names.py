"""
Look names from an Aseprite tileset's slices, and the decorator's rules made
from them (docs/dungeon_decoration.md R17-R25, 5.2, 5.4).

A slice over a placed tile on the tilemap layer's canvas names that tile:

    <base>[_<code>[_<corners>]][_<variant>][_w<weight>]

- base: a terrain's name (wall, floor) or a feature's look (door_shut);
- code: four letters, north, east, south, west: the letter of the terrain
  that neighbour must be, or x for any. A terrain's looks only;
- corners: four more, north-east, south-east, south-west, north-west; all
  x if left out;
- variant: a number from 1, for interchangeable looks of one name; 1 if
  left out;
- weight: w and 1-255, a variant's share of its name's picks (1 if left
  out).

The game's terrains and looks are listed in a names file
(assets/tile_names.txt), every one required:

    terrain w wall      the terrain with letter w; its base is its name
    look door_shut      a look the game names

A terrain's rules are its looks, most specific first (more letters that
aren't x), by name on a tie; its default, its name alone, matches any
neighbours and comes last. Checking collects every problem: errors (the
import stops) and warnings (the slice is ignored, or two rules overlap),
each naming the slice and its canvas cell.
"""
import re

ANY = 'x'                       # a code's letter for any terrain
ANY_VALUE = 0xFF                # its value in the tables (TILE_ANY)
WEIGHT_MAX = 255
WEIGHT_TOTAL_MAX = 256          # the pick is an 8-bit hash modulo the total
TABLE_MAX = 255                 # uint8_t indices

# A code's letters and the corners' as positions in the neighbours' order N, NE, E, SE, S, SW, W, NW
SIDE_POSITIONS = (0, 2, 4, 6)
CORNER_POSITIONS = (1, 3, 5, 7)

UNNAMED = re.compile(r'(Slice \d+)?')                    # Aseprite's default names, or none
NAME_CHARS = re.compile(r'[a-z0-9_]+')
WEIGHT = re.compile(r'w(\d+)')
LETTER = re.compile(r'[a-z]')


class NamesFileError(Exception):
    """The names file can't be read."""


class Problem:
    def __init__(self, error, text):
        self.error = error      # True: an error; False: a warning
        self.text = text

    def __str__(self):
        return f'{"error" if self.error else "warning"}: {self.text}'


class Names:
    """The names file: the terrains, in TERRAIN_* order, and the other looks the game names."""

    def __init__(self, terrains=(), looks=()):
        self.terrains = list(terrains)                   # (letter, name)
        self.looks = list(looks)
        self.letters = {letter: n for n, (letter, _) in enumerate(self.terrains)}
        self.terrain_names = [name for _, name in self.terrains]

    def all(self):
        return self.terrain_names + self.looks

    def is_code(self, part):
        return len(part) == 4 and all(c == ANY or c in self.letters for c in part)


class Look:
    """One name's tiles: its variants, by number."""

    def __init__(self, base, code, corners):
        self.base = base
        self.code = code
        self.corners = corners
        self.variants = {}      # variant -> (tile, weight, cell)

    @property
    def name(self):
        return '_'.join(p for p in (self.base, self.code, self.corners) if p)

    def ordered(self):
        """Variants by number: (tile, weight, cell)."""
        return [self.variants[v] for v in sorted(self.variants)]

    def letters(self):
        """Its 8 letters in the neighbours' order: x where it doesn't care."""
        letters = [ANY] * 8
        for position, letter in zip(SIDE_POSITIONS, self.code or ''):
            letters[position] = letter
        for position, letter in zip(CORNER_POSITIONS, self.corners or ''):
            letters[position] = letter
        return letters

    def specificity(self):
        return sum(letter != ANY for letter in self.letters())

    def want(self, names):
        """The rule's want[8]: each neighbour's terrain, or ANY_VALUE."""
        return [ANY_VALUE if letter == ANY else names.letters[letter] for letter in self.letters()]


def read_names(path):
    """The names file as Names; raises NamesFileError naming the line."""
    terrains = []
    looks = []
    seen = set()
    letters = set()
    with open(path) as f:
        for number, line in enumerate(f, 1):
            words = line.split('#', 1)[0].split()
            if not words:
                continue
            if not ((words[0] == 'terrain' and len(words) == 3) or (words[0] == 'look' and len(words) == 2)):
                raise NamesFileError(f'{path}:{number}: expected "terrain <letter> <name>" or "look <name>"')
            name = words[-1]
            if not NAME_CHARS.fullmatch(name):
                raise NamesFileError(f'{path}:{number}: "{name}": lower case letters, digits and _ only')
            if name in seen:
                raise NamesFileError(f'{path}:{number}: "{name}" is listed twice')
            seen.add(name)
            if words[0] == 'look':
                looks.append(name)
                continue
            letter = words[1]
            if not LETTER.fullmatch(letter) or letter == ANY:
                raise NamesFileError(f'{path}:{number}: "{letter}": a terrain\'s letter is one lower case '
                                     f'letter, not "{ANY}" (any)')
            if letter in letters:
                raise NamesFileError(f'{path}:{number}: the letter "{letter}" is used twice')
            letters.add(letter)
            terrains.append((letter, name))
    return Names(terrains, looks)


def _base_of(name, known):
    """The longest known name that name is, or begins with followed by "_"; None if none."""
    best = None
    for k in known:
        if (name == k or name.startswith(k + '_')) and (best is None or len(k) > len(best)):
            best = k
    return best


def _tail(parts):
    """(variant, weight) from a name's last parts, popping them; None for a variant left out."""
    weight = 1
    variant = None
    match = WEIGHT.fullmatch(parts[-1]) if parts else None
    if match and len(parts) > 1:
        weight = int(match.group(1))
        parts.pop()
    if parts and parts[-1].isdigit() and len(parts) > 1:
        variant = int(parts.pop())
    return variant, weight


def parse(name, names=None):
    """
    (base, code, corners, variant, weight) of a name. With names, the base
    is the longest listed one it begins with, and None if there is none
    (the name isn't the game's). Without, every name is a base with no
    code. Raises ValueError with what is wrong.
    """
    if not NAME_CHARS.fullmatch(name):
        raise ValueError('lower case letters, digits and "_" only')
    parts = name.split('_')
    if '' in parts:
        raise ValueError('an empty part (a "_" at an end, or two together)')
    if names is None:
        variant, weight = _tail(parts)
        base, code, corners = '_'.join(parts), None, None
    else:
        base = _base_of(name, names.all())
        if base is None:
            return None, None, None, None, 1
        rest = name[len(base) + 1:].split('_') if name != base else []
        terrain = base in names.terrain_names
        code = corners = None
        if terrain and rest and names.is_code(rest[0]):
            code = rest.pop(0)
            if rest and names.is_code(rest[0]):
                corners = rest.pop(0)
        variant = None
        weight = 1
        if rest and rest[0].isdigit():
            variant = int(rest.pop(0))
        if rest and WEIGHT.fullmatch(rest[0]):
            weight = int(rest.pop(0)[1:])
        if rest:
            fits = 'a code, corners, a variant or a weight' if terrain else \
                'a variant or a weight (a feature\'s look has no code)'
            raise ValueError(f'"{rest[0]}" is not {fits} of "{base}"')
    if variant == 0:
        raise ValueError('variant 0: variants are numbered from 1')
    if not 1 <= weight <= WEIGHT_MAX:
        raise ValueError(f'weight {weight}: 1-{WEIGHT_MAX}')
    return base, code, corners, variant or 1, weight


def collect(sprite, names, layer=None):
    """
    The looks named by the sprite's slices.

    names: Names, or None to accept every base and require none. Returns
    (looks, problems): looks {(base, code, corners): Look}, problems a list
    of Problem, errors and warnings in the order found.
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
            base, code, corners, variant, weight = parse(s.name, names)
        except ValueError as e:
            problems.append(Problem(True, f'{where}: {e}'))
            continue
        if base is None:
            problems.append(Problem(False, f'{where}: "{s.name.split("_")[0]}" is not a name the game uses '
                                           '(assets/tile_names.txt): ignored'))
            continue
        tile = sprite.tile_at(s.x, s.y, layer)
        look = looks.setdefault((base, code, corners), Look(base, code, corners))
        if variant in look.variants:
            other_tile, _, other_cell = look.variants[variant]
            if other_tile == tile:
                problems.append(Problem(False, f'{where}: repeats the slice at {other_cell}: ignored'))
            else:
                problems.append(Problem(True, f'{where}: the same name is on tile {other_tile} at {other_cell}'))
            continue
        look.variants[variant] = (tile, weight, (col, row))

    for look in sorted(looks.values(), key=lambda look: look.name):
        if 1 not in look.variants:
            problems.append(Problem(True, f'"{look.name}" has variants but no variant 1 (a slice named '
                                          f'"{look.name}" or "{look.name}_1"): it is LOOK_{look.name.upper()}'))
        total = sum(weight for _, weight, _ in look.variants.values())
        if total > WEIGHT_TOTAL_MAX:
            problems.append(Problem(True, f'"{look.name}": its weights add up to {total}, at most '
                                          f'{WEIGHT_TOTAL_MAX}'))
    if names is not None:
        for name in names.all():
            if (name, None, None) not in looks:
                what = "the terrain's default" if name in names.terrain_names else 'a look the game needs'
                problems.append(Problem(True, f'no slice named "{name}": {what} (assets/tile_names.txt)'))
        problems += overlaps(looks, names)
    return looks, problems


def terrain_rules(looks, terrain):
    """A terrain's looks in the order they are tried: most specific first, by name, the default last."""
    return sorted((look for look in looks.values() if look.base == terrain),
                  key=lambda look: (look.code is None, -look.specificity(), look.name))


def _shared(a, b):
    """The letters of the cells two rules both match, or None if they share none."""
    shared = []
    for p, q in zip(a, b):
        if p != ANY and q != ANY and p != q:
            return None
        shared.append(p if p != ANY else q)
    return shared


def _covers(rule, cells):
    return all(p == ANY or p == q for p, q in zip(rule, cells))


def _code_text(letters):
    """8 letters as a name's code: the corners only when they aren't all x."""
    sides = ''.join(letters[p] for p in SIDE_POSITIONS)
    corners = ''.join(letters[p] for p in CORNER_POSITIONS)
    return sides if corners == ANY * 4 else f'{sides}_{corners}'


def overlaps(looks, names):
    """Warnings for rules of the same specificity that match some cells alike, with no more specific rule there."""
    problems = []
    for terrain in names.terrain_names:
        rules = [look for look in terrain_rules(looks, terrain) if look.code and 1 in look.variants]
        for i, a in enumerate(rules):
            for b in rules[i + 1:]:
                if a.specificity() != b.specificity():
                    continue
                cells = _shared(a.letters(), b.letters())
                if cells is None:
                    continue
                if any(r.specificity() > a.specificity() and _covers(r.letters(), cells) for r in rules):
                    continue
                problems.append(Problem(False, f'slices "{a.name}" at {a.variants[1][2]} and "{b.name}" at '
                                               f'{b.variants[1][2]} both match "{_code_text(cells)}": '
                                               f'"{a.name}" is used'))
    return problems


def tables(looks, names, banks_of):
    """
    The decorator's tables: (bases, rules, entries).

    bases: [(terrain, first rule, rules)], one a terrain in the names
    file's order (none without one); rules: [(want, first entry, entries,
    look name)], each terrain's in the order tried, its default last;
    entries: [(tile, bank, weight, look name)], each rule's variants by
    number. banks_of: each tile's palette bank.
    """
    entries = []
    rules = []
    bases = []
    for terrain in (names.terrain_names if names is not None else []):
        first_rule = len(rules)
        for look in terrain_rules(looks, terrain):
            first = len(entries)
            for tile, weight, _ in look.ordered():
                entries.append((tile, banks_of[tile], weight, look.name))
            rules.append((look.want(names), first, len(entries) - first, look.name))
        bases.append((terrain, first_rule, len(rules) - first_rule))
    if len(entries) > TABLE_MAX or len(rules) > TABLE_MAX:
        raise ValueError(f'{len(entries)} looks, {len(rules)} rules: at most {TABLE_MAX} each')
    return bases, rules, entries
