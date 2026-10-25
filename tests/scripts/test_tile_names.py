"""
Tests for the look names (scripts/tile_names.py, the slices and tilemap cels
in aseprite.py, and their output from ase2tiles.py; docs/dungeon_decoration.md
R17-R25, 5.10). Run by make test, or:

    python3 -I -m unittest discover -s tests/scripts
"""
import contextlib
import io
import os
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
sys.path.insert(0, HERE)

import ase2tiles     # noqa: E402
import aseprite      # noqa: E402
import ase_writer    # noqa: E402
import tile_names    # noqa: E402

CLEAR = (0, 0, 0, 0)


def solid(level):
    """A 16 x 16 tile of one opaque grey, a distinct one for each level 1-7."""
    v = (0, 36, 73, 109, 146, 182, 219, 255)[level]
    return [[(v, v, v, 255)] * 16 for _ in range(16)]


EMPTY = [[CLEAR] * 16 for _ in range(16)]
TILES = [EMPTY] + [solid(n) for n in range(1, 8)]   # tile 0 empty, 1-7 greys

NAMES = tile_names.Names([('r', 'rock'), ('w', 'wall'), ('f', 'floor')], ['door_shut'])
NAMES_TEXT = 'terrain r rock\nterrain w wall\nterrain f floor\nlook door_shut\n'


class TempDir:
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.path = self.dir.name

    def tearDown(self):
        self.dir.cleanup()

    def write(self, data, name='test.aseprite'):
        path = os.path.join(self.path, name)
        with open(path, 'wb') as f:
            f.write(data)
        return path

    def sprite(self, canvas, slices, **kw):
        return aseprite.read(self.write(ase_writer.named_tileset_file(TILES, canvas, slices, **kw)))

    def collect(self, canvas, slices, names=NAMES, **kw):
        return tile_names.collect(self.sprite(canvas, slices, **kw), names)


# The required names, on a 4 x 2 canvas of tiles 1-7 (and 0 at the end)
CANVAS = [[1, 2, 3, 4], [5, 6, 7, 0]]
REQUIRED = [('rock', 3, 1), ('wall', 0, 0), ('floor', 1, 0), ('door_shut', 2, 0)]


def errors(problems):
    return [str(p) for p in problems if p.error]


def warnings(problems):
    return [str(p) for p in problems if not p.error]


class TestReading(TempDir, unittest.TestCase):

    def test_slices_and_the_tile_under_them(self):
        sprite = self.sprite(CANVAS, [('wall', 0, 0), ('edge', 20, 18, 4, 4)])
        self.assertEqual([(s.name, s.x, s.y, s.width, s.height) for s in sprite.slices],
                         [('wall', 0, 0, 16, 16), ('edge', 20, 18, 4, 4)])
        self.assertEqual(sprite.tile_at(0, 0), 1)
        self.assertEqual(sprite.tile_at(20, 18), 6)
        self.assertEqual(sprite.tile_at(63, 31), 0)

    def test_outside_the_cel_is_tile_0(self):
        sprite = self.sprite([[1, 2]], [], width=64, height=32, cel_at=(16, 0))
        self.assertEqual(sprite.tile_at(0, 0), 0)      # left of the cel
        self.assertEqual(sprite.tile_at(16, 0), 1)
        self.assertEqual(sprite.tile_at(48, 0), 0)     # right of it
        self.assertEqual(sprite.tile_at(16, 20), 0)    # below it

    def test_flip_bits_are_masked(self):
        data = ase_writer.aseprite_file([ase_writer.layer_chunk('Tilemap 1', kind=2),
                                         ase_writer.tileset_chunk(TILES, ase_writer.RGBA),
                                         ase_writer.tilemap_cel_chunk(0, [[3]], flips=0x80000000)])
        self.assertEqual(aseprite.read(self.write(data)).tile_at(0, 0), 3)

    def test_a_slice_with_keys_on_several_frames_and_a_pivot(self):
        data = ase_writer.aseprite_file([ase_writer.layer_chunk('Tilemap 1', kind=2),
                                         ase_writer.tileset_chunk(TILES, ase_writer.RGBA),
                                         ase_writer.tilemap_cel_chunk(0, CANVAS),
                                         ase_writer.slice_chunk('wall', 0, 0, keys=[(3, 32, 0, 16, 16),
                                                                                    (0, 16, 0, 16, 16)],
                                                                flags=1 | 2)],
                                        width=64, height=32)
        s = aseprite.read(self.write(data)).slices[0]
        self.assertEqual((s.x, s.y), (16, 0))          # frame 0's key


class TestParse(unittest.TestCase):

    def test_parts(self):
        for name, parts in [('wall', ('wall', None, None, 1, 1)),
                            ('wall_xwfw', ('wall', 'xwfw', None, 1, 1)),
                            ('wall_wwww_xfxx', ('wall', 'wwww', 'xfxx', 1, 1)),
                            ('floor_2', ('floor', None, None, 2, 1)),
                            ('floor_1_w7', ('floor', None, None, 1, 7)),
                            ('floor_w8', ('floor', None, None, 1, 8)),
                            ('floor_xxrx_3_w2', ('floor', 'xxrx', None, 3, 2)),
                            ('door_shut', ('door_shut', None, None, 1, 1)),
                            ('door_shut_2', ('door_shut', None, None, 2, 1)),
                            ('pillar', (None, None, None, None, 1))]:
            self.assertEqual(tile_names.parse(name, NAMES), parts, msg=name)

    def test_without_a_names_file_every_name_is_a_base(self):
        self.assertEqual(tile_names.parse('pillar_2_w3'), ('pillar', None, None, 2, 3))
        self.assertEqual(tile_names.parse('wall_xwfw'), ('wall_xwfw', None, None, 1, 1))

    def test_errors(self):
        for name, text in [('Wall', 'lower case'), ('wall__x', 'empty part'), ('wall_', 'empty part'),
                           ('wall_xwfq', '"xwfq" is not a code, corners, a variant or a weight of "wall"'),
                           ('wall_xwfw_xx', '"xx" is not a code'),
                           ('door_shut_xwfw', '"xwfw" is not a variant or a weight .*has no code'),
                           ('floor_lip', '"lip" is not a code'), ('floor_2_3', '"3" is not'),
                           ('floor_0', 'variant 0'), ('floor_w0', 'weight 0'), ('floor_w256', 'weight 256')]:
            with self.assertRaisesRegex(ValueError, text, msg=name):
                tile_names.parse(name, NAMES)


class TestChecks(TempDir, unittest.TestCase):

    def test_the_required_names_pass(self):
        looks, problems = self.collect(CANVAS, REQUIRED)
        self.assertEqual(problems, [])
        self.assertEqual(looks[('rock', None, None)].ordered()[0][0], 0)   # tile 0 named

    def test_a_default_or_a_look_missing(self):
        _, problems = self.collect(CANVAS, REQUIRED[1:3])
        self.assertEqual(errors(problems), [
            'error: no slice named "rock": the terrain\'s default (assets/tile_names.txt)',
            'error: no slice named "door_shut": a look the game needs (assets/tile_names.txt)'])

    def test_slices_the_wrong_size_or_place(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall_xwfw', 0, 0, 20, 16), ('wall_fwxw', 8, 0, 16, 16),
                                                       ('wall_wfwx', 60, 0, 8, 8)])
        self.assertEqual(errors(problems), [
            'error: slice "wall_xwfw" at (0, 0): 20 x 16, larger than one tile (16 x 16)',
            'error: slice "wall_fwxw" at (0, 0): crosses from one tile into the next',
            'error: slice "wall_wfwx" at (3, 0): off the canvas'])

    def test_a_mistyped_code_of_a_known_base(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall_xwfq', 1, 1)])
        self.assertEqual(errors(problems), ['error: slice "wall_xwfq" at (1, 1): "xwfq" is not a code, corners, '
                                            'a variant or a weight of "wall"'])

    def test_a_name_on_two_tiles(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall', 1, 1)])
        self.assertEqual(errors(problems), ['error: slice "wall" at (1, 1): the same name is on tile 1 at (0, 0)'])

    def test_a_name_without_a_number_is_variant_1(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('floor_1', 1, 1)])
        self.assertEqual(errors(problems), ['error: slice "floor_1" at (1, 1): the same name is on tile 2 at (1, 0)'])

    def test_variants_without_a_variant_1(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall_xwfw_2', 1, 1)])
        self.assertEqual(errors(problems), ['error: "wall_xwfw" has variants but no variant 1 (a slice named '
                                            '"wall_xwfw" or "wall_xwfw_1"): it is LOOK_WALL_XWFW'])

    def test_weights_adding_up_to_more_than_256(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('floor_2_w200', 1, 1), ('floor_3_w55', 2, 1)])
        self.assertEqual(errors(problems), [])                          # 1 + 200 + 55: 256
        _, problems = self.collect(CANVAS, REQUIRED + [('floor_2_w200', 1, 1), ('floor_3_w56', 2, 1)])
        self.assertEqual(errors(problems), ['error: "floor": its weights add up to 257, at most 256'])

    def test_warnings(self):
        looks, problems = self.collect(CANVAS, REQUIRED + [('pillar', 1, 1), ('wall', 0, 0), ('Slice 1', 2, 1)])
        self.assertEqual(errors(problems), [])
        self.assertEqual(warnings(problems), [
            'warning: slice "pillar" at (1, 1): "pillar" is not a name the game uses '
            '(assets/tile_names.txt): ignored',
            'warning: slice "wall" at (0, 0): repeats the slice at (0, 0): ignored',
            'warning: slice "Slice 1" at (2, 1): no name: ignored'])
        self.assertNotIn(('pillar', None, None), looks)

    def test_overlapping_rules(self):
        corners = [('wall_xwwx', 1, 1), ('wall_xxww', 2, 1)]
        _, problems = self.collect(CANVAS, REQUIRED + corners)
        self.assertEqual(warnings(problems), ['warning: slices "wall_xwwx" at (1, 1) and "wall_xxww" at (2, 1) '
                                              'both match "xwww": "wall_xwwx" is used'])
        _, problems = self.collect(CANVAS, REQUIRED + corners + [('wall_xwww', 0, 1)])
        self.assertEqual(warnings(problems), [])                        # the T is drawn
        _, problems = self.collect(CANVAS, REQUIRED + [('wall_rwwr', 1, 1), ('wall_xxww', 2, 1)])
        self.assertEqual(warnings(problems), [])                        # rock and wall: never the same cell

    def test_a_tile_with_two_names(self):
        looks, problems = self.collect(CANVAS, REQUIRED + [('wall_rwfw', 2, 1), ('wall_fwrw', 2, 1)])
        self.assertEqual(problems, [])
        self.assertEqual(looks[('wall', 'rwfw', None)].ordered()[0][0], 7)
        self.assertEqual(looks[('wall', 'fwrw', None)].ordered()[0][0], 7)

    def test_without_a_names_file_every_base_is_taken(self):
        looks, problems = self.collect(CANVAS, [('pillar', 1, 1)], names=None)
        self.assertEqual(problems, [])
        self.assertIn(('pillar', None, None), looks)


class TestTables(TempDir, unittest.TestCase):

    def tables(self, slices):
        looks, problems = self.collect(CANVAS, REQUIRED + slices)
        self.assertEqual(errors(problems), [])
        return tile_names.tables(looks, NAMES, [0] * 8)

    def test_want_for_a_code_and_corners(self):
        _, rules, _ = self.tables([('wall_xwfw', 1, 1), ('wall_wwww_xfxx', 2, 1)])
        by_name = {name: want for want, _, _, name in rules}
        X = tile_names.ANY_VALUE
        self.assertEqual(by_name['wall_xwfw'], [X, X, 1, X, 2, X, 1, X])   # N any, E wall, S floor, W wall
        self.assertEqual(by_name['wall_wwww_xfxx'], [1, X, 1, 2, 1, X, 1, X])
        self.assertEqual(by_name['wall'], [X] * 8)

    def test_most_specific_first_by_name_the_default_last(self):
        bases, rules, entries = self.tables([('wall_xxww', 1, 1), ('wall_ffff', 2, 1), ('wall_xwww', 0, 1),
                                             ('wall_xwwx', 3, 0)])
        _, first, count = bases[1]
        self.assertEqual([rules[first + r][3] for r in range(count)],
                         ['wall_ffff', 'wall_xwww', 'wall_xwwx', 'wall_xxww', 'wall'])
        self.assertEqual(entries[rules[first + count - 1][1]][0], 1)     # the default: wall's tile

    def test_a_base_a_terrain_in_the_names_files_order_and_no_tables_for_looks(self):
        bases, rules, entries = self.tables([])
        self.assertEqual([b[0] for b in bases], ['rock', 'wall', 'floor'])
        self.assertNotIn('door_shut', [name for *_, name in entries])

    def test_variants_and_weights(self):
        _, _, entries = self.tables([('floor_3_w7', 1, 1), ('floor_2', 2, 1)])
        floor = [(tile, weight) for tile, _, weight, name in entries if name == 'floor']
        self.assertEqual(floor, [(2, 1), (7, 1), (6, 7)])                # by number: 1, 2, 3


class TestOutput(TempDir, unittest.TestCase):

    def run_import(self, slices, names=NAMES):
        source = self.write(ase_writer.named_tileset_file(TILES, CANVAS, slices))
        out = os.path.join(self.path, 'out')
        return source, out, ase2tiles.run(source, out, names=names)

    def names_file(self):
        path = os.path.join(self.path, 'names.txt')
        with open(path, 'w') as f:
            f.write(NAMES_TEXT)
        return path

    def test_header_and_tables(self):
        _, out, (summary, warns) = self.run_import(REQUIRED + [('wall_xwfw', 1, 1), ('floor_2', 2, 1)])
        self.assertEqual(warns, [])
        with open(os.path.join(out, 'tile_data.h')) as f:
            header = f.read()
        for line in ['#define LOOK_WALL_XWFW                   6', '#define LOOK_WALL_XWFW_BANK              0',
                     '#define LOOK_ROCK                        0', '#define LOOK_FLOOR                       2',
                     '#define LOOK_DOOR_SHUT                   3', '#define TILE_TERRAIN_WALL                1',
                     '#define TILE_TERRAINS                    3', '#define TILE_RULES                       4',
                     'extern const tile_rule_t tile_rules[];']:
            self.assertIn(line, header)
        self.assertIn('#define TILE_ANY                         0xFF', header)
        self.assertNotIn('LOOK_BASE_', header)
        with open(os.path.join(out, 'tile_rules_data.c')) as f:
            rules = f.read()
        self.assertIn('#pragma constseg PAGE_103', rules)
        self.assertIn('    { { TILE_ANY, TILE_ANY, 1, TILE_ANY, 2, TILE_ANY, 1, TILE_ANY }, 1, 1 },   /* wall_xwfw */',
                      rules)
        self.assertIn('    { 1, 2 },   /* wall */', rules)
        self.assertIn('tiles with no name: 4, 5', summary[-1])

    def test_no_names_gives_tables_c_accepts(self):
        source = self.write(ase_writer.tileset_file([EMPTY, solid(1)]))
        out = os.path.join(self.path, 'out')
        ase2tiles.run(source, out)
        with open(os.path.join(out, 'tile_rules_data.c')) as f:
            self.assertIn('{ 0 },   /* none: C has no empty arrays */', f.read())

    def test_every_problem_reported_errors_first_and_nothing_written(self):
        source = self.write(ase_writer.named_tileset_file(TILES, CANVAS, REQUIRED[1:] + [
            ('pillar', 1, 1), ('wall_xwfq', 2, 1), ('wall_xwfw', 0, 0, 20, 16)]))
        out = os.path.join(self.path, 'out')
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(ase2tiles.main([source, out, '--names', self.names_file()]), 1)
        lines = err.getvalue().splitlines()
        self.assertEqual([line.split(':')[1].strip() for line in lines], ['error', 'error', 'error', 'warning'])
        self.assertIn('"xwfq" is not a code', lines[0])
        self.assertIn('larger than one tile', lines[1])
        self.assertIn('no slice named "rock"', lines[2])
        self.assertIn('"pillar" is not a name the game uses', lines[3])
        self.assertFalse(os.path.exists(out))

    def test_warnings_dont_stop_the_import(self):
        source = self.write(ase_writer.named_tileset_file(TILES, CANVAS, REQUIRED + [('pillar', 1, 1)]))
        out = os.path.join(self.path, 'out')
        with contextlib.redirect_stderr(io.StringIO()) as err, contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(ase2tiles.main([source, out, '--names', self.names_file()]), 0)
        self.assertIn('warning: slice "pillar"', err.getvalue())
        self.assertTrue(os.path.exists(os.path.join(out, 'tile_rules_data.c')))


class TestNamesFile(TempDir, unittest.TestCase):

    def names(self, text):
        path = os.path.join(self.path, 'names.txt')
        with open(path, 'w') as f:
            f.write(text)
        return tile_names.read_names(path)

    def test_read(self):
        names = self.names('# the game\nterrain r rock\n\nterrain w wall  # walls\nlook door_shut\n')
        self.assertEqual(names.terrains, [('r', 'rock'), ('w', 'wall')])
        self.assertEqual(names.looks, ['door_shut'])
        self.assertEqual(names.letters, {'r': 0, 'w': 1})

    def test_errors_name_the_line(self):
        for text, message in [('terrain r rock\nbase floor\n', ':2: expected'),
                              ('terrain rock\n', ':1: expected'), ('look door shut\n', ':1: expected'),
                              ('terrain r Rock\n', ':1: "Rock"'),
                              ('terrain r rock\nlook rock\n', ':2: "rock" is listed twice'),
                              ('terrain r rock\nterrain r ridge\n', ':2: the letter "r" is used twice'),
                              ('terrain x rock\n', ':1: "x": a terrain'), ('terrain ro rock\n', ':1: "ro"'),
                              ('terrain 1 rock\n', ':1: "1"')]:
            with self.assertRaisesRegex(tile_names.NamesFileError, message, msg=text):
                self.names(text)


class TestRealTileset(unittest.TestCase):
    """The committed art and names: every slice parses, and every required name is there."""

    def test_tilesheet(self):
        sprite = aseprite.read(os.path.join(ROOT, 'assets', 'Tilesheet.aseprite'))
        names = tile_names.read_names(os.path.join(ROOT, 'assets', 'tile_names.txt'))
        _, problems = tile_names.collect(sprite, names)
        self.assertEqual(errors(problems), [])


if __name__ == '__main__':
    unittest.main()
