/*
 * The help screen's page data, in page 105 (slot 7 while its code in page
 * 104 runs): the pages, constant text. Each page's text is one string for
 * nge_puts(): lines end with \n, and colour changes are the stream's own
 * INK code, by the macros below. See docs/help_screen.md, D1.
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

const screen_guide_page_t screen_guide_pages[SCREEN_GUIDE_PAGES] = {
    { "Commands",            H "Keys in play" T "\n" },
    { "Screens and prompts", H "Keys in screens and prompts" T "\n" },
    { "Goal",                H "How to win" T "\n" },
    { "Abilities and stats", H "What the abilities do" T "\n" },
};
