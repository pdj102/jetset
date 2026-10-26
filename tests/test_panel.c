/* ui/nge_panel: panels filled and opened on the fake Layer 2 (support/test_layer2.c). */
#include "test.h"

#include "io/nge_l2_stream.h"
#include "io/nge_print.h"
#include "io/nge_stream.h"
#include "io/nge_text.h"
#include "layer2/nge_layer2.h"
#include "ui/nge_panel.h"

#define BLACK  0x00
#define BLUE   0x02

static const nge_panel_t panels[3] = {
    { { .x1 = 0, .x2 = 9, .y1 = 0, .y2 = 0 }, BLUE, NGE_PANEL_CLIP },                      /* a title */
    { { .x1 = 0, .x2 = 4, .y1 = 1, .y2 = 2 }, NGE_LAYER2_TRANSPARENT, NGE_PANEL_CLIP },    /* a hole */
    { { .x1 = 5, .x2 = 9, .y1 = 1, .y2 = 2 }, BLACK, NGE_PANEL_SCROLL },                   /* a log */
};

static char row[NGE_LAYER2_COLS + 1];

int main(void)
{
    NGE_FILE *streams[3];
    NGE_FILE *hog[NGE_FILE_MAX];
    uint8_t i;
    uint8_t n;

    test_init();
    nge_layer2_init();

    /* ---- open: each panel filled with its paper on its buffer, a stream on each text panel ---- */
    nge_layer2_fill(1, 0, 0, 9, 2, BLACK);   /* the hole starts opaque */
    CHECK(nge_panel_open_all(1, panels, 3, streams));
    CHECK(streams[0] != NULL && streams[1] == NULL && streams[2] != NULL);
    CHECK(test_layer2_buf[1][0][0].paper == BLUE && test_layer2_buf[1][2][4].paper == NGE_LAYER2_TRANSPARENT);
    CHECK(test_layer2_buf[1][2][9].paper == BLACK);
    CHECK(test_layer2_buf[0][0][0].paper == NGE_LAYER2_TRANSPARENT);   /* not buffer 0 */

    /* ---- the modes: the title clips, the log wraps ---- */
    nge_puts("a very long title", streams[0]);
    test_layer2_buf_row(1, 0, 0, 9, row);
    CHECK_STR(row, "a very lon");
    nge_puts("abcdefg", streams[2]);
    test_layer2_buf_row(1, 2, 5, 9, row);
    CHECK_STR(row, "fg   ");

    /* ---- centred, and too wide to centre ---- */
    nge_text_cls(streams[0]);
    nge_panel_centre(&panels[0], 0, "mid", streams[0]);
    test_layer2_buf_row(1, 0, 0, 9, row);
    CHECK_STR(row, "   mid    ");
    nge_panel_centre(&panels[0], 0, "0123456789AB", streams[0]);
    test_layer2_buf_row(1, 0, 0, 9, row);
    CHECK_STR(row, "0123456789");

    /* ---- close: the slots freed, the entries NULL ---- */
    nge_panel_close_all(streams, 3);
    CHECK(streams[0] == NULL && streams[2] == NULL);
    CHECK(nge_panel_open_all(1, panels, 3, streams));
    nge_panel_close_all(streams, 3);

    /* ---- too few slots: what was opened is closed again ---- */
    for (n = 0; n < NGE_FILE_MAX; n++) {
        hog[n] = nge_open_layer2(0, &panels[0].area, BLACK);
        if (hog[n] == NULL) {
            break;
        }
    }
    nge_close(hog[--n]);          /* one slot free: room for the title, not the log */
    CHECK(!nge_panel_open_all(1, panels, 3, streams));
    CHECK(streams[0] == NULL && streams[1] == NULL && streams[2] == NULL);
    hog[n] = nge_open_layer2(0, &panels[0].area, BLACK);
    CHECK(hog[n] != NULL);        /* the title's slot was given back */
    for (i = 0; i <= n; i++) {
        nge_close(hog[i]);
    }

    /* ---- fill: a panel on its own ---- */
    nge_panel_fill(0, &panels[0]);
    CHECK(test_layer2_buf[0][0][9].paper == BLUE);

    return test_summary("panel");
}
