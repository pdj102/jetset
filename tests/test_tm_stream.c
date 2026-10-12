/* nge_tm_stream.c: tilemap window streams. */
#include "test.h"

#include <stdlib.h>

#include "output/nge_print.h"
#include "output/nge_tm_stream.h"

static char row[NGE_TILEMAP_WIDTH + 1];

static void clear_map(void)
{
    static const nge_tile_t dot = { '.', 0 };

    nge_tilemap_clear(&dot);
}

/* ---- Directed tests ---- */

static void test_status_bar(void)
{
    static const nge_rect_t hud = { 0, 9, 0, 0 };   /* 10 wide, 1 row */
    NGE_FILE *h;

    clear_map();
    h = nge_open_tilemap(&hud, 0);

    /* Wrap mode: an exact fit does not scroll; the next character does */
    nge_puts("SCORE 1234", h);
    test_tilemap_row(0, 0, 9, row);
    CHECK_STR(row, "SCORE 1234");
    nge_puts("5", h);
    test_tilemap_row(0, 0, 9, row);
    CHECK_STR(row, "5         ");

    /* Clip mode: overflow is dropped and LF does not scroll */
    nge_tm_clip(1, h);
    nge_tm_cls(h);
    nge_puts("SCORE 123456", h);
    nge_putc('\n', h);
    nge_puts("X", h);
    test_tilemap_row(0, 0, 9, row);
    CHECK_STR(row, "XCORE 1234");

    /* Redraw a shorter number, then clear to the end of the line */
    nge_tm_at(0, 6, h);
    nge_putu16(99, h);
    nge_tm_clreol(h);
    test_tilemap_row(0, 0, 9, row);
    CHECK_STR(row, "XCORE 99  ");

    nge_close(h);
}

static void test_message_window(void)
{
    static const nge_rect_t msg = { 20, 24, 0, 2 };  /* 5 wide, 3 rows */
    NGE_FILE *m;

    clear_map();
    m = nge_open_tilemap(&msg, 0);

    /* A full line followed by LF leaves no blank line */
    nge_puts("ABCDE\nFGHIJ\n", m);
    test_tilemap_row(0, 20, 24, row);
    CHECK_STR(row, "ABCDE");
    test_tilemap_row(1, 20, 24, row);
    CHECK_STR(row, "FGHIJ");
    test_tilemap_row(2, 20, 24, row);
    CHECK_STR(row, ".....");

    /* Backspace and clear-to-EOL while a wrap is pending */
    nge_puts("ABCDE", m);
    nge_putc(NGE_CTRL_BS, m);
    nge_putc('z', m);
    test_tilemap_row(2, 20, 24, row);
    CHECK_STR(row, "ABCDz");
    nge_tm_clreol(m);
    test_tilemap_row(2, 20, 24, row);
    CHECK_STR(row, "ABCDz");

    /* Ink sets attribute bits 7-4 and keeps the low bits */
    nge_tm_attr(0x05, m);
    nge_tm_ink(NGE_INK_CYAN, m);
    nge_tm_at(0, 0, m);
    nge_putc('Q', m);
    CHECK(test_tilemap[20].tile_id == 'Q');
    CHECK(test_tilemap[20].tile_attr == ((NGE_INK_CYAN << 4) | 0x05));

    nge_close(m);
}

static void test_ink_palette(void)
{
    int bank;
    int i;
    int ok = 1;

    test_reg_reset();
    nge_tm_load_ink_palette();

    /* Each bank: select tilemap palette 0, set index to bank * 16, 4 entries */
    for (bank = 0, i = 0; bank < 4; bank++) {
        ok = ok && test_reg_log_reg[i] == 0x43 && (test_reg_log_val[i] & 0x70) == 0x30;
        ok = ok && test_reg_log_reg[i + 1] == 0x40 && test_reg_log_val[i + 1] == (NGE_INK_WHITE + bank) * 16;
        i += 2 + 4 * 2;
    }
    CHECK(ok);
    CHECK(test_reg_log_count == 4 * (2 + 4 * 2));
}

static void test_open_asserts(void)
{
    static const nge_rect_t too_wide = { 0, NGE_TILEMAP_WIDTH, 0, 0 };

    TEST_EXPECT_HALT(nge_open_tilemap(&too_wide, 0));
    CHECK(test_halt_count == 1);
}

/* ---- Randomised comparison with a reference model ---- */

/* Recomputes every position from scratch, following the rules in nge_tm_stream.h */
typedef struct {
    int x1, x2, y1, y2, cx, cy, attr, esc, arg, clip;
} ref_t;

static nge_tile_t ref_map[NGE_TILEMAP_WIDTH * NGE_TILEMAP_HEIGHT];

static void ref_set(int x, int y, int id, int attr)
{
    ref_map[y * NGE_TILEMAP_WIDTH + x].tile_id = (uint8_t)id;
    ref_map[y * NGE_TILEMAP_WIDTH + x].tile_attr = (uint8_t)attr;
}

static void ref_lf(ref_t *r)
{
    int x, y;

    if (++r->cy > r->y2) {
        r->cy = r->y2;
        if (!r->clip) {
            for (y = r->y1; y < r->y2; y++)
                for (x = r->x1; x <= r->x2; x++)
                    ref_map[y * NGE_TILEMAP_WIDTH + x] = ref_map[(y + 1) * NGE_TILEMAP_WIDTH + x];
            for (x = r->x1; x <= r->x2; x++)
                ref_set(x, r->y2, 0x20, r->attr);
        }
    }
}

static void ref_put(ref_t *r, int c)
{
    int st = r->esc;
    int x, y;

    r->esc = 0;
    switch (st) {
    case 1: r->attr = (r->attr & 0x0F) | ((c << 4) & 0xF0); return;
    case 2: r->attr = c; return;
    case 3: r->arg = c; r->esc = 4; return;
    case 4:
        r->cx = c > r->x2 - r->x1 ? r->x2 : r->x1 + c;
        r->cy = r->arg > r->y2 - r->y1 ? r->y2 : r->y1 + r->arg;
        return;
    case 5: r->clip = c; return;
    }
    if (c >= 0x20) {
        if (r->cx > r->x2) {
            if (r->clip)
                return;
            r->cx = r->x1;
            ref_lf(r);
        }
        ref_set(r->cx, r->cy, c, r->attr);
        r->cx++;
        return;
    }
    switch (c) {
    case 0x0A: r->cx = r->x1; ref_lf(r); break;
    case 0x0D: r->cx = r->x1; break;
    case 0x08: if (r->cx > r->x1) r->cx--; break;
    case 0x05: for (x = r->cx; x <= r->x2; x++) ref_set(x, r->cy, 0x20, r->attr); break;
    case 0x0C:
        for (y = r->y1; y <= r->y2; y++)
            for (x = r->x1; x <= r->x2; x++)
                ref_set(x, y, 0x20, r->attr);
        r->cx = r->x1;
        r->cy = r->y1;
        break;
    case 0x10: r->esc = 1; break;
    case 0x11: r->esc = 2; break;
    case 0x12: r->esc = 5; break;
    case 0x16: r->esc = 3; break;
    }
}

static void both(NGE_FILE *f, ref_t *r, int c)
{
    nge_putc((uint8_t)c, f);
    ref_put(r, c);
}

static void test_random(void)
{
    /* Three separate windows (one a single column), or one full-screen window */
    static const nge_rect_t areas[4] = { {0, 19, 0, 9}, {20, 39, 10, 20}, {5, 5, 25, 31}, {0, 39, 0, 31} };
    int seed, i, w;
    int mismatches = 0;

    for (seed = 1; seed <= 300; seed++) {
        int nw = (seed % 4 == 0) ? 1 : 3;
        NGE_FILE *f[3];
        ref_t r[3];

        srand(seed);
        memset(test_tilemap_mem, 0, sizeof(ref_map));
        memset(ref_map, 0, sizeof(ref_map));
        for (w = 0; w < nw; w++) {
            const nge_rect_t *a = (nw == 1) ? &areas[3] : &areas[w];
            f[w] = nge_open_tilemap(a, 0);
            r[w] = (ref_t){ a->x1, a->x2, a->y1, a->y2, a->x1, a->y1, 0, 0, 0, 0 };
        }
        for (i = 0; i < 3000; i++) {
            int k = rand() % 100;
            w = rand() % nw;
            if (k < 68)      both(f[w], &r[w], 0x20 + rand() % 96);
            else if (k < 75) both(f[w], &r[w], 0x0A);
            else if (k < 78) both(f[w], &r[w], 0x0D);
            else if (k < 82) both(f[w], &r[w], 0x08);
            else if (k < 83) both(f[w], &r[w], 0x0C);
            else if (k < 85) both(f[w], &r[w], 0x05);
            else if (k < 87) { both(f[w], &r[w], 0x10); both(f[w], &r[w], rand() % 16); }
            else if (k < 89) { both(f[w], &r[w], 0x11); both(f[w], &r[w], rand() % 256); }
            else if (k < 91) { both(f[w], &r[w], 0x12); both(f[w], &r[w], rand() % 3); }
            else if (k < 96) { both(f[w], &r[w], 0x16); both(f[w], &r[w], rand() % 40); both(f[w], &r[w], rand() % 50); }
            else             both(f[w], &r[w], rand() % 32);
        }
        if (memcmp(test_tilemap_mem, ref_map, sizeof(ref_map)) != 0) {
            mismatches++;
        }
        for (w = 0; w < nw; w++) {
            nge_close(f[w]);
        }
    }
    CHECK(mismatches == 0);
}

int main(void)
{
    test_init();
    test_status_bar();
    test_message_window();
    test_ink_palette();
    test_open_asserts();
    test_random();
    return test_summary("tm_stream");
}
