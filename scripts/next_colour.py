"""
Next colours and palette banks, for any importer (docs/tileset_import.md).

A Next colour is 9 bits, 3 each of red, green and blue: here an int,
red << 6 | green << 3 | blue. In 8 bits a channel is one of LEVELS, and only
those convert: a colour from the Next's 512-colour palette converts exactly,
anything else is an error.

Banks: 16 palette entries that 4-bit graphics index with pixel values 0-15,
0 being transparent. fit_banks() gives each graphic a bank, first fit in
order (D1).
"""

# 8-bit channel values of the 3-bit levels 0-7: round(level * 255 / 7)
LEVELS = (0, 36, 73, 109, 146, 182, 219, 255)

BANK_SIZE = 16
BANK_COLOURS = BANK_SIZE - 1    # entry 0 is transparent
BANKS_MAX = 16


class ColourError(Exception):
    """A colour that isn't a Next colour, or graphics that don't fit the banks."""


def to_next(rgb):
    """The 9-bit Next colour of an (r, g, b) in 8 bits a channel, or None if it isn't one."""
    try:
        red, green, blue = (LEVELS.index(v) for v in rgb[:3])
    except ValueError:
        return None
    return red << 6 | green << 3 | blue


def entry_bytes(colour):
    """A colour as nge_palette_entry_9_t's two bytes: RRRGGGBB, then the low blue bit."""
    return colour >> 1, colour & 1


def fit_banks(graphics, banks_max=BANKS_MAX):
    """
    Give each graphic a bank.

    graphics: for each graphic, its opaque colours (9-bit), in the order its
    pixels first use them. Each graphic in turn goes in the first bank its
    colours fit in together with the bank's, at most BANK_COLOURS; if none
    fits, in a new bank. A bank's colours stay in the order they were added,
    so a graphic added at the end never changes the others' pixel values.

    Returns (banks, assigned): banks, a list of each bank's colours (pixel
    value i + 1 is colour i); assigned, each graphic's bank number. Raises
    ColourError naming the graphic (by its position) if it has too many
    colours or the banks run out.
    """
    banks = []
    assigned = []
    for n, colours in enumerate(graphics):
        if len(set(colours)) > BANK_COLOURS:
            raise ColourError(f'tile {n}: {len(set(colours))} colours, at most {BANK_COLOURS}')
        for b, bank in enumerate(banks):
            new = [c for c in dict.fromkeys(colours) if c not in bank]
            if len(bank) + len(new) <= BANK_COLOURS:
                bank.extend(new)
                break
        else:
            if len(banks) == banks_max:
                raise ColourError(f'tile {n}: its colours fit no bank, and all {banks_max} are used')
            banks.append(list(dict.fromkeys(colours)))
            b = len(banks) - 1
        assigned.append(b)
    return banks, assigned


def pixel_value(bank, colour):
    """The pixel value (1-15) of a colour in its bank."""
    return bank.index(colour) + 1
