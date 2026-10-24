"""
Reads an Aseprite file (.aseprite, .ase): the parts an importer needs.

The format is Aseprite's docs/ase-file-specs.md. Read are the header, the
palette, the layers, the tilesets, the tags, the slices, and every frame's
duration and cels; every other chunk (user data) is skipped by its size.

Tiles are given as rows of (r, g, b, a) pixels, whatever the sprite's colour
mode, with alpha 0 (a transparent pixel, given as (0, 0, 0, 0)) or 255, so
importers don't deal with colour modes. A pixel with any other alpha is an
error.

    sprite = aseprite.read('assets/Tilesheet.aseprite')
    tileset = sprite.tilemap_tileset()
    tileset.tiles[n][y][x]  -> (r, g, b, a)

Frames of an indexed sprite are given as their visible layers combined, as
rows of palette indices, None where transparent:

    sprite = aseprite.read('assets/Sprites.aseprite')
    sprite.frames[n].duration          -> ms
    sprite.frame_indices(n)[y][x]      -> palette index, or None
    sprite.tags                        -> Tag(name, first, last, direction)

Slices name rectangles of the canvas; with a tilemap layer, the tile under
a point of the canvas:

    sprite.slices                      -> Slice(name, x, y, width, height)
    sprite.tile_at(px, py)             -> tile number on the tilemap layer, 0 if none
"""
import struct
import zlib

HEADER_SIZE = 128
HEADER_MAGIC = 0xA5E0
FRAME_MAGIC = 0xF1FA

DEPTH_RGBA = 32
DEPTH_GRAYSCALE = 16
DEPTH_INDEXED = 8

HEADER_OPACITY_VALID = 1   # header flags: layers' opacity is set

CHUNK_OLD_PALETTE = 0x0004
CHUNK_LAYER = 0x2004
CHUNK_CEL = 0x2005
CHUNK_TAGS = 0x2018
CHUNK_PALETTE = 0x2019
CHUNK_SLICE = 0x2022
CHUNK_TILESET = 0x2023

LAYER_IMAGE = 0
LAYER_GROUP = 1
LAYER_TILEMAP = 2

LAYER_VISIBLE = 1        # layer flags
LAYER_REFERENCE = 64

CEL_RAW = 0
CEL_LINKED = 1
CEL_COMPRESSED = 2
CEL_TILEMAP = 3

SLICE_NINE_PATCH = 1     # slice flags: what each key has after its rectangle
SLICE_PIVOT = 2

TAG_FORWARD = 0          # tag directions
TAG_DIRECTIONS = ('forward', 'reverse', 'ping-pong', 'ping-pong reverse')

TILESET_EXTERNAL = 1   # tileset flags
TILESET_IN_FILE = 2

TRANSPARENT = (0, 0, 0, 0)


class AsepriteError(Exception):
    """The file can't be read, or uses something the importers don't support."""


class Layer:
    def __init__(self, name, kind, tileset_index, flags=LAYER_VISIBLE, level=0, opacity=255):
        self.name = name
        self.kind = kind                    # LAYER_IMAGE, LAYER_GROUP or LAYER_TILEMAP
        self.tileset_index = tileset_index  # a tilemap layer's tileset; None otherwise
        self.flags = flags                  # LAYER_VISIBLE, LAYER_REFERENCE...
        self.level = level                  # 0 at the top; a group's layers are one deeper
        self.opacity = opacity
        self.shown = True                   # visible, and so are the groups it is in; not a reference


class Cel:
    def __init__(self, layer, x, y, opacity, kind, width=0, height=0, pixels=b'', link=None, tiles=None):
        self.layer = layer                  # index into Sprite.layers
        self.x = x
        self.y = y
        self.opacity = opacity
        self.kind = kind                    # CEL_RAW, CEL_LINKED, CEL_COMPRESSED or CEL_TILEMAP
        self.width = width
        self.height = height
        self.pixels = pixels                # uncompressed, a byte or 4 a pixel
        self.link = link                    # a linked cel's frame
        self.tiles = tiles                  # a tilemap cel's tile numbers, width x height of them


class Slice:
    def __init__(self, name, x, y, width, height):
        self.name = name
        self.x = x                          # its first key's rectangle, in canvas pixels
        self.y = y
        self.width = width
        self.height = height


class Frame:
    def __init__(self, duration):
        self.duration = duration            # ms
        self.cels = {}                      # by layer index


class Tag:
    def __init__(self, name, first, last, direction):
        self.name = name
        self.first = first                  # frames first to last, inclusive
        self.last = last
        self.direction = direction          # TAG_FORWARD, or another of TAG_DIRECTIONS


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
        self.frames = []
        self.tags = []
        self.slices = []

    def tilemap_tileset(self, layer_name=None):
        """The tileset of the tilemap layer named, or of the only tilemap layer."""
        return self.tilesets[self.layers[self.tilemap_layer(layer_name)].tileset_index]

    def tilemap_layer(self, layer_name=None):
        """The index of the tilemap layer named, or of the only tilemap layer, with its tileset."""
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
        return self.layers.index(tilemaps[0])

    def tile_at(self, px, py, layer_name=None):
        """
        The tile number at canvas pixel (px, py) on the tilemap layer's cel in
        the first frame: 0 (the empty tile) where the cel has none.
        """
        layer = self.tilemap_layer(layer_name)
        tileset = self.tilesets[self.layers[layer].tileset_index]
        cel = self.cel(0, layer) if self.frames else None
        if cel is None or cel.tiles is None:
            return 0
        col = (px - cel.x) // tileset.width
        row = (py - cel.y) // tileset.height
        if px < cel.x or py < cel.y or col >= cel.width or row >= cel.height:
            return 0
        return cel.tiles[row * cel.width + col]

    def cel(self, frame, layer):
        """The cel of layer in frame, a linked cel followed to its frame's; None if there is none."""
        cel = self.frames[frame].cels.get(layer)
        if cel is not None and cel.kind == CEL_LINKED:
            cel = self.frames[cel.link].cels.get(layer)
            if cel is None or cel.kind == CEL_LINKED:
                raise AsepriteError(f'frame {frame}: a linked cel with nothing to link to')
        return cel

    def frame_indices(self, n):
        """
        Frame n of an indexed sprite: its shown image layers combined, bottom to
        top, as rows of palette indices, None where transparent. A cel's pixels
        off the canvas are left out.
        """
        if self.depth != DEPTH_INDEXED:
            raise AsepriteError('frames as palette indices need indexed colour mode')
        rows = [[None] * self.width for _ in range(self.height)]
        for i, layer in enumerate(self.layers):
            if layer.kind != LAYER_IMAGE or not layer.shown:
                continue
            cel = self.cel(n, i)
            if cel is None:
                continue
            for cy in range(cel.height):
                y = cel.y + cy
                if not 0 <= y < self.height:
                    continue
                for cx in range(cel.width):
                    x = cel.x + cx
                    index = cel.pixels[cy * cel.width + cx]
                    if 0 <= x < self.width and index != self.transparent_index:
                        rows[y][x] = index
        return rows


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
    header_flags = struct.unpack_from('<I', data, 14)[0]
    sprite = Sprite(width, height, depth, data[28])

    old_palette = None
    raw_tilesets = []
    pos = HEADER_SIZE
    for n in range(frames):
        f = _Reader(data, pos)
        frame_size, frame_magic, old_chunks, duration, _, new_chunks = f.take('IHHH2sI')
        if frame_magic != FRAME_MAGIC:
            raise AsepriteError(f'frame {n}: bad frame header')
        frame = Frame(duration)
        sprite.frames.append(frame)
        chunks = new_chunks if new_chunks else old_chunks
        frame_end = pos + frame_size
        at = f.pos
        for _ in range(chunks):
            size, kind = struct.unpack_from('<IH', data, at)
            if size < 6 or at + size > frame_end:
                raise AsepriteError(f'frame {n}: bad chunk size at offset {at}')
            body = _Reader(data[at + 6:at + size])
            if kind == CHUNK_LAYER:
                sprite.layers.append(_layer(body, header_flags))
            elif kind == CHUNK_PALETTE:
                _palette(body, sprite.palette)
            elif kind == CHUNK_OLD_PALETTE:
                old_palette = _old_palette(body)
            elif kind == CHUNK_TILESET:
                raw_tilesets.append(_tileset_header(body))
            elif kind == CHUNK_CEL:
                cel = _cel(body, depth // 8, n)
                frame.cels[cel.layer] = cel
            elif kind == CHUNK_TAGS:
                sprite.tags = _tags(body)
            elif kind == CHUNK_SLICE:
                sprite.slices.append(_slice(body))
            at += size
        pos = frame_end

    _show_layers(sprite.layers)
    if not sprite.palette and old_palette is not None:
        sprite.palette = old_palette
    # Pixels are converted once the palette is known: it can come after the tilesets
    for index, name, width, height, count, pixels in raw_tilesets:
        sprite.tilesets[index] = Tileset(index, name, width, height,
                                         _tiles(sprite, name, width, height, count, pixels))
    return sprite


def _layer(r, header_flags):
    flags, kind, level, _w, _h, _blend, opacity, _ = r.take('HHHHHHB3s')
    name = r.string()
    tileset_index = r.one('I') if kind == LAYER_TILEMAP else None
    if not header_flags & HEADER_OPACITY_VALID:
        opacity = 255
    return Layer(name, kind, tileset_index, flags, level, opacity)


def _show_layers(layers):
    """Set each layer's shown: it and every group it is in are visible, and it isn't a reference layer."""
    groups = []   # the groups the layer is in: groups[level] is its parent at that level
    for layer in layers:
        del groups[layer.level:]
        parent_shown = groups[-1].shown if groups else True
        layer.shown = (parent_shown and bool(layer.flags & LAYER_VISIBLE) and
                       not layer.flags & LAYER_REFERENCE)
        if layer.kind == LAYER_GROUP:
            groups.append(layer)


def _cel(r, step, frame):
    layer, x, y, opacity, kind, z_index, _ = r.take('HhhBHh5s')
    if z_index != 0:
        raise AsepriteError(f'frame {frame}, layer {layer}: a cel with a z-index is not supported')
    if kind == CEL_LINKED:
        return Cel(layer, x, y, opacity, kind, link=r.one('H'))
    if kind == CEL_TILEMAP:
        width, height, bits, id_mask, _x_flip, _y_flip, _d_flip, _ = r.take('HHHIIII10s')
        if bits != 32:
            raise AsepriteError(f'frame {frame}, layer {layer}: tilemap of {bits}-bit tiles, only 32 is supported')
        raw = zlib.decompress(r.data[r.pos:])
        if len(raw) < width * height * 4:
            raise AsepriteError(f'frame {frame}, layer {layer}: tilemap too short')
        tiles = [v & id_mask for v in struct.unpack_from(f'<{width * height}I', raw)]
        return Cel(layer, x, y, opacity, kind, width, height, tiles=tiles)
    if kind in (CEL_RAW, CEL_COMPRESSED):
        width, height = r.take('HH')
        pixels = r.data[r.pos:]
        if kind == CEL_COMPRESSED:
            pixels = zlib.decompress(pixels)
        if len(pixels) < width * height * step:
            raise AsepriteError(f'frame {frame}, layer {layer}: cel image too short')
        if step != 1:
            pixels = pixels[:width * height * step]
        return Cel(layer, x, y, opacity, kind, width, height, pixels)
    return Cel(layer, x, y, opacity, kind)


def _slice(r):
    count, flags, _ = r.take('III')
    name = r.string()
    first = None
    for _ in range(count):
        frame, x, y, width, height = r.take('IiiII')
        if flags & SLICE_NINE_PATCH:
            r.take('iiII')
        if flags & SLICE_PIVOT:
            r.take('ii')
        if first is None or frame == 0:
            first = Slice(name, x, y, width, height)
    if first is None:
        raise AsepriteError(f'slice "{name}" has no keys')
    return first


def _tags(r):
    count, _ = r.take('H8s')
    tags = []
    for _ in range(count):
        first, last, direction, _repeat, _, _rgb, _extra = r.take('HHBH6s3sB')
        tags.append(Tag(r.string(), first, last, direction))
    return tags


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
