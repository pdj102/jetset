/*
 * Panels: common memory, so a banked caller's tables and strings stay
 * readable. Each record is copied before the banked fill maps its page
 * over the caller's.
 */
#include "ui/nge_panel.h"

#include <stddef.h>
#include <string.h>

#include "core/nge_util.h"
#include "io/nge_l2_stream.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "layer2/nge_layer2.h"

void nge_panel_fill(uint8_t b, const nge_panel_t *p)
{
    nge_panel_t q = *p;

    nge_layer2_fill(b, q.area.x1, q.area.y1, q.area.x2, q.area.y2, q.paper);
}

uint8_t nge_panel_open_all(uint8_t b, const nge_panel_t *panels, uint8_t n, NGE_FILE **streams)
{
    nge_panel_t p;
    NGE_FILE *s;
    uint8_t i;

    for (i = 0; i < n; i++) {
        streams[i] = NULL;
    }
    for (i = 0; i < n; i++) {
        p = panels[i];
        nge_layer2_fill(b, p.area.x1, p.area.y1, p.area.x2, p.area.y2, p.paper);
        if (p.paper != NGE_LAYER2_TRANSPARENT) {
            s = nge_open_layer2(b, &p.area, p.paper);
            if (s == NULL) {
                nge_panel_close_all(streams, n);
                return 0;
            }
            nge_text_clip(p.mode, s);
            streams[i] = s;
        }
    }
    return 1;
}

void nge_panel_close_all(NGE_FILE **streams, uint8_t n)
{
    uint8_t i;

    for (i = 0; i < n; i++) {
        nge_close(streams[i]);   /* NULL is ignored */
        streams[i] = NULL;
    }
}

void nge_panel_centre(const nge_panel_t *p, uint8_t row, const char *s, NGE_FILE *stream)
{
    uint8_t width = (uint8_t)(p->area.x2 - p->area.x1 + 1);
    size_t len = strlen(s);

    nge_text_at(row, (len < width) ? (uint8_t)((width - len) / 2) : 0, stream);
    nge_puts(s, stream);
}
