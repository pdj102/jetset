/*
 * Development prototypes: banked in page 126 with their literals, in
 * GAME_DEV_KIT builds only (game/dev.h). They pass their literals only to
 * common code; text for Layer 2 is built in buffers on the stack.
 */
#ifdef __SDCC
#pragma codeseg PAGE_126
#pragma constseg PAGE_126
#endif

#include "game/proto/proto.h"

#if GAME_DEV_KIT

#include <arch/zxn.h>
#include <string.h>

#include "core/nge_util.h"
#include "data/sprite_data.h"
#include "hw/nge_frame.h"
#include "input/nge_input.h"
#include "io/nge_tm_stream.h"
#include "layer2/nge_layer2.h"
#include "tilemap/nge_tilemap.h"

#include "game/game.h"
#include "game/screen.h"

/* Colours on Layer 2 (RRRGGGBB) */
#define INK      0xFF   /* white */
#define TITLE    0xFC   /* yellow */
#define PANEL    0x02   /* dark blue */
#define MESSAGE  0x49   /* dark grey */
#define HELP     0x00   /* black */

#define LINE_MAX  41    /* a 40-cell line, and its NUL */
#define US_PER_LINE  64 /* a raster line at 50Hz */

/* The panels, in cells: the same places as the tilemap UI's windows (game/screen.h) */
#define STATS_X1    28
#define STATS_Y2    26
#define MESSAGE_Y1  27
#define MESSAGE_Y2  30
#define HELP_Y      31
#define LAST_COL    39
#define HALF_COL    20   /* timed calls are made in two halves: 0-19, 20-39 (TIME_HALVES()) */

/* The tiles the test sprites stand on, and their patterns */
#define SPRITES  4
static const uint8_t sprite_tile_x[SPRITES] = { 5, 12, 20, 8 };
static const uint8_t sprite_tile_y[SPRITES] = { 5, 9, 14, 20 };
static const uint8_t sprite_pattern[SPRITES] = {
    DEMO_SPRITE_PLAYER, DEMO_SPRITE_RAT, DEMO_SPRITE_SWORD, DEMO_SPRITE_RING,
};

static uint8_t message_count;   /* messages added with M */

/*
 * Timing by the raster line (nge_frame_line()), across frames: the line is
 * read between small steps of the work, each much shorter than a frame, and
 * the lines between reads are added up. The frame's length in lines is
 * measured at the start (calibrate()), not assumed
 */
static uint16_t frame_lines;   /* lines in a frame, as the line register counts them */
static uint16_t last_line;     /* the line at the last read */
static uint32_t lines;         /* lines since timer_start() */

static void calibrate(void)
{
    uint16_t prev;
    uint16_t now;
    uint16_t max = 0;

    nge_frame_wait_vblank();
    prev = nge_frame_line();
    for (;;) {
        now = nge_frame_line();
        if (now < prev) {
            break;   /* wrapped: the frame's last line was max */
        }
        if (now > max) {
            max = now;
        }
        prev = now;
    }
    frame_lines = (uint16_t)(max + 1);
}

static void timer_start(void)
{
    lines = 0;
    last_line = nge_frame_line();
}

/* Between steps: add the lines since the last read */
static void timer_tick(void)
{
    uint16_t now = nge_frame_line();

    lines += (now >= last_line) ? (uint16_t)(now - last_line) : (uint16_t)(now + frame_lines - last_line);
    last_line = now;
}

static uint32_t timer_us(void)
{
    return lines * US_PER_LINE;
}

/*
 * CPU calibration: a loop of known length, timed like the drawing, gives
 * the T-states the CPU really runs a microsecond (28 at 28MHz with no wait
 * states). spin() is 26032 T-states a call: call 17, ld bc 10, 999 passes
 * of the loop at 26 (dec bc 6, ld a,b 4, or c 4, jr nz taken 12), the last
 * at 21 (jr nz not taken 7), ret 10
 */
#define SPIN_T      26032UL   /* T-states a call of spin() */
#define SPIN_CALLS  200       /* about 0.19 s at 28MHz; each call well under a frame at any speed */

static void spin(void) __naked
{
    __asm
        ld bc, 1000
    proto_spin_loop:
        dec bc
        ld a, b
        or c
        jr nz, proto_spin_loop
        ret
    __endasm;
}

/* The CPU's speed in tenths of a T-state a microsecond (280 at 28MHz): the ticks add about 1% */
static uint16_t cpu_speed(void)
{
    uint16_t i;

    timer_start();
    for (i = 0; i < SPIN_CALLS; i++) {
        spin();
        timer_tick();
    }
    return (uint16_t)(SPIN_T * SPIN_CALLS * 10 / timer_us());
}

/* Append the decimal digits of v to s, a buffer of size bytes */
static void add_u32(char *s, uint8_t size, uint32_t v)
{
    char digits[11];
    uint8_t n = 0;
    uint8_t len = (uint8_t)strlen(s);

    do {
        digits[n++] = (char)('0' + (uint8_t)(v % 10));
        v /= 10;
    } while (v != 0);
    while (n != 0 && len + 1 < size) {
        s[len++] = digits[--n];
    }
    s[len] = '\0';
}

/* Append t to s, a buffer of size bytes, cut to fit */
static void add_str(char *s, uint8_t size, const char *t)
{
    strncat(s, t, size - 1 - strlen(s));
}

/* Text from this page onto Layer 2, through a copy on the stack */
static void text(uint8_t col, uint8_t row, const char *s, uint8_t ink, uint8_t paper)
{
    char line[LINE_MAX];   /* on the stack: common memory, for nge_layer2_text() */

    line[0] = '\0';
    add_str(line, sizeof(line), s);
    nge_layer2_text(col, row, line, ink, paper);
}

/* One panel row and one console line: label, then a time in microseconds */
static void show_time(uint8_t row, const char *label, uint32_t us)
{
    char line[LINE_MAX];

    line[0] = '\0';
    add_str(line, sizeof(line), label);
    add_u32(line, sizeof(line), us);
    nge_layer2_text(STATS_X1 + 1, row, line, INK, PANEL);
    nge_util_debugf("layer2: %s us\n", line);
}

/*
 * A new message on the bottom line of the message panel, the others up one,
 * as the game would draw it (a scroll, a line): the time it took. The
 * scroll is made in two halves, timed apart, so no part runs past a frame
 * between line reads (TIME_HALVES())
 */
static uint32_t add_message(void)
{
    char line[LINE_MAX];

    line[0] = '\0';
    add_str(line, sizeof(line), "Message ");
    add_u32(line, sizeof(line), ++message_count);
    add_str(line, sizeof(line), ": the quick brown fox jumps");
    nge_frame_wait_vblank();
    timer_start();
    nge_layer2_scroll_up(0, MESSAGE_Y1, HALF_COL - 1, MESSAGE_Y2, MESSAGE);
    timer_tick();
    nge_layer2_scroll_up(HALF_COL, MESSAGE_Y1, LAST_COL, MESSAGE_Y2, MESSAGE);
    timer_tick();
    nge_layer2_text(0, MESSAGE_Y2, line, INK, MESSAGE);   /* up to 40 characters: well under a frame */
    timer_tick();
    return timer_us();
}

/* The whole tilemap: floor, a wall every 8 tiles, and the row number at the left, so scrolling shows */
static void draw_test_map(void)
{
    nge_tile_t t;
    uint8_t x;
    uint8_t y;

    for (y = 0; y < NGE_TILEMAP_HEIGHT; y++) {
        for (x = 0; x < NGE_TILEMAP_WIDTH; x++) {
            if (x % 8 == 0 || y % 8 == 0) {
                t.tile_id = '#';
                t.tile_attr = (uint8_t)(NGE_INK_YELLOW << 4);
            } else {
                t.tile_id = '.';
                t.tile_attr = (uint8_t)(NGE_INK_CYAN << 4);
            }
            nge_tilemap_set(x, y, &t);
        }
        t.tile_id = (uint8_t)('0' + y % 10);
        t.tile_attr = (uint8_t)(NGE_INK_WHITE << 4);
        nge_tilemap_set(1, y, &t);
    }
}

/* Sprite positions are 9 bits: one off the left or top edge by k pixels is at 512 - k */
#define SPRITE_COORD_MASK  0x1FF
#define SPRITE_PIXELS      16

/*
 * The test sprites on their tiles, through the scroll. The tilemap wraps at
 * 320 x 256, so a tile's screen position is (its pixel - scroll) wrapped to
 * the screen; a sprite that would then run off the right or bottom edge is
 * placed off the left or top instead (wrapped to 9 bits), so the part of it
 * still on the screen shows. Placing it at the right edge would put it
 * outside the sprite clip window, and it would vanish whole
 */
static void place_sprites(uint16_t sx, uint8_t sy)
{
    uint8_t i;
    int16_t x;
    int16_t y;

    for (i = 0; i < SPRITES; i++) {
        x = (int16_t)(((uint16_t)sprite_tile_x[i] * 8 + 320 - sx) % 320);
        y = (int16_t)(((uint16_t)sprite_tile_y[i] * 8 + 256 - sy) % 256);
        if (x > 320 - SPRITE_PIXELS) {
            x -= 320;
        }
        if (y > 256 - SPRITE_PIXELS) {
            y -= 256;
        }
        screen_overlay_set(i, (uint16_t)x & SPRITE_COORD_MASK, (uint16_t)y & SPRITE_COORD_MASK, sprite_pattern[i]);
    }
}

/* -1, 0 or 1: the arrow keys (or CAPS+5-8, or h j k l) held for one axis */
static int8_t held(uint8_t less, uint8_t less_caps, uint8_t less_key, uint8_t more, uint8_t more_caps,
                   uint8_t more_key)
{
    uint8_t caps = nge_key_down(NGE_KEY_CAPS);

    if (nge_key_down(less) || nge_key_down(less_key) || (caps && nge_key_down(less_caps))) {
        return -1;
    }
    if (nge_key_down(more) || nge_key_down(more_key) || (caps && nge_key_down(more_caps))) {
        return 1;
    }
    return 0;
}

/*
 * The time of a call made in two halves, columns 0 to HALF_COL - 1, then
 * HALF_COL to LAST_COL, with the line read between: each half must end
 * within a frame of the last read, so totals up to about two frames (40 ms)
 * are exact. One call reaching past a frame read short: a 20.5 ms scroll
 * read 0.5 ms (2026-10-20)
 */
#define TIME_HALVES(us, first, second) \
    do {                               \
        nge_frame_wait_vblank();       \
        timer_start();                 \
        first;                         \
        timer_tick();                  \
        second;                        \
        timer_tick();                  \
        (us) = timer_us();             \
    } while (0)

#define FILL(x1, x2)    nge_layer2_fill((x1), MESSAGE_Y1, (x2), MESSAGE_Y2, MESSAGE)
#define SCROLL(x1, x2)  nge_layer2_scroll_up((x1), MESSAGE_Y1, (x2), MESSAGE_Y2, MESSAGE)

/*
 * Time the Layer 2 drawing and show the times: a fill of the 4 message
 * rows by the CPU and by the DMA, a scroll of them (by the CPU: see
 * nge_layer2_draw.c), a line of 40 characters, and a message (a scroll and
 * a line)
 */
static void measure(void)
{
    char line[LINE_MAX];
    uint32_t fill_cpu_us;
    uint32_t fill_dma_us;
    uint32_t scroll_us;
    uint32_t line_us;
    uint32_t message_us;
    uint16_t speed;

    calibrate();
    speed = cpu_speed();

    nge_layer2_set_dma(0);
    TIME_HALVES(fill_cpu_us, FILL(0, HALF_COL - 1), FILL(HALF_COL, LAST_COL));
    nge_layer2_set_dma(1);
    TIME_HALVES(fill_dma_us, FILL(0, HALF_COL - 1), FILL(HALF_COL, LAST_COL));
    TIME_HALVES(scroll_us, SCROLL(0, HALF_COL - 1), SCROLL(HALF_COL, LAST_COL));   /* always by the CPU */

    /* A line of 40 characters: two calls of 20 */
    line[0] = '\0';
    add_str(line, sizeof(line), "01234567890123456789");
    TIME_HALVES(line_us, nge_layer2_text(0, MESSAGE_Y2, line, INK, MESSAGE),
                nge_layer2_text(HALF_COL, MESSAGE_Y2, line, INK, MESSAGE));

    message_us = add_message();

    text(STATS_X1 + 1, 4, "Times, us:", TITLE, PANEL);
    show_time(5, "fillC ", fill_cpu_us);
    show_time(6, "fillD ", fill_dma_us);
    show_time(7, "scroll", scroll_us);
    show_time(8, "glyph ", line_us / NGE_LAYER2_COLS);
    show_time(9, "line  ", line_us);
    show_time(10, "msg   ", message_us);
    text(STATS_X1 + 1, 13, "C cpu, D dma", INK, PANEL);
    text(STATS_X1 + 1, 14, "4 rows, 40ch", INK, PANEL);
    nge_util_debugf("layer2: frame %u lines, turbo reg %u\n", frame_lines, ZXN_READ_REG(REG_TURBO_MODE));
    line[0] = '\0';
    add_str(line, sizeof(line), "CPU ");
    add_u32(line, sizeof(line), speed / 10);
    add_str(line, sizeof(line), ".");
    add_u32(line, sizeof(line), speed % 10);
    add_str(line, sizeof(line), " T/us");
    nge_layer2_text(STATS_X1 + 1, 16, line, INK, PANEL);
    nge_util_debugf("layer2: cpu %s (%u calls of %u T)\n", line, SPIN_CALLS, (uint16_t)SPIN_T);
}

void proto_layer2_run(void) NGE_BANKED
{
    nge_key_event_t ev;
    uint16_t sx = 0;
    uint8_t sy = 0;
    uint32_t us;
    char buf[12];   /* a time, for the console */

    game_frame_start();   /* a map waiting to be redrawn is drawn now, not over the test map */
    draw_test_map();
    nge_layer2_init();
    nge_layer2_fill(STATS_X1, 0, LAST_COL, STATS_Y2, PANEL);
    nge_layer2_fill(0, HELP_Y, LAST_COL, HELP_Y, HELP);
    text(29, 1, "Layer 2 UI", TITLE, PANEL);
    text(29, 2, "prototype", TITLE, PANEL);
    measure();
    nge_layer2_fill(0, MESSAGE_Y1, LAST_COL, MESSAGE_Y2, MESSAGE);
    text(0, MESSAGE_Y2, "Arrows scroll the map a pixel a frame.", INK, MESSAGE);
    text(0, HELP_Y, "Arrows: scroll   M: message   ENTER: back", INK, HELP);
    place_sprites(sx, sy);
    message_count = 0;
    nge_key_flush();

    for (;;) {
        game_frame_start();                  /* the sprites placed for (sx, sy) */
        nge_tilemap_set_scroll(sx, sy);      /* and the tilemap's scroll, in the same blanking period */
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE && nge_key_ascii(ev) == 13) {
            break;
        }
        if (ev != NGE_KEY_NONE && (nge_key_ascii(ev) == 'm' || nge_key_ascii(ev) == 'M')) {
            us = add_message();
            buf[0] = '\0';
            add_u32(buf, sizeof(buf), us);
            nge_util_debugf("layer2: message %s us\n", buf);
        }
        sx = (uint16_t)((sx + 320 + held(NGE_KEY_LEFT, NGE_KEY_5, NGE_KEY_H, NGE_KEY_RIGHT, NGE_KEY_8, NGE_KEY_L)) % 320);
        sy = (uint8_t)(sy + held(NGE_KEY_UP, NGE_KEY_7, NGE_KEY_K, NGE_KEY_DOWN, NGE_KEY_6, NGE_KEY_J));
        place_sprites(sx, sy);
        game_frame_end();
    }

    /* Back as the title expects: no overlay sprites, no scroll, no Layer 2, the tilemap UI afresh */
    for (sy = 0; sy < SCREEN_OVERLAY_COUNT; sy++) {
        screen_overlay_hide(sy);
    }
    game_frame_start();
    nge_tilemap_set_scroll(0, 0);
    nge_layer2_show(0);
    screen_init();
    game_frame_end();
    nge_key_flush();
}

#else

typedef char proto_none_t;   /* a release build has no prototypes, and C wants a declaration */

#endif
