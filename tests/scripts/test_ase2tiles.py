"""
Tests for the tileset importer (scripts/ase2tiles.py, aseprite.py,
next_colour.py; docs/tileset_import.md section 7). Run by make test, or:

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
import next_colour   # noqa: E402

CLEAR = (0, 0, 0, 0)
BLACK = (0, 0, 0, 255)
WHITE = (255, 255, 255, 255)
RED = (255, 0, 0, 255)
GREEN = (0, 255, 0, 255)
BLUE = (0, 0, 255, 255)


def next_rgb(level):
    """An opaque grey of a 3-bit level: a distinct Next colour for each 0-7."""
    v = next_colour.LEVELS[level]
    return (v, v, v, 255)


def colour_n(n):
    """The nth of the 512 Next colours, opaque."""
    return (next_colour.LEVELS[n >> 6], next_colour.LEVELS[(n >> 3) & 7], next_colour.LEVELS[n & 7], 255)


def tile(fill=CLEAR, size=16, pixels=()):
    """A tile filled with one pixel, with (x, y, pixel) changes."""
    rows = [[fill] * size for _ in range(size)]
    for x, y, p in pixels:
        rows[y][x] = p
    return rows


def tile_of(colours):
    """A tile using each of colours once, from the top left, the rest transparent."""
    return tile(pixels=[(i % 16, i // 16, c) for i, c in enumerate(colours)])


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


class TestReading(TempDir, unittest.TestCase):

    def read(self, data):
        return aseprite.read(self.write(data)).tilemap_tileset()

    def test_rgba_and_indexed_give_the_same_tiles(self):
        palette = [BLACK, WHITE, RED, (9, 9, 9, 255)]
        indexed = [tile(3), tile(3, pixels=[(0, 0, 1), (5, 7, 2), (15, 15, 0)])]
        rgba = [tile(CLEAR), tile(CLEAR, pixels=[(0, 0, WHITE), (5, 7, RED), (15, 15, BLACK)])]
        from_indexed = self.read(ase_writer.tileset_file(indexed, ase_writer.INDEXED, palette,
                                                         transparent_index=3))
        from_rgba = self.read(ase_writer.tileset_file(rgba))
        self.assertEqual(from_indexed.tiles, from_rgba.tiles)
        self.assertEqual(from_rgba.tiles[1][7][5], RED)

    def test_old_palette_used_without_a_new_one(self):
        chunks = [ase_writer.old_palette_chunk([(0, 0, 0), (255, 0, 0)]),
                  ase_writer.layer_chunk('Tilemap 1', kind=2),
                  ase_writer.tileset_chunk([tile(0), tile(1)], ase_writer.INDEXED)]
        tiles = self.read(ase_writer.aseprite_file(chunks, ase_writer.INDEXED, transparent_index=0)).tiles
        self.assertEqual(tiles[0][0][0], CLEAR)
        self.assertEqual(tiles[1][0][0], RED)

    def test_alpha_0_is_transparent(self):
        tiles = self.read(ase_writer.tileset_file([tile((10, 20, 30, 0))])).tiles
        self.assertEqual(tiles[0][3][3], CLEAR)

    def test_other_chunks_skipped(self):
        data = ase_writer.tileset_file([tile(), tile(RED)],
                                       extra=[ase_writer.other_chunk(), ase_writer.other_chunk(0x2005, 37)])
        self.assertEqual(self.read(data).tiles[1][0][0], RED)

    def test_part_transparent_alpha_is_an_error(self):
        with self.assertRaisesRegex(aseprite.AsepriteError, r'tile 1, pixel \(2, 4\): alpha 128'):
            self.read(ase_writer.tileset_file([tile(), tile(pixels=[(2, 4, (255, 0, 0, 128))])]))

    def test_grayscale_is_an_error(self):
        with self.assertRaisesRegex(aseprite.AsepriteError, 'grayscale'):
            self.read(ase_writer.tileset_file([tile((0, 0))], ase_writer.GRAYSCALE))

    def test_external_tileset_is_an_error(self):
        chunks = [ase_writer.layer_chunk('Tilemap 1', kind=2),
                  ase_writer.tileset_chunk([tile()], ase_writer.RGBA, flags=1)]
        with self.assertRaisesRegex(aseprite.AsepriteError, 'external'):
            self.read(ase_writer.aseprite_file(chunks))

    def test_not_an_aseprite_file(self):
        with self.assertRaisesRegex(aseprite.AsepriteError, 'magic'):
            self.read(bytes(200))

    def test_the_tilemap_layer_chosen(self):
        chunks = [ase_writer.layer_chunk('A', kind=2, tileset_index=0),
                  ase_writer.layer_chunk('B', kind=2, tileset_index=1),
                  ase_writer.tileset_chunk([tile()], ase_writer.RGBA, index=0),
                  ase_writer.tileset_chunk([tile(), tile()], ase_writer.RGBA, index=1)]
        sprite = aseprite.read(self.write(ase_writer.aseprite_file(chunks)))
        with self.assertRaisesRegex(aseprite.AsepriteError, 'several tilemap layers'):
            sprite.tilemap_tileset()
        self.assertEqual(len(sprite.tilemap_tileset('B').tiles), 2)
        with self.assertRaisesRegex(aseprite.AsepriteError, 'no tilemap layer named "C"'):
            sprite.tilemap_tileset('C')

    def test_no_tilemap_layer(self):
        sprite = aseprite.read(self.write(ase_writer.aseprite_file([ase_writer.layer_chunk('Background')])))
        with self.assertRaisesRegex(aseprite.AsepriteError, 'no tilemap layer'):
            sprite.tilemap_tileset()


class TestColours(unittest.TestCase):

    def test_levels_convert_exactly(self):
        for level, v in enumerate(next_colour.LEVELS):
            self.assertEqual(next_colour.to_next((v, 0, 0)), level << 6)
            self.assertEqual(next_colour.to_next((0, v, 0)), level << 3)
            self.assertEqual(next_colour.to_next((0, 0, v)), level)

    def test_a_colour_between_levels_is_not_a_next_colour(self):
        self.assertIsNone(next_colour.to_next((37, 0, 0)))
        self.assertIsNone(next_colour.to_next((0, 0, 254)))

    def test_entry_bytes(self):
        self.assertEqual(next_colour.entry_bytes(next_colour.to_next(BLACK)), (0x00, 0))
        self.assertEqual(next_colour.entry_bytes(next_colour.to_next(WHITE)), (0xFF, 1))
        self.assertEqual(next_colour.entry_bytes(next_colour.to_next(RED)), (0xE0, 0))
        self.assertEqual(next_colour.entry_bytes(next_colour.to_next(GREEN)), (0x1C, 0))
        self.assertEqual(next_colour.entry_bytes(next_colour.to_next(BLUE)), (0x03, 1))
        self.assertEqual(next_colour.entry_bytes(next_colour.to_next((0, 0, 182))), (0x02, 1))

    def test_not_a_next_colour_named(self):
        tileset = aseprite.Tileset(0, 'T', 16, 16, [tile(), tile(pixels=[(3, 9, (37, 0, 0, 255))])])
        with self.assertRaisesRegex(ase2tiles.TileError, r'tile 1, pixel \(3, 9\): colour \(37, 0, 0\)'):
            ase2tiles.convert(tileset)


class TestBanks(unittest.TestCase):

    def test_graphics_that_fit_share_a_bank(self):
        banks, assigned = next_colour.fit_banks([[], [1, 2, 3], [3, 4]])
        self.assertEqual(assigned, [0, 0, 0])
        self.assertEqual(banks, [[1, 2, 3, 4]])

    def test_one_that_does_not_fit_opens_a_bank(self):
        banks, assigned = next_colour.fit_banks([list(range(10)), list(range(10, 20)), [5, 25]])
        self.assertEqual(assigned, [0, 1, 0])
        self.assertEqual(banks[0], list(range(10)) + [25])

    def test_15_colours_fit_16_do_not(self):
        next_colour.fit_banks([list(range(15))])
        with self.assertRaisesRegex(next_colour.ColourError, 'tile 1: 16 colours'):
            next_colour.fit_banks([[], list(range(16))])

    def test_a_17th_bank_is_an_error(self):
        graphics = [list(range(15 * n, 15 * n + 15)) for n in range(17)]
        next_colour.fit_banks(graphics[:16])
        with self.assertRaisesRegex(next_colour.ColourError, 'tile 16: .*all 16'):
            next_colour.fit_banks(graphics)

    def test_adding_at_the_end_changes_nothing_before(self):
        tiles = [tile(), tile_of([next_rgb(1), next_rgb(2)]), tile_of([colour_n(n) for n in range(100, 114)]),
                 tile_of([next_rgb(2), next_rgb(3)])]
        more = tiles + [tile_of([next_rgb(4), colour_n(100)])]
        before = ase2tiles.convert(aseprite.Tileset(0, 'T', 16, 16, tiles))
        after = ase2tiles.convert(aseprite.Tileset(0, 'T', 16, 16, more))
        self.assertEqual(after[1][:4], before[1])
        self.assertEqual(after[2][:4], before[2])
        for b, bank in enumerate(before[0]):
            self.assertEqual(after[0][b][:len(bank)], bank)

    def test_pixel_values_index_the_bank(self):
        tiles = [tile(), tile(pixels=[(0, 0, RED), (1, 0, BLUE), (2, 0, RED)])]
        banks, assigned, pixels = ase2tiles.convert(aseprite.Tileset(0, 'T', 16, 16, tiles))
        self.assertEqual(pixels[1][0][:4], [1, 2, 1, 0])
        self.assertEqual(banks[assigned[1]], [next_colour.to_next(RED), next_colour.to_next(BLUE)])


class TestOutput(TempDir, unittest.TestCase):

    def test_hardware_tiles_in_order_left_pixel_high(self):
        metatile = [[(x // 8) + 2 * (y // 8) + 1 for x in range(16)] for y in range(16)]
        metatile[0][0] = 9
        tiles = ase2tiles.hardware_tiles(metatile)
        self.assertEqual([t[1] for t in tiles], [0x11, 0x22, 0x33, 0x44])
        self.assertEqual(tiles[0][0], 0x91)
        self.assertTrue(all(len(t) == 32 for t in tiles))

    def test_padded_to_64_metatiles(self):
        text = ase2tiles.defs_asm('x', [0, 0], [tile(0), tile(1)])
        self.assertIn('; metatile 1: bank 0', text)
        self.assertIn('; metatile 63: unused (transparent)', text)
        self.assertIn('sprite255:', text)
        self.assertNotIn('sprite256:', text)
        self.assertEqual(text.count('\tdb $11, $11'), 8)

    def test_more_than_64_is_an_error(self):
        with self.assertRaisesRegex(ase2tiles.TileError, '65 tiles, at most 64'):
            ase2tiles.convert(aseprite.Tileset(0, 'T', 16, 16, [tile()] * 65))

    def test_tile_0_must_be_transparent(self):
        with self.assertRaisesRegex(ase2tiles.TileError, 'tile 0 is not fully transparent'):
            ase2tiles.convert(aseprite.Tileset(0, 'T', 16, 16, [tile(pixels=[(1, 1, RED)])]))

    def test_tiles_must_be_16_by_16(self):
        with self.assertRaisesRegex(ase2tiles.TileError, '8 x 8'):
            ase2tiles.convert(aseprite.Tileset(0, 'T', 8, 8, [tile(size=8)]))

    def test_palette_entry_0_transparent_and_bank_padded(self):
        text = ase2tiles.palette_asm('x', [[next_colour.to_next(WHITE)]], [0, 0])
        self.assertIn('; bank 0: metatiles 0, 1', text)
        self.assertIn('\tdb $00, $00, $FF, $01, $00, $00', text)
        self.assertEqual(text.count('\tdb '), 2)

    def test_header(self):
        text = ase2tiles.header('x', [[1], [2]], [0, 0, 1])
        self.assertIn('#define TILE_PALETTE_BANKS  2', text)
        self.assertIn('#define TILE_PALETTE_ENTRIES 32', text)
        self.assertIn('#define TILE_METATILES      3', text)
        self.assertIn('#define TILE_BANK_2  1', text)

    def test_files_written(self):
        source = self.write(ase_writer.tileset_file([tile(), tile(RED)]))
        out = os.path.join(self.path, 'out')
        summary = ase2tiles.run(source, out)
        self.assertEqual(sorted(os.listdir(out)), ['tile_data.h', 'tile_defs_data.asm', 'tile_palette_data.asm'])
        self.assertIn('2 metatiles, 1 of 16 banks', summary[0])

    def test_nothing_written_on_an_error(self):
        source = self.write(ase_writer.tileset_file([tile(), tile_of([colour_n(n) for n in range(16)])]))
        out = os.path.join(self.path, 'out')
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(ase2tiles.main([source, out]), 1)
        self.assertIn('tile 1: 16 colours', err.getvalue())
        self.assertFalse(os.path.exists(out) and os.listdir(out))


class TestRealFile(unittest.TestCase):
    """The committed art: only what holds as it changes. Catches a misreading of
    the format that a file written by ase_writer would share."""

    def test_tilesheet(self):
        tileset = aseprite.read(os.path.join(ROOT, 'assets', 'Tilesheet.aseprite')).tilemap_tileset()
        self.assertEqual((tileset.width, tileset.height), (16, 16))
        self.assertTrue(all(p == CLEAR for row in tileset.tiles[0] for p in row))
        ase2tiles.convert(tileset)


if __name__ == '__main__':
    unittest.main()
