/*
 * The help screen: banked in page 104 beside the PC screen and the spell
 * list, as page 116 is full. Its pages' text is in page 105
 * (screen_guide_pdata.c), mapped into slot 7 while this code runs, and is
 * passed only to common code (the panels, the output path).
 */
#ifdef __SDCC
#pragma codeseg PAGE_104
#pragma constseg PAGE_104
#endif

#include "game/ui/screen_guide.h"

#include "core/nge_util.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "ui/nge_panel.h"

#include "game/ui/screen_internal.h"

/* The screen's panels, by PANEL_* */
#define PANEL_TITLE  0   /* the heading, and "n/4" at the right */
#define PANEL_BODY   1   /* the page's text */
#define PANEL_COUNT  2

static const nge_panel_t panels[PANEL_COUNT] = {
    [PANEL_TITLE] = { { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 1 }, SCREEN_PAPER, NGE_PANEL_CLIP },
    [PANEL_BODY]  = { { .x1 = 0, .x2 = 39, .y1 = 2, .y2 = 30 }, SCREEN_PAPER, NGE_PANEL_CLIP },
};

/* The title panel: "n/4" right-aligned on row 0 */
#define COL_PAGE  37

/* Characters of the keys (nge_key_ascii()) */
#define CHAR_LEFT   8
#define CHAR_RIGHT  9

uint8_t screen_guide_turn(uint8_t page, uint8_t c)
{
    switch (c) {
    case CHAR_RIGHT:
    case 'l':
        return (uint8_t)((uint8_t)(page + 1) % SCREEN_GUIDE_PAGES);
    case CHAR_LEFT:
    case 'h':
        return (uint8_t)((uint8_t)(page + SCREEN_GUIDE_PAGES - 1) % SCREEN_GUIDE_PAGES);
    default:
        if (c >= '1' && c < '1' + SCREEN_GUIDE_PAGES) {
            return (uint8_t)(c - '1');
        }
        return page;
    }
}

/* Page page, whole: its heading and number on the title panel, its text on the body */
static void draw(uint8_t page, NGE_FILE **streams)
{
    NGE_FILE *title = streams[PANEL_TITLE];
    NGE_FILE *body = streams[PANEL_BODY];
    const char *heading = screen_guide_pages[page].title;   /* fields into locals (T-ZSDCC-1) */
    const char *text = screen_guide_pages[page].text;

    nge_text_cls(title);
    nge_text_ink(NGE_INK_YELLOW, title);
    nge_panel_centre(&panels[PANEL_TITLE], 0, heading, title);
    nge_text_at(0, COL_PAGE, title);
    nge_text_ink(NGE_INK_WHITE, title);
    nge_putu16((uint16_t)(page + 1), title);
    nge_putc('/', title);
    nge_putu16(SCREEN_GUIDE_PAGES, title);

    nge_text_cls(body);
    nge_text_ink(NGE_INK_WHITE, body);
    nge_puts(text, body);
}

void screen_guide_run(void) NGE_BANKED
{
    NGE_FILE *streams[PANEL_COUNT];
    uint8_t page = 0;
    uint8_t next;
    uint8_t c;
    uint8_t ok;

    screen_open();
    ok = nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, PANEL_COUNT, streams);
    nge_util_assert(ok);
    screen_help("Left/right:page 1-4:go to SPACE:back");
    draw(page, streams);
    while ((c = nge_key_ascii(screen_wait_key())) != ' ') {
        next = screen_guide_turn(page, c);
        if (next != page) {
            page = next;
            draw(page, streams);
        }
    }
    nge_panel_close_all(streams, PANEL_COUNT);
    screen_close();
}
