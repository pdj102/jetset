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

NAMES = {'rock': True, 'wall': True, 'floor': True, 'floor_lip': False, 'door_shut': True}


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
        self.assertEqual(tile_names.parse('wall'), ('wall', None, None, None, 1))
        self.assertEqual(tile_names.parse('wall_xxox'), ('wall', 'xxox', None, None, 1))
        self.assertEqual(tile_names.parse('wall_xxxx_se'), ('wall', 'xxxx', 'se', None, 1))
        self.assertEqual(tile_names.parse('floor_2'), ('floor', None, None, 2, 1))
        self.assertEqual(tile_names.parse('floor_1_w7'), ('floor', None, None, 1, 7))
        self.assertEqual(tile_names.parse('floor_lip_ooxo_3_w2'), ('floor_lip', 'ooxo', None, 3, 2))
        self.assertEqual(tile_names.parse('door_shut'), ('door_shut', None, None, None, 1))

    def test_errors(self):
        for name, text in [('Wall', 'lower case'), ('wall__x', 'empty part'), ('wall_', 'empty part'),
                           ('wall_se', 'needs a code'), ('floor_w0', 'weight 0'), ('floor_w256', 'weight 256')]:
            with self.assertRaisesRegex(ValueError, text, msg=name):
                tile_names.parse(name)


class TestChecks(TempDir, unittest.TestCase):

    def test_the_required_names_pass(self):
        looks, problems = self.collect(CANVAS, REQUIRED)
        self.assertEqual(problems, [])
        self.assertEqual(looks[('rock', None, None)].ordered()[0][0], 0)   # tile 0 named

    def test_a_required_name_missing(self):
        _, problems = self.collect(CANVAS, REQUIRED[1:])
        self.assertEqual(errors(problems), ['error: no slice named "rock": a look the game needs '
                                            '(assets/tile_names.txt)'])

    def test_slices_the_wrong_size_or_place(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall_xxox', 0, 0, 20, 16), ('wall_oxxx', 8, 0, 16, 16),
                                                       ('wall_xoxx', 60, 0, 8, 8)])
        self.assertEqual(errors(problems), [
            'error: slice "wall_xxox" at (0, 0): 20 x 16, larger than one tile (16 x 16)',
            'error: slice "wall_oxxx" at (0, 0): crosses from one tile into the next',
            'error: slice "wall_xoxx" at (3, 0): off the canvas'])

    def test_a_mistyped_code_of_a_known_base(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall_0x0x', 1, 1)])
        self.assertEqual(errors(problems), ['error: slice "wall_0x0x" at (1, 1): "0x0x" is not a code, a '
                                            'diagonal, a variant or a weight of "wall"'])

    def test_the_longest_known_base(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('floor_lip_oxox_q', 1, 1)])
        self.assertIn('"q" is not a code, a diagonal, a variant or a weight of "floor_lip"', errors(problems)[0])

    def test_a_name_on_two_tiles(self):
        _, problems = self.collect(CANVAS, REQUIRED + [('wall', 1, 1)])
        self.assertEqual(errors(problems), ['error: slice "wall" at (1, 1): the same name is on tile 1 at (0, 0)'])

    def test_warnings(self):
        looks, problems = self.collect(CANVAS, REQUIRED + [('pillar', 1, 1), ('wall', 0, 0), ('Slice 1', 2, 1)])
        self.assertEqual(errors(problems), [])
        self.assertEqual(warnings(problems), [
            'warning: slice "pillar" at (1, 1): "pillar" is not a name the game uses '
            '(assets/tile_names.txt): ignored',
            'warning: slice "wall" at (0, 0): repeats the slice at (0, 0): ignored',
            'warning: slice "Slice 1" at (2, 1): no name: ignored'])
        self.assertNotIn(('pillar', None, None), looks)

    def test_a_tile_with_two_names(self):
        looks, problems = self.collect(CANVAS, REQUIRED + [('wall_xoxx', 2, 1), ('wall_xxxo', 2, 1)])
        self.assertEqual(problems, [])
        self.assertEqual(looks[('wall', 'xoxx', None)].ordered()[0][0], 7)
        self.assertEqual(looks[('wall', 'xxxo', None)].ordered()[0][0], 7)

    def test_without_a_names_file_every_base_is_taken(self):
        looks, problems = self.collect(CANVAS, [('pillar', 1, 1)], names=None)
        self.assertEqual(problems, [])
        self.assertIn(('pillar', None, None), looks)


class TestTables(TempDir, unittest.TestCase):

    def tables(self, slices):
        looks, problems = self.collect(CANVAS, REQUIRED + slices)
        self.assertEqual(errors(problems), [])
        return tile_names.tables(looks, NAMES, [0] * 8)

    def test_codes_care_and_want(self):
        _, rules, _ = self.tables([('wall_xxox', 1, 1), ('wall_xxxx_se', 2, 1)])
        by_name = {name: (care, want) for care, want, _, _, name in rules}
        self.assertEqual(by_name['wall_xxox'], (0x55, 0x45))             # N, E, W join; S doesn't
        self.assertEqual(by_name['wall_xxxx_se'], (0x5D, 0x55))          # and SE doesn't

    def test_diagonals_first_then_the_fallback(self):
        bases, rules, entries = self.tables([('wall_xxox', 1, 1), ('wall_xxxx_se', 2, 1), ('wall_xxxx_nw', 0, 1)])
        wall = [b for b in bases if b[0] == 'wall'][0]
        _, first, count, fallback, fallback_count = wall
        self.assertEqual([rules[first + r][4] for r in range(count)], ['wall_xxxx_nw', 'wall_xxxx_se', 'wall_xxox'])
        self.assertEqual((entries[fallback][0], fallback_count), (1, 1))

    def test_bases_in_the_names_files_order(self):
        bases, _, _ = self.tables([('floor_lip_ooxo', 1, 1)])
        self.assertEqual([b[0] for b in bases], ['rock', 'wall', 'floor', 'floor_lip', 'door_shut'])

    def test_variants_and_weights(self):
        _, _, entries = self.tables([('floor_1_w7', 1, 1), ('floor_2', 2, 1)])
        floor = [(tile, weight) for tile, _, weight, name in entries if name == 'floor']
        self.assertEqual(floor, [(2, 1), (6, 7), (7, 1)])                # unnumbered first, then by number


class TestOutput(TempDir, unittest.TestCase):

    def run_import(self, slices, names=NAMES):
        source = self.write(ase_writer.named_tileset_file(TILES, CANVAS, slices))
        out = os.path.join(self.path, 'out')
        return source, out, ase2tiles.run(source, out, names=names)

    def test_header_and_tables(self):
        _, out, (summary, warns) = self.run_import(REQUIRED + [('wall_xxox', 1, 1), ('floor_2', 2, 1)])
        self.assertEqual(warns, [])
        with open(os.path.join(out, 'tile_data.h')) as f:
            header = f.read()
        for line in ['#define LOOK_WALL_XXOX                   6', '#define LOOK_WALL_XXOX_BANK              0',
                     '#define LOOK_ROCK                        0', '#define LOOK_FLOOR                       2',
                     '#define LOOK_BASE_WALL                   1', '#define TILE_RULES                       1',
                     'extern const tile_rule_t tile_rules[];']:
            self.assertIn(line, header)
        with open(os.path.join(out, 'tile_rules_data.c')) as f:
            rules = f.read()
        self.assertIn('#pragma constseg PAGE_103', rules)
        self.assertIn('    { 0x55, 0x45, 1, 1 },   /* wall_xxox */', rules)
        self.assertIn('tiles with no name: 4, 5', summary[-1])

    def test_no_names_gives_tables_c_accepts(self):
        source = self.write(ase_writer.tileset_file([EMPTY, solid(1)]))
        out = os.path.join(self.path, 'out')
        ase2tiles.run(source, out)
        with open(os.path.join(out, 'tile_rules_data.c')) as f:
            self.assertIn('{ 0 },   /* none: C has no empty arrays */', f.read())

    def test_every_problem_reported_errors_first_and_nothing_written(self):
        source = self.write(ase_writer.named_tileset_file(TILES, CANVAS, REQUIRED[1:] + [
            ('pillar', 1, 1), ('wall_0x0x', 2, 1), ('wall_xxox', 0, 0, 20, 16)]))
        out = os.path.join(self.path, 'out')
        names = os.path.join(self.path, 'names.txt')
        with open(names, 'w') as f:
            f.write('base rock required\nbase wall required\nbase floor required\nlook door_shut required\n')
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(ase2tiles.main([source, out, '--names', names]), 1)
        lines = err.getvalue().splitlines()
        self.assertEqual([line.split(':')[1].strip() for line in lines], ['error', 'error', 'error', 'warning'])
        self.assertIn('"0x0x" is not a code', lines[0])
        self.assertIn('larger than one tile', lines[1])
        self.assertIn('no slice named "rock"', lines[2])
        self.assertIn('"pillar" is not a name the game uses', lines[3])
        self.assertFalse(os.path.exists(out))

    def test_warnings_dont_stop_the_import(self):
        source = self.write(ase_writer.named_tileset_file(TILES, CANVAS, REQUIRED + [('pillar', 1, 1)]))
        out = os.path.join(self.path, 'out')
        names = os.path.join(self.path, 'names.txt')
        with open(names, 'w') as f:
            f.write('base rock required\nbase wall required\nbase floor required\nlook door_shut required\n')
        with contextlib.redirect_stderr(io.StringIO()) as err, contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(ase2tiles.main([source, out, '--names', names]), 0)
        self.assertIn('warning: slice "pillar"', err.getvalue())
        self.assertTrue(os.path.exists(os.path.join(out, 'tile_rules_data.c')))


class TestNamesFile(TempDir, unittest.TestCase):

    def names(self, text):
        path = os.path.join(self.path, 'names.txt')
        with open(path, 'w') as f:
            f.write(text)
        return tile_names.read_names(path)

    def test_read(self):
        self.assertEqual(self.names('# the game\nbase wall required\n\nbase floor_lip  # optional\nlook door_shut '
                                    'required\n'),
                         {'wall': True, 'floor_lip': False, 'door_shut': True})

    def test_errors_name_the_line(self):
        for text, message in [('base wall\nbases floor\n', ':2: expected'), ('base wall needed\n', ':1: expected'),
                              ('base Wall\n', ':1: "Wall"'), ('base wall\nlook wall\n', ':2: "wall" is listed twice')]:
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
