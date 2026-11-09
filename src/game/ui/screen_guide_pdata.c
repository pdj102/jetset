/*
 * The help screen's page data, in page 105 (slot 7 while its code in page
 * 104 runs): the pages, constant text. Each page's text is one string for
 * nge_puts(): lines end with \n, and colour changes are the stream's own
 * INK code, by the macros below. See docs/help_screen.md, D1.
 *
 * The text names keys and rules that live elsewhere, each marked
 * "described in the help screen" (D9): player_command() and the screens'
 * and prompts' keys, the quest's stairs check, the effects' saves
 * (effects_pdata.c), the features' checks (feature.c) and the level and
 * gold gains (pc.c). A change there changes a line here. A PC test checks
 * that each page fits its panel: 29 lines of 40 characters (R11).
 */
#ifdef __SDCC
#pragma constseg PAGE_105
#endif

#include "game/ui/screen_internal.h"

/*
 * INK (NGE_CTRL_INK) and its colour (NGE_INK_*): H a heading, yellow; K a
 * key, cyan; T text, white. Each ends at a string boundary (H "Moving"),
 * so the hex escape can't swallow the letters after it
 */
#define H  "\x10\x0D"
#define K  "\x10\x0E"
#define T  "\x10\x0C"

/* A heading's line; a key's line (the key padded to 13 columns, then what it does); a line of text */
#define HEAD(s)       H s "\n"
#define KEY(k, s)     " " K k T s "\n"
#define TEXT(s)       T s "\n"

static const char commands[] =
    HEAD("Moving")
    KEY("arrows/hjkl  ", "move; two keys: diagonal")
    KEY(".            ", "wait a turn")
    KEY("< >          ", "go up, down the stairs")
    HEAD("Items")
    KEY("g ,          ", "pick up")
    KEY("i            ", "inventory")
    KEY("w r          ", "wear, take off")
    KEY("e            ", "exchange for what's worn")
    KEY("d            ", "drop")
    KEY("u            ", "use: drink, read, eat")
    HEAD("Fighting and magic")
    KEY("move into    ", "attack; open a door")
    KEY("f            ", "fire or throw a missile")
    KEY("a            ", "aim a wand")
    KEY("m            ", "cast a spell")
    HEAD("The dungeon")
    KEY("o            ", "use a shrine or fountain")
    KEY("c            ", "close a door")
    KEY("p            ", "unlock: a key or a pick")
    KEY("s            ", "search for hidden traps")
    KEY("D            ", "disarm a trap")
    KEY("x            ", "look around")
    HEAD("You")
    KEY("C            ", "your character")
    KEY("?            ", "this help")
    KEY("Q            ", "quit the game");

static const char screens[] =
    HEAD("Inventory (i)")
    KEY("up/down jk   ", "select an item")
    KEY("w r x d u a  ", "act on it (bottom line)")
    KEY("ENTER        ", "use, aim, wear or take off")
    KEY("SPACE        ", "back")
    HEAD("Choosing an item (d w r e u a p)")
    KEY("letter       ", "choose it")
    KEY("up/down      ", "select; ENTER chooses")
    KEY("SPACE        ", "back")
    HEAD("Spells (m)")
    KEY("letter       ", "cast it")
    KEY("up/down jk   ", "select; ENTER casts")
    KEY("SPACE        ", "back")
    HEAD("Aiming, firing, looking (a f m x)")
    KEY("arrows/hjkl  ", "move the cursor")
    KEY("ENTER SPACE  ", "choose the tile")
    KEY("other keys   ", "back")
    HEAD("Directions (c o p D)")
    KEY("a direction  ", "choose it")
    KEY("other keys   ", "back")
    HEAD("This help (?)")
    KEY("arrows h l   ", "turn the page")
    KEY("1-4          ", "go to a page")
    KEY("SPACE        ", "back")
    "\n"
    TEXT("The bottom line always shows the keys")
    TEXT("you can use now.");

static const char goal[] =
    HEAD("The crown")
    TEXT(" The crown lies on depth 10, the")
    TEXT(" deepest level of the dungeon.")
    "\n"
    HEAD("To win")
    TEXT(" 1 Go down the stairs (>) to depth 10.")
    TEXT(" 2 Find the crown and pick it up (g).")
    TEXT(" 3 Climb back up (<). Leave by the up")
    TEXT("   stairs on depth 1 with the crown.")
    "\n"
    TEXT(" You can't leave without the crown.")
    "\n"
    HEAD("If you drop the crown")
    TEXT(" It stays on that depth. Levels are")
    TEXT(" made anew on each visit, but the")
    TEXT(" crown will be there when you return.")
    "\n"
    HEAD("If you die")
    TEXT(" The game is over.");

static const char abilities[] =
    HEAD("Abilities")
    TEXT(" 1-30. Each adds a modifier to its")
    TEXT(" rolls: (score - 10) / 2, rounded down.")
    TEXT(" 8-9 -1, 10-11 +0, 12-13 +1.")
    KEY("Str  ", "resisting being held")
    KEY("Dex  ", "disarming traps, picking locks")
    KEY("Con  ", "HP at each level; resisting")
    TEXT("      poison, weakness and blindness")
    KEY("Int  ", "MP at each level")
    KEY("Wis  ", "finding hidden traps; resisting")
    TEXT("      slowness and sleep")
    KEY("Cha  ", "no use yet")
    HEAD("Stats")
    KEY("HP     ", "hit points: at 0 you die")
    KEY("MP     ", "mana: spells cost MP")
    KEY("AC     ", "armour class: higher is hit")
    TEXT("        less often")
    KEY("Speed  ", "lower acts more often")
    KEY("XP     ", "from kills, and 1 a gold piece:")
    TEXT("        enough XP raises your level")
    "\n"
    HEAD("Colours (C screen)")
    TEXT(" cyan better than base, red worse");

const screen_guide_page_t screen_guide_pages[SCREEN_GUIDE_PAGES] = {
    { "Commands",            commands },
    { "Screens and prompts", screens },
    { "Goal",                goal },
    { "Abilities and stats", abilities },
};
