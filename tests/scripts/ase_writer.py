"""
Writes small Aseprite files for the scripts' tests, so no binary test files
are committed. Only what the readers look at is written: a header, one
frame, and its palette, layer and tileset chunks (Aseprite's
docs/ase-file-specs.md).
"""
import struct
import zlib

RGBA = 32
GRAYSCALE = 16
INDEXED = 8


def _string(text):
    data = text.encode('utf-8')
    return struct.pack('<H', len(data)) + data


def _chunk(kind, body):
    return struct.pack('<IH', 6 + len(body), kind) + body


def palette_chunk(colours):
    """A palette chunk (0x2019) of (r, g, b, a) entries."""
    body = struct.pack('<III8s', len(colours), 0, len(colours) - 1, b'')
    for r, g, b, a in colours:
        body += struct.pack('<HBBBB', 0, r, g, b, a)
    return _chunk(0x2019, body)


def old_palette_chunk(colours):
    """An old palette chunk (0x0004) of (r, g, b) entries, in one packet."""
    body = struct.pack('<HBB', 1, 0, len(colours) & 0xFF)
    for r, g, b in colours:
        body += struct.pack('<BBB', r, g, b)
    return _chunk(0x0004, body)


def layer_chunk(name, kind=0, tileset_index=0):
    """A layer chunk (0x2004): kind 0 a normal layer, 2 a tilemap layer drawing from tileset_index."""
    body = struct.pack('<HHHHHHB3s', 1, kind, 0, 0, 0, 0, 255, b'') + _string(name)
    if kind == 2:
        body += struct.pack('<I', tileset_index)
    return _chunk(0x2004, body)


def tileset_chunk(tiles, depth, index=0, name='Tileset', flags=2 | 4):
    """
    A tileset chunk (0x2023). tiles: tiles[n][y][x], each pixel an (r, g, b, a)
    for RGBA, a (value, alpha) for grayscale, or a palette index for indexed.
    """
    height = len(tiles[0])
    width = len(tiles[0][0])
    pixels = bytearray()
    for tile in tiles:
        for row in tile:
            for p in row:
                pixels += bytes(p) if depth != INDEXED else bytes([p])
    body = struct.pack('<IIIHHh14s', index, flags, len(tiles), width, height, 1, b'') + _string(name)
    if flags & 1:
        body += struct.pack('<II', 0, 0)
    if flags & 2:
        packed = zlib.compress(bytes(pixels))
        body += struct.pack('<I', len(packed)) + packed
    return _chunk(0x2023, body)


def other_chunk(kind=0x2020, size=10):
    """A chunk the readers don't use (user data by default), to be skipped."""
    return _chunk(kind, bytes(size))


def aseprite_file(chunks, depth=RGBA, transparent_index=0, width=16, height=16):
    """The bytes of a file with one frame holding chunks."""
    frame_body = b''.join(chunks)
    frame = struct.pack('<IHHH2sI', 16 + len(frame_body), 0xF1FA, len(chunks), 100, b'',
                        len(chunks)) + frame_body
    header = struct.pack('<IHHHHHIHII', 128 + len(frame), 0xA5E0, 1, width, height, depth,
                         1, 100, 0, 0)
    header += struct.pack('<B3sH', transparent_index, b'', 256)
    header += bytes(128 - len(header))
    return header + frame


def tileset_file(tiles, depth=RGBA, palette=None, transparent_index=0, extra=()):
    """A file with a Background layer, one tilemap layer and its tileset, plus any extra chunks."""
    chunks = []
    if palette is not None:
        chunks.append(palette_chunk(palette))
    chunks += [layer_chunk('Background'), layer_chunk('Tilemap 1', kind=2)]
    chunks += list(extra)
    chunks.append(tileset_chunk(tiles, depth))
    return aseprite_file(chunks, depth, transparent_index)
