"""
Tests for the sprite grid tool (scripts/sprite_grids.py, docs/sprite_style.md
section 7). Run by make test, or:

    python3 -I -m unittest discover -s tests/scripts
"""
import os
import sys
import tempfile
import unittest
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'scripts'))
sys.path.insert(0, HERE)

import aseprite        # noqa: E402
import ase_writer      # noqa: E402
import sprite_grids    # noqa: E402

BLOCK = ['................',
         '................',
         '................',
         '................',
         '.....111111.....',
         '.....1abbc1.....',
         '.....1bbbc1.....',
         '.....1bbcc1.....',
         '.....1bccc1.....',
         '.....1cccc1.....',
         '.....111111.....',
         '................',
         '................',
         '................',
         '................',
         '................']


def block(tag='block', others=(), glow=False):
    return sprite_grids.Grid(tag, 13, others, glow, list(BLOCK))


def changed(*changes):
    """BLOCK with (x, y, character) changes."""
    rows = [list(row) for row in BLOCK]
    for x, y, c in changes:
        rows[y][x] = c
    return sprite_grids.Grid('block', 13, rows=[''.join(row) for row in rows])


def problems(grid):
    return ' '.join(sprite_grids.check_item(grid))


def creature(size='medium', air=False, top=4, height=11):
    """An outlined box 8 wide, on the feet row unless it starts elsewhere."""
    rows = ['.' * 16] * 16
    rows[top] = rows[top + height - 1] = '....11111111....'
    for y in range(top + 1, top + height - 1):
        rows[y] = '....1bbbbbb1....'
    return sprite_grids.Grid('thing.idle', 6, rows=rows, size=size, air=air)


class ParseTest(unittest.TestCase):
    def test_a_header_and_rows_round_trip(self):
        grid = sprite_grids.parse(sprite_grids.format_grid(block(others=[6, 7], glow=True)))[0]
        self.assertEqual((grid.tag, grid.bank, grid.others, grid.glow), ('block', 13, [6, 7], True))
        self.assertEqual(grid.rows, BLOCK)

    def test_a_creatures_words_round_trip(self):
        grid = sprite_grids.parse(sprite_grids.format_grid(creature('tiny', air=True)))[0]
        self.assertEqual((grid.size, grid.air), ('tiny', True))

    def test_an_unknown_word_is_an_error(self):
        with self.assertRaisesRegex(sprite_grids.GridError, 'not a header'):
            sprite_grids.parse('block 13 huge\n' + '\n'.join(BLOCK))

    def test_comments_and_blank_lines_are_skipped(self):
        text = '# a note\n\n' + sprite_grids.format_grid(block()) + '\n' + sprite_grids.format_grid(block('two'))
        self.assertEqual([g.tag for g in sprite_grids.parse(text)], ['block', 'two'])

    def test_a_short_grid_is_an_error(self):
        with self.assertRaisesRegex(sprite_grids.GridError, '15 rows'):
            sprite_grids.parse('block 13\n' + '\n'.join(BLOCK[:15]))

    def test_a_bad_character_names_the_row(self):
        rows = list(BLOCK)
        rows[3] = '...............x'
        with self.assertRaisesRegex(sprite_grids.GridError, 'row 3'):
            sprite_grids.parse('block 13\n' + '\n'.join(rows))

    def test_a_bank_past_15_is_an_error(self):
        with self.assertRaisesRegex(sprite_grids.GridError, 'not a header'):
            sprite_grids.parse('block 16\n' + '\n'.join(BLOCK))


class DumpTest(unittest.TestCase):
    def sprite(self):
        pixels = [[0] * 16 for _ in range(16)]
        pixels[1][2] = 13 * 16 + 1
        pixels[1][3] = 13 * 16 + 11
        data = ase_writer.sprite_file([pixels, pixels, pixels], [('one', 0, 0), ('two', 1, 2)])
        with tempfile.NamedTemporaryFile(suffix='.aseprite') as f:
            f.write(data)
            f.flush()
            return aseprite.read(f.name)

    def test_a_frame_gives_its_bank_and_slots(self):
        grid = sprite_grids.dump(self.sprite(), ['one'])[0]
        self.assertEqual(grid.bank, 13)
        self.assertEqual(grid.rows[1], '..1b............')

    def test_a_tag_of_several_frames_numbers_them(self):
        self.assertEqual([g.tag for g in sprite_grids.dump(self.sprite(), ['two'])], ['two#0', 'two#1'])

    def test_an_unknown_tag_is_an_error(self):
        with self.assertRaisesRegex(sprite_grids.GridError, 'no tag'):
            sprite_grids.dump(self.sprite(), ['three'])


class CheckTest(unittest.TestCase):
    def test_a_good_item_is_clean(self):
        self.assertEqual(sprite_grids.check_item(block(others=[6])), [])

    def test_empty(self):
        self.assertIn('empty', problems(sprite_grids.Grid('none', 13, rows=['.' * 16] * 16)))

    def test_too_wide(self):
        grid = changed(*[(x, 7, '1') for x in range(1, 15)])
        self.assertIn('body box 14 x', problems(grid))

    def test_reaches_a_side_column(self):
        self.assertIn('column 0', problems(changed(*[(x, 7, '1') for x in range(0, 5)])))

    def test_reaches_a_row_too_low(self):
        self.assertIn('row 14', problems(changed(*[(7, y, '1') for y in range(11, 15)])))

    def test_off_centre(self):
        rows = [row[3:] + '...' for row in BLOCK]
        grid = sprite_grids.Grid('block', 13, rows=rows)
        self.assertIn('columns 2-7 are off centre', problems(grid))

    def test_a_stray_pixel(self):
        self.assertIn('not joined', problems(changed((1, 1, '1'))))

    def test_corners_touching_are_joined(self):
        self.assertNotIn('not joined', problems(changed((4, 3, '1'))))

    def test_an_edge_pixel_must_be_outline(self):
        self.assertIn('row 4 column 7', problems(changed((7, 4, 'b'))))

    def test_glow_may_leave_the_outline_off(self):
        grid = changed((7, 4, 'b'))
        grid.glow = True
        self.assertEqual(sprite_grids.check_item(grid), [])

    def test_several_banks_need_a_recolourable_slot(self):
        rows = [row.replace('a', '3').replace('b', '3').replace('c', '4') for row in BLOCK]
        self.assertIn('no slot 9-15', problems(sprite_grids.Grid('block', 13, [6], rows=rows)))

    def test_near_silhouettes_are_reported(self):
        self.assertIn('block and master', sprite_grids.near_silhouettes([block()], [block('master')])[0])
        self.assertEqual(sprite_grids.near_silhouettes([block()], [block()]), [])

    def test_different_silhouettes_are_not(self):
        other = sprite_grids.Grid('other', 13, rows=[row[4:] + '....' for row in BLOCK])
        self.assertEqual(sprite_grids.near_silhouettes([block(), other]), [])


    def test_frames_of_a_tag_are_not_near_each_other(self):
        self.assertEqual(sprite_grids.near_silhouettes([block(), block()]), [])


class CreatureCheckTest(unittest.TestCase):
    def test_a_good_creature_is_clean(self):
        self.assertEqual(sprite_grids.check_creature(creature()), [])

    def test_the_box_is_the_classes(self):
        self.assertIn('the most for tiny is 8 x 7', ' '.join(sprite_grids.check_creature(creature('tiny'))))

    def test_feet_must_be_on_their_row(self):
        self.assertIn('feet are on row 13', ' '.join(sprite_grids.check_creature(creature(top=3))))

    def test_in_the_air_the_bottom_is_higher(self):
        self.assertEqual(sprite_grids.check_creature(creature('tiny', True, top=5, height=7)), [])
        self.assertIn('in the air', ' '.join(sprite_grids.check_creature(creature('tiny', True, top=8, height=7))))

    def test_the_shared_rules_apply(self):
        grid = creature()
        grid.rows[4] = '....111b1111....'
        self.assertIn('row 4 column 7', ' '.join(sprite_grids.check_creature(grid)))


class PreviewTest(unittest.TestCase):
    def test_a_png_of_the_expected_size(self):
        palette = [(i, i, i, 255) for i in range(256)]
        png = sprite_grids.preview([block(others=[6, 7])], palette)
        self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n')
        width = int.from_bytes(png[16:20], 'big')
        height = int.from_bytes(png[20:24], 'big')
        self.assertEqual((width, height), (4 + 4 * 132 + 2 * 68 + 16 + 4, 4 + 132))
        body = png[png.index(b'IDAT') + 4:png.index(b'IEND') - 8]
        self.assertEqual(len(zlib.decompress(body)), height * (1 + 3 * width))


class LuaTest(unittest.TestCase):
    def test_pixels_are_palette_indices_in_the_grids_bank(self):
        script = sprite_grids.lua_script([block()])
        self.assertIn('tag = "block"', script)
        self.assertIn(f'{13 * 16 + 1},{13 * 16 + 1}', script)
        self.assertNotIn('{ 0,0,1', script)
        self.assertIn('spr:saveAs(spr.filename)', script)

    def test_grids_of_one_tag_are_its_frames(self):
        script = sprite_grids.lua_script([creature(), creature()])
        self.assertEqual(script.count('tag = "thing.idle"'), 1)
        self.assertIn('duration = 0.5', script)

    def test_an_item_keeps_its_duration(self):
        self.assertIn('duration = 0.1', sprite_grids.lua_script([block()]))

    def test_a_new_frame_does_not_stretch_the_last_tag(self):
        self.assertIn('t2.toFrame = last', sprite_grids.lua_script([block()]))


if __name__ == '__main__':
    unittest.main()
