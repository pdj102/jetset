"""
Tests for the sprite importer (scripts/ase2sprites.py, the frame reading in
aseprite.py, and the starter palette, sprite_palette.py;
docs/sprite_import.md section 7). Run by make test, or:

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

import ase2sprites     # noqa: E402
import aseprite        # noqa: E402
import ase_writer      # noqa: E402
import next_colour     # noqa: E402
import sprite_palette  # noqa: E402


def image(fill=0, size=16, pixels=()):
    """Rows of palette indices filled with one, with (x, y, index) changes."""
    rows = [[fill] * size for _ in range(size)]
    for x, y, p in pixels:
        rows[y][x] = p
    return rows


def dot(index, x=0, y=0):
    """A 16 x 16 image, transparent but for one pixel."""
    return image(pixels=[(x, y, index)])


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

    def read(self, data):
        return aseprite.read(self.write(data))

    def convert(self, data):
        return ase2sprites.convert(self.read(data))


class TestReadingFrames(TempDir, unittest.TestCase):

    def test_frames_and_durations(self):
        sprite = self.read(ase_writer.sprite_file([dot(1), dot(2)], [('a', 0, 1)], durations=[500, 40]))
        self.assertEqual([f.duration for f in sprite.frames], [500, 40])
        self.assertEqual(sprite.frame_indices(1)[0][0], 2)
        self.assertIsNone(sprite.frame_indices(1)[0][1])

    def test_linked_cel_shows_its_frame(self):
        frames = [dot(3, 4, 5), {0: ase_writer.cel_chunk(0, None, link=0)}]
        sprite = self.read(ase_writer.sprite_file(frames, [('a', 0, 1)]))
        self.assertEqual(sprite.frame_indices(1), sprite.frame_indices(0))
        self.assertEqual(sprite.frame_indices(1)[5][4], 3)

    def test_raw_cels_too(self):
        frames = [{0: ase_writer.cel_chunk(0, dot(5, 1, 1), kind=0)}]
        self.assertEqual(self.read(ase_writer.sprite_file(frames, [('a', 0, 0)])).frame_indices(0)[1][1], 5)

    def test_layers_combine_bottom_to_top(self):
        layers = [ase_writer.layer_chunk('under'), ase_writer.layer_chunk('over')]
        frames = [{0: image(1), 1: dot(2, 3, 3)}]
        rows = self.read(ase_writer.sprite_file(frames, [('a', 0, 0)], layers=layers)).frame_indices(0)
        self.assertEqual(rows[3][3], 2)    # covered
        self.assertEqual(rows[0][0], 1)    # the upper layer's transparent pixels show the lower

    def test_hidden_reference_and_hidden_group_layers_left_out(self):
        layers = [ase_writer.layer_chunk('shown'),
                  ase_writer.layer_chunk('hidden', flags=0),
                  ase_writer.layer_chunk('reference', flags=1 | 64),
                  ase_writer.layer_chunk('group', kind=1, flags=0),
                  ase_writer.layer_chunk('in hidden group', level=1),
                  ase_writer.layer_chunk('group 2', kind=1),
                  ase_writer.layer_chunk('in shown group', level=1)]
        frames = [{0: dot(1, 0), 1: dot(2, 1), 2: dot(3, 2), 4: dot(4, 3), 6: dot(5, 4)}]
        rows = self.read(ase_writer.sprite_file(frames, [('a', 0, 0)], layers=layers)).frame_indices(0)
        self.assertEqual(rows[0][:5], [1, None, None, None, 5])

    def test_cels_off_the_canvas_are_clipped(self):
        cel = ase_writer.cel_chunk(0, image(7, size=8), x=12, y=-4)
        rows = self.read(ase_writer.sprite_file([{0: cel}], [('a', 0, 0)])).frame_indices(0)
        self.assertEqual(rows[0][11:], [None, 7, 7, 7, 7])
        self.assertEqual(rows[3][12], 7)
        self.assertIsNone(rows[4][12])

    def test_tags(self):
        sprite = self.read(ase_writer.sprite_file([dot(1)] * 3, [('a', 0, 1), ('b.c', 1, 2, 2)]))
        self.assertEqual([(t.name, t.first, t.last, t.direction) for t in sprite.tags],
                         [('a', 0, 1, 0), ('b.c', 1, 2, 2)])

    def test_z_index_is_an_error(self):
        frames = [{0: ase_writer.cel_chunk(0, dot(1), z_index=1)}]
        with self.assertRaisesRegex(aseprite.AsepriteError, 'z-index'):
            self.read(ase_writer.sprite_file(frames, [('a', 0, 0)]))


class TestRules(TempDir, unittest.TestCase):

    def error(self, data, pattern):
        with self.assertRaisesRegex(ase2sprites.SpriteError, pattern):
            self.convert(data)

    def test_a_frame_in_no_tag(self):
        self.error(ase_writer.sprite_file([dot(1)] * 3, [('a', 0, 0), ('b', 2, 2)]), 'frame 1 is in no tag')

    def test_a_frame_over_two_rows(self):
        frames = [image(pixels=[(0, 0, 1), (1, 0, 17)])]
        self.error(ase_writer.sprite_file(frames, [('a', 0, 0)]), 'frame 0 uses palette rows 0, 1')

    def test_a_tag_over_two_rows(self):
        self.error(ase_writer.sprite_file([dot(1), dot(18)], [('a', 0, 1)]), 'tag "a": .*rows 0, 1')

    def test_two_durations_in_a_tag(self):
        self.error(ase_writer.sprite_file([dot(1)] * 2, [('a', 0, 1)], durations=[100, 200]),
                   'tag "a": .*different durations \\(100, 200 ms\\)')

    def test_only_forward(self):
        self.error(ase_writer.sprite_file([dot(1)], [('a', 0, 0, 2)]), 'tag "a": plays ping-pong')

    def test_names(self):
        self.error(ase_writer.sprite_file([dot(1)], [('rat!', 0, 0)]), 'tag "rat!": not a C name')
        self.error(ase_writer.sprite_file([dot(1)], [('rat/walk', 0, 0)]), 'not a C name')
        self.error(ase_writer.sprite_file([dot(1)] * 2, [('rat.walk', 0, 0), ('rat walk', 1, 1)]),
                   'tag "rat walk": its name SPRITE_RAT_WALK is also tag "rat.walk"')
        self.error(ase_writer.sprite_file([dot(1)] * 2, [('rat', 0, 0), ('rat.count', 1, 1)]),
                   'SPRITE_RAT_COUNT is also tag "rat"')
        self.error(ase_writer.sprite_file([dot(1)], [('patterns', 0, 0)]), 'one the header uses')

    def test_129_frames(self):
        self.error(ase_writer.sprite_file([dot(1)] * 129, [('a', 0, 128)]), '129 frames, at most 128')

    def test_canvas_16_by_16(self):
        self.error(ase_writer.sprite_file([image(1, 32)], [('a', 0, 0)], width=32, height=32), '32 x 32')

    def test_indexed_only(self):
        self.error(ase_writer.aseprite_file([ase_writer.layer_chunk('Layer 1')]), 'indexed colour mode')

    def test_transparent_index_0(self):
        data = ase_writer.aseprite_frames([(100, [ase_writer.layer_chunk('L')])], ase_writer.INDEXED, 5)
        self.error(data, 'transparent colour is index 5')

    def test_opacity(self):
        layers = [ase_writer.layer_chunk('half', opacity=128)]
        self.error(ase_writer.sprite_file([dot(1)], [('a', 0, 0)], layers=layers), 'layer "half": opacity 128')
        frames = [{0: ase_writer.cel_chunk(0, dot(1), opacity=10)}]
        self.error(ase_writer.sprite_file(frames, [('a', 0, 0)]), 'frame 0, layer "Layer 1": cel opacity 10')

    def test_tilemap_layer(self):
        layers = [ase_writer.layer_chunk('map', kind=2)]
        self.error(ase_writer.sprite_file([{}], [('a', 0, 0)], layers=layers), 'tilemap layer')

    def test_palette_colour_not_next(self):
        palette = [(0, 0, 0, 255), (37, 0, 0, 255)]
        self.error(ase_writer.sprite_file([dot(1)], [('a', 0, 0)], palette=palette),
                   r'palette entry 1 \(bank 0, slot 1\): colour \(37, 0, 0\)')

    def test_slot_0_painted(self):
        self.error(ase_writer.sprite_file([dot(16, 2, 3)], [('a', 0, 0)]),
                   r'frame 0, pixel \(2, 3\): palette index 16 is slot 0')

    def test_index_past_the_palette(self):
        self.error(ase_writer.sprite_file([dot(70)], [('a', 0, 0)]), 'index 70 past the palette')


class TestOutput(TempDir, unittest.TestCase):

    def test_a_tags_run(self):
        frames = [dot(1)] + [dot(33)] * 3 + [dot(2)]
        tags = [('potion', 0, 0), ('rat.walk', 1, 3), ('blink', 4, 4)]
        _, rows, runs, _ = self.convert(ase_writer.sprite_file(frames, tags,
                                                               durations=[10, 500, 500, 500, 30]))
        by_name = {run.name: run for run in runs}
        walk = by_name['SPRITE_RAT_WALK']
        self.assertEqual((walk.first, walk.count, walk.rate, walk.bank), (1, 3, 25, 2))
        self.assertEqual(by_name['SPRITE_POTION'].rate, 1)     # 10 ms: at least a frame
        self.assertEqual(by_name['SPRITE_BLINK'].rate, 2)      # 30 ms: rounded up
        self.assertEqual(rows, [0, 2, 2, 2, 0])

    def test_overlapping_tags_and_an_empty_frame(self):
        frames = [dot(49), image()]
        _, rows, runs, _ = self.convert(ase_writer.sprite_file(frames, [('all', 0, 1), ('end', 1, 1)]))
        self.assertEqual(rows, [3, None])
        self.assertEqual([(r.name, r.first, r.count, r.bank) for r in runs],
                         [('SPRITE_ALL', 0, 2, 3), ('SPRITE_END', 1, 1, 0)])

    def test_pixels_are_slots_left_pixel_high(self):
        patterns, _, _, _ = self.convert(ase_writer.sprite_file([image(pixels=[(0, 0, 18), (1, 0, 21)])],
                                                                [('a', 0, 0)]))
        data = ase2sprites.pattern_bytes(patterns[0])
        self.assertEqual(len(data), 128)
        self.assertEqual(data[0], 0x25)
        self.assertEqual(data[1:], [0] * 127)

    def test_64_patterns_a_page(self):
        self.assertEqual(ase2sprites.pages(64), [(137, 0, 64)])
        self.assertEqual(ase2sprites.pages(65), [(137, 0, 64), (139, 64, 1)])
        self.assertEqual(ase2sprites.pages(128), [(137, 0, 64), (139, 64, 64)])

    def test_two_pages_written(self):
        data = ase_writer.sprite_file([dot(1)] * 65, [('a', 0, 64)])
        out = os.path.join(self.path, 'out')
        ase2sprites.run(self.write(data), out)
        with open(os.path.join(out, 'sprite_patterns_data.asm')) as f:
            text = f.read()
        self.assertIn('SECTION PAGE_137', text)
        self.assertIn('SECTION PAGE_139', text)
        self.assertEqual(text.count('; pattern '), 65)
        self.assertIn('\tdb $10, $00, $00, $00, $00, $00, $00, $00  ; 1...............', text)
        with open(os.path.join(out, 'sprite_data.h')) as f:
            text = f.read()
        self.assertIn('#define SPRITE_PATTERNS_COUNT_1      1', text)
        self.assertIn('extern const uint8_t sprite_patterns_1[];', text)

    def test_the_palette_banks(self):
        palette = [(0, 0, 0, 255), (255, 255, 255, 255)] + [(0, 0, 255, 255)] * 16
        _, _, _, banks = self.convert(ase_writer.sprite_file([dot(1)], [('a', 0, 0)], palette=palette))
        self.assertEqual(len(banks), 2)
        self.assertEqual(banks[0][:2], [0, next_colour.to_next((255, 255, 255))])
        self.assertEqual(banks[1][0], 0)                          # slot 0: transparent, black
        self.assertEqual(banks[1][1], next_colour.to_next((0, 0, 255)))
        self.assertEqual(banks[1][2:], [0] * 14)                  # past the palette: black
        text = ase2sprites.palette_asm('x', banks)
        self.assertIn('SECTION PAGE_135', text)
        self.assertIn('\tdb $00, $00, $FF, $01, $03, $01', text)

    def test_header(self):
        frames = [dot(1)] * 2 + [dot(33)]
        _, _, runs, banks = self.convert(ase_writer.sprite_file(frames, [('rat.walk', 0, 1), ('ring', 2, 2)],
                                                                durations=[500, 500, 100]))
        text = ase2sprites.header('x', [None] * 3, runs, banks)
        for line in ['#define SPRITE_RAT_WALK              0', '#define SPRITE_RAT_WALK_COUNT        2',
                     '#define SPRITE_RAT_WALK_RATE         25', '#define SPRITE_RING_BANK             2',
                     '#define SPRITE_PALETTE_ENTRIES       64', '#define SPRITE_PATTERNS              3']:
            self.assertIn(line, text)

    def test_nothing_written_on_an_error(self):
        source = self.write(ase_writer.sprite_file([dot(1)] * 2, [('a', 0, 0)]))
        out = os.path.join(self.path, 'out')
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(ase2sprites.main([source, out]), 1)
        self.assertIn('frame 1 is in no tag', err.getvalue())
        self.assertFalse(os.path.exists(out) and os.listdir(out))


class TestStarterPalette(unittest.TestCase):

    def entries(self):
        with open(os.path.join(ROOT, 'assets', 'sprite_palette.gpl')) as f:
            lines = [line for line in f.read().splitlines()[3:] if not line.startswith('#')]
        return [tuple(int(v) for v in line.split('\t')[0].split()) for line in lines]

    def test_committed_palette_is_the_scripts(self):
        with open(os.path.join(ROOT, 'assets', 'sprite_palette.gpl')) as f:
            self.assertEqual(f.read(), sprite_palette.gpl())

    def test_the_spec(self):
        entries = self.entries()
        self.assertEqual(len(entries), 256)
        self.assertTrue(all(next_colour.to_next(e) is not None for e in entries))
        banks = [entries[16 * b:16 * b + 16] for b in range(16)]
        self.assertEqual(len(set(banks[1][1:])), 1)     # flash
        self.assertEqual(len(set(banks[2][1:])), 1)     # shadow
        for b in [0] + list(range(3, 16)):
            self.assertEqual(banks[b][1:9], banks[3][1:9], f'bank {b}')


if __name__ == '__main__':
    unittest.main()
