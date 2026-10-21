"""
Reads an Aseprite file (.aseprite, .ase): the parts an importer needs.

The format is Aseprite's docs/ase-file-specs.md. Read are the header, the
palette, the layers and the tilesets; every other chunk (cels, tags, user
data, slices) is skipped by its size. Only the first frame's chunks matter:
layers, palettes and tilesets are all in it.

Tiles are given as rows of (r, g, b, a) pixels, whatever the sprite's colour
mode, with alpha 0 (a transparent pixel, given as (0, 0, 0, 0)) or 255, so
importers don't deal with colour modes. A pixel with any other alpha is an
error.

    sprite = aseprite.read('assets/Tilesheet.aseprite')
    tileset = sprite.tilemap_tileset()
    tileset.tiles[n][y][x]  -> (r, g, b, a)
"""
import struct
import zlib

HEADER_SIZE = 128
HEADER_MAGIC = 0xA5E0
FRAME_MAGIC = 0xF1FA

DEPTH_RGBA = 32
DEPTH_GRAYSCALE = 16
DEPTH_INDEXED = 8

CHUNK_OLD_PALETTE = 0x0004
CHUNK_LAYER = 0x2004
CHUNK_PALETTE = 0x2019
CHUNK_TILESET = 0x2023

LAYER_TILEMAP = 2

TILESET_EXTERNAL = 1   # tileset flags
TILESET_IN_FILE = 2

TRANSPARENT = (0, 0, 0, 0)


class AsepriteError(Exception):
    """The file can't be read, or uses something the importers don't support."""


class Layer:
    def __init__(self, name, kind, tileset_index):
        self.name = name
        self.kind = kind                    # LAYER_TILEMAP, or another layer type
        self.tileset_index = tileset_index  # a tilemap layer's tileset; None otherwise


class Tileset:
    def __init__(self, index, name, width, height, tiles):
        self.index = index
        self.name = name
        self.width = width
        self.height = height
        self.tiles = tiles                  # tiles[n][y][x] -> (r, g, b, a)


class Sprite:
    def __init__(self, width, height, depth, transparent_index):
        self.width = width
        self.height = height
        self.depth = depth
        self.transparent_index = transparent_index   # indexed mode only
        self.palette = []                            # (r, g, b, a) per entry
        self.layers = []
        self.tilesets = {}                           # by tileset index

    def tilemap_tileset(self, layer_name=None):
        """The tileset of the tilemap layer named, or of the only tilemap layer."""
        tilemaps = [layer for layer in self.layers if layer.kind == LAYER_TILEMAP]
        if layer_name is not None:
            tilemaps = [layer for layer in tilemaps if layer.name == layer_name]
            if not tilemaps:
                raise AsepriteError(f'no tilemap layer named "{layer_name}"')
        elif not tilemaps:
            raise AsepriteError('no tilemap layer')
        elif len(tilemaps) > 1:
            names = ', '.join(f'"{layer.name}"' for layer in tilemaps)
            raise AsepriteError(f'several tilemap layers ({names}): choose one with --layer')
        index = tilemaps[0].tileset_index
        if index not in self.tilesets:
            raise AsepriteError(f'layer "{tilemaps[0].name}": tileset {index} not found')
        return self.tilesets[index]


class _Reader:
    """Little-endian fields from a bytes object, from a position."""

    def __init__(self, data, pos=0):
        self.data = data
        self.pos = pos

    def take(self, fmt):
        values = struct.unpack_from('<' + fmt, self.data, self.pos)
        self.pos += struct.calcsize('<' + fmt)
        return values

    def one(self, fmt):
        return self.take(fmt)[0]

    def string(self):
        length = self.one('H')
        text = self.data[self.pos:self.pos + length].decode('utf-8')
        self.pos += length
        return text


def read(path):
    """Read the file at path: a Sprite."""
    with open(path, 'rb') as f:
        data = f.read()
    try:
        return _parse(data)
    except (struct.error, zlib.error, UnicodeDecodeError) as e:
        raise AsepriteError(f'{path}: not a readable Aseprite file ({e})') from e


def _parse(data):
    if len(data) < HEADER_SIZE:
        raise AsepriteError('too short for an Aseprite header')
    r = _Reader(data)
    _size, magic, frames, width, height, depth = r.take('IHHHHH')
    if magic != HEADER_MAGIC:
        raise AsepriteError('not an Aseprite file (bad magic number)')
    if depth == DEPTH_GRAYSCALE:
        raise AsepriteError('grayscale colour mode is not supported: use RGB or indexed')
    if depth not in (DEPTH_RGBA, DEPTH_INDEXED):
        raise AsepriteError(f'unknown colour depth {depth}')
    sprite = Sprite(width, height, depth, data[28])
    if frames == 0:
        return sprite

    # The first frame
    f = _Reader(data, HEADER_SIZE)
    frame_size, frame_magic, old_chunks, _duration, _, new_chunks = f.take('IHHH2sI')
    if frame_magic != FRAME_MAGIC:
        raise AsepriteError('bad frame header')
    chunks = new_chunks if new_chunks else old_chunks
    frame_end = HEADER_SIZE + frame_size

    old_palette = None
    raw_tilesets = []
    pos = f.pos
    for _ in range(chunks):
        size, kind = struct.unpack_from('<IH', data, pos)
        if size < 6 or pos + size > frame_end:
            raise AsepriteError(f'bad chunk size at offset {pos}')
        body = _Reader(data[pos + 6:pos + size])
        if kind == CHUNK_LAYER:
            sprite.layers.append(_layer(body))
        elif kind == CHUNK_PALETTE:
            _palette(body, sprite.palette)
        elif kind == CHUNK_OLD_PALETTE:
            old_palette = _old_palette(body)
        elif kind == CHUNK_TILESET:
            raw_tilesets.append(_tileset_header(body))
        pos += size

    if not sprite.palette and old_palette is not None:
        sprite.palette = old_palette
    # Pixels are converted once the palette is known: it can come after the tilesets
    for index, name, width, height, count, pixels in raw_tilesets:
        sprite.tilesets[index] = Tileset(index, name, width, height,
                                         _tiles(sprite, name, width, height, count, pixels))
    return sprite


def _layer(r):
    _flags, kind, _child, _w, _h, _blend, _opacity, _ = r.take('HHHHHHB3s')
    name = r.string()
    tileset_index = r.one('I') if kind == LAYER_TILEMAP else None
    return Layer(name, kind, tileset_index)


def _palette(r, palette):
    size, first, last, _ = r.take('III8s')
    if len(palette) < size:
        palette.extend([TRANSPARENT] * (size - len(palette)))
    for i in range(first, last + 1):
        flags, red, green, blue, alpha = r.take('HBBBB')
        if flags & 1:
            r.string()
        palette[i] = (red, green, blue, alpha)


def _old_palette(r):
    palette = [TRANSPARENT] * 256
    index = 0
    for _ in range(r.one('H')):
        skip, count = r.take('BB')
        index += skip
        for _ in range(count or 256):
            red, green, blue = r.take('BBB')
            palette[index] = (red, green, blue, 255)
            index += 1
    return palette


def _tileset_header(r):
    index, flags, count, width, height, _base, _ = r.take('IIIHHh14s')
    name = r.string()
    if flags & TILESET_EXTERNAL:
        raise AsepriteError(f'tileset "{name}" is in an external file: not supported')
    if not flags & TILESET_IN_FILE:
        raise AsepriteError(f'tileset "{name}" has no tiles in the file')
    length = r.one('I')
    pixels = zlib.decompress(r.data[r.pos:r.pos + length])
    return index, name, width, height, count, pixels


def _tiles(sprite, name, width, height, count, pixels):
    """The tileset's image (the tiles stacked top to bottom) as tiles of RGBA pixels."""
    step = sprite.depth // 8
    if len(pixels) < width * height * count * step:
        raise AsepriteError(f'tileset "{name}": image too short')
    tiles = []
    for n in range(count):
        rows = []
        for y in range(height):
            row = []
            for x in range(width):
                at = ((n * height + y) * width + x) * step
                pixel = _rgba(sprite, pixels, at)
                if pixel[3] == 0:
                    pixel = TRANSPARENT
                elif pixel[3] != 255:
                    raise AsepriteError(f'tile {n}, pixel ({x}, {y}): alpha {pixel[3]}, '
                                        'only fully transparent or opaque is supported')
                row.append(pixel)
            rows.append(row)
        tiles.append(rows)
    return tiles


def _rgba(sprite, pixels, at):
    if sprite.depth == DEPTH_RGBA:
        return tuple(pixels[at:at + 4])
    index = pixels[at]
    if index == sprite.transparent_index:
        return TRANSPARENT
    if index >= len(sprite.palette):
        raise AsepriteError(f'palette index {index} past the palette ({len(sprite.palette)} entries)')
    return sprite.palette[index]
