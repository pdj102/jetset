"""
Writes small Aseprite files for the scripts' tests, so no binary test files
are committed. Only what the readers look at is written: a header, frames
with their durations, and palette, layer, tileset, cel and tags chunks
(Aseprite's docs/ase-file-specs.md).
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


def layer_chunk(name, kind=0, tileset_index=0, flags=1, level=0, opacity=255):
    """
    A layer chunk (0x2004): kind 0 a normal layer, 1 a group, 2 a tilemap layer
    drawing from tileset_index. flags: 1 visible, 64 a reference layer; level:
    0 at the top, a group's layers one deeper.
    """
    body = struct.pack('<HHHHHHB3s', flags, kind, level, 0, 0, 0, opacity, b'') + _string(name)
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


def cel_chunk(layer, rows, x=0, y=0, kind=2, opacity=255, link=None, z_index=0):
    """
    A cel chunk (0x2005) of an indexed image: rows of palette indices. kind 0
    raw, 2 compressed; or link, a frame whose cel this one shows (kind 1).
    """
    if link is not None:
        kind = 1
    body = struct.pack('<HhhBHh5s', layer, x, y, opacity, kind, z_index, b'')
    if kind == 1:
        return _chunk(0x2005, body + struct.pack('<H', link))
    pixels = bytes(p for row in rows for p in row)
    if kind == 2:
        pixels = zlib.compress(pixels)
    return _chunk(0x2005, body + struct.pack('<HH', len(rows[0]), len(rows)) + pixels)


def tags_chunk(tags):
    """A tags chunk (0x2018): tags as (name, first, last) or (name, first, last, direction)."""
    body = struct.pack('<H8s', len(tags), b'')
    for tag in tags:
        name, first, last = tag[:3]
        direction = tag[3] if len(tag) > 3 else 0
        body += struct.pack('<HHBH6s3sB', first, last, direction, 0, b'', b'', 0) + _string(name)
    return _chunk(0x2018, body)


def other_chunk(kind=0x2020, size=10):
    """A chunk the readers don't use (user data by default), to be skipped."""
    return _chunk(kind, bytes(size))


def aseprite_file(chunks, depth=RGBA, transparent_index=0, width=16, height=16):
    """The bytes of a file with one frame holding chunks."""
    return aseprite_frames([(100, chunks)], depth, transparent_index, width, height)


def aseprite_frames(frames, depth=RGBA, transparent_index=0, width=16, height=16):
    """The bytes of a file with frames, each (duration in ms, its chunks)."""
    body = b''
    for duration, chunks in frames:
        frame_body = b''.join(chunks)
        body += struct.pack('<IHHH2sI', 16 + len(frame_body), 0xF1FA, len(chunks), duration, b'',
                            len(chunks)) + frame_body
    header = struct.pack('<IHHHHHIHII', 128 + len(body), 0xA5E0, len(frames), width, height, depth,
                         1, 100, 0, 0)
    header += struct.pack('<B3sH', transparent_index, b'', 256)
    header += bytes(128 - len(header))
    return header + body


def tileset_file(tiles, depth=RGBA, palette=None, transparent_index=0, extra=()):
    """A file with a Background layer, one tilemap layer and its tileset, plus any extra chunks."""
    chunks = []
    if palette is not None:
        chunks.append(palette_chunk(palette))
    chunks += [layer_chunk('Background'), layer_chunk('Tilemap 1', kind=2)]
    chunks += list(extra)
    chunks.append(tileset_chunk(tiles, depth))
    return aseprite_file(chunks, depth, transparent_index)


def sprite_file(frames, tags, palette=None, durations=None, layers=None, width=16, height=16):
    """
    An indexed sprite file: frames[n] is {layer index: rows of palette indices,
    or a ready-made cel chunk} (or just the rows, for layer 0); tags as for
    tags_chunk(); layers are layer chunks (one normal layer "Layer 1" by
    default); palette (r, g, b, a) entries (by default 4 banks of distinct
    Next colours); durations in ms (100 each by default).
    """
    if palette is None:
        levels = (0, 36, 73, 109, 146, 182, 219, 255)
        palette = [(levels[i % 8], levels[(i // 8) % 8], 255, 255) for i in range(64)]
    if layers is None:
        layers = [layer_chunk('Layer 1')]
    if durations is None:
        durations = [100] * len(frames)
    out = []
    for n, frame in enumerate(frames):
        if not isinstance(frame, dict):
            frame = {0: frame}
        chunks = []
        if n == 0:
            chunks += [palette_chunk(palette)] + list(layers) + [tags_chunk(tags)]
        for layer, rows in frame.items():
            chunks.append(rows if isinstance(rows, bytes) else cel_chunk(layer, rows))
        out.append((durations[n], chunks))
    return aseprite_frames(out, INDEXED, 0, width, height)
