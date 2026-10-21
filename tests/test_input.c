/* nge_input.c, nge_input_setup.c: keyboard snapshot, events, repeat, ASCII, waiting. */
#include "test.h"

#include "input/nge_input.h"

/* The fake keyboard: keys held per half-row (1 = down) and the extended key registers */
static uint8_t rows[8];
static uint8_t ext[2];
static int row0_reads;      /* reads of the first half-row: one per nge_input_update() */
static int press_after;     /* if > 0: hold the key in press_key from this update on */
static uint8_t press_key;
static uint8_t line_low;    /* alternates so nge_frame_wait_vblank() returns */

static void set_key(uint8_t k, uint8_t down)
{
    uint8_t *row = k < 64 ? &rows[k >> 3] : &ext[(k >> 3) - 8];
    uint8_t mask = (uint8_t)(1 << (k & 7));

    *row = down ? (uint8_t)(*row | mask) : (uint8_t)(*row & ~mask);
}

static void release_all(void)
{
    memset(rows, 0, sizeof(rows));
    memset(ext, 0, sizeof(ext));
}

static uint8_t read_port(uint16_t port)
{
    uint8_t half_row = (uint8_t)(port >> 8);
    int r;

    CHECK((port & 0xFF) == 0xFE);
    if (half_row == 0xFE && ++row0_reads == press_after) {
        set_key(press_key, 1);
    }
    for (r = 0; r < 8; r++) {
        if (half_row == (uint8_t)~(1 << r)) {
            return (uint8_t)~rows[r];   /* active low; bits 5-7 read as 1 */
        }
    }
    CHECK(0);   /* one half-row at a time */
    return 0xFF;
}

static uint8_t read_reg(uint8_t reg)
{
    switch (reg) {
    case 0xB0:
        return ext[0];
    case 0xB1:
        return ext[1];
    case 0x1F:          /* active video line, low byte */
        line_low = line_low ? 0 : 200;
        return line_low;
    case 0x1E:          /* high byte */
        return 0;
    default:
        return test_regs[reg];
    }
}

/* All events waiting, as key codes with modifiers, until NGE_KEY_NONE */
static int take(nge_key_event_t *out, int max)
{
    int n = 0;
    nge_key_event_t ev;

    while ((ev = nge_key_get()) != NGE_KEY_NONE && n < max) {
        out[n++] = ev;
    }
    return n;
}

#define EV(k, mods)  ((nge_key_event_t)(((mods) << 8) | (k)))

int main(void)
{
    nge_key_event_t ev[16];
    int n;
    int i;
    int ok;

    test_init();
    test_port_read_hook = read_port;
    test_reg_read_hook = read_reg;

    /* ---- init: register 0x68 bit 4 set, other bits kept ---- */
    test_regs[0x68] = 0x81;
    nge_input_init();
    CHECK(test_regs[0x68] == 0x91);

    /* ---- nothing held ---- */
    nge_input_update();
    CHECK(!nge_key_down(NGE_KEY_A) && !nge_key_pressed(NGE_KEY_A) && !nge_key_released(NGE_KEY_A));
    CHECK(nge_key_get() == NGE_KEY_NONE && nge_key_waiting() == 0);

    /* ---- held, pressed, released ---- */
    set_key(NGE_KEY_A, 1);
    nge_input_update();
    CHECK(nge_key_down(NGE_KEY_A) && nge_key_pressed(NGE_KEY_A) && !nge_key_released(NGE_KEY_A));
    CHECK(nge_key_waiting() == 1 && nge_key_waiting() == 1);     /* counted, not taken */
    CHECK(nge_key_get() == EV(NGE_KEY_A, 0) && nge_key_get() == NGE_KEY_NONE);
    CHECK(nge_key_waiting() == 0);
    nge_input_update();
    CHECK(nge_key_down(NGE_KEY_A) && !nge_key_pressed(NGE_KEY_A));
    CHECK(!nge_key_down(NGE_KEY_S));                         /* a neighbouring key */
    set_key(NGE_KEY_A, 0);
    nge_input_update();
    CHECK(!nge_key_down(NGE_KEY_A) && nge_key_released(NGE_KEY_A));
    nge_input_update();
    CHECK(!nge_key_released(NGE_KEY_A));

    /* Every half-row and the extended keys: combinations held together */
    {
        static const uint8_t keys[] = { NGE_KEY_V, NGE_KEY_G, NGE_KEY_T, NGE_KEY_5, NGE_KEY_6,
                                        NGE_KEY_Y, NGE_KEY_H, NGE_KEY_B, NGE_KEY_UP, NGE_KEY_DELETE };

        for (i = 0; i < (int)sizeof(keys); i++) {
            set_key(keys[i], 1);
        }
        nge_input_update();
        ok = 1;
        for (i = 0; i < (int)sizeof(keys); i++) {
            ok = ok && nge_key_down(keys[i]) && nge_key_pressed(keys[i]);
        }
        CHECK(ok);
        CHECK(!nge_key_down(NGE_KEY_DOWN) && !nge_key_down(NGE_KEY_Z));
        nge_key_flush();
        release_all();
        nge_input_update();
    }

    /* ---- events: several in one frame are queued in scan order ---- */
    set_key(NGE_KEY_RIGHT, 1);
    set_key(NGE_KEY_P, 1);
    set_key(NGE_KEY_Q, 1);
    nge_input_update();
    n = take(ev, 16);
    CHECK(n == 3 && ev[0] == EV(NGE_KEY_Q, 0) && ev[1] == EV(NGE_KEY_P, 0) && ev[2] == EV(NGE_KEY_RIGHT, 0));
    release_all();
    nge_input_update();

    /* ---- shift keys: no events alone, recorded with a key ---- */
    set_key(NGE_KEY_CAPS, 1);
    nge_input_update();
    set_key(NGE_KEY_SYM, 1);
    nge_input_update();
    CHECK(nge_key_get() == NGE_KEY_NONE && nge_key_down(NGE_KEY_CAPS) && nge_key_down(NGE_KEY_SYM));
    set_key(NGE_KEY_SYM, 0);
    set_key(NGE_KEY_A, 1);
    nge_input_update();
    CHECK(nge_key_get() == EV(NGE_KEY_A, NGE_MOD_CAPS));
    set_key(NGE_KEY_SYM, 1);
    set_key(NGE_KEY_B, 1);
    nge_input_update();
    CHECK(nge_key_get() == EV(NGE_KEY_B, NGE_MOD_CAPS | NGE_MOD_SYM));
    release_all();
    nge_input_update();
    CHECK(NGE_KEY_CODE(EV(NGE_KEY_B, 3)) == NGE_KEY_B && NGE_KEY_MODS(EV(NGE_KEY_B, 3)) == 3);

    /* ---- the queue holds NGE_KEY_QUEUE events; later ones are dropped ---- */
    for (i = 0; i < 10; i++) {
        set_key((uint8_t)(NGE_KEY_Q + (i % 5)), 1);
        nge_input_update();
        release_all();
        nge_input_update();
    }
    n = take(ev, 16);
    CHECK(n == NGE_KEY_QUEUE && ev[0] == EV(NGE_KEY_Q, 0) && ev[5] == EV(NGE_KEY_Q, 0));
    set_key(NGE_KEY_Q, 1);
    nge_input_update();
    nge_key_flush();
    CHECK(nge_key_get() == NGE_KEY_NONE);
    release_all();
    nge_input_update();

    /* ---- repeat: after delay frames, then every rate frames ---- */
    nge_input_set_repeat(3, 2);
    set_key(NGE_KEY_X, 1);
    {
        /* Frames 1-8 holding X: events on 1 (press), 4, 6, 8 */
        static const uint8_t expect[] = { 1, 0, 0, 1, 0, 1, 0, 1 };

        ok = 1;
        for (i = 0; i < (int)sizeof(expect); i++) {
            nge_input_update();
            n = take(ev, 16);
            ok = ok && n == expect[i] && (n == 0 || ev[0] == EV(NGE_KEY_X, 0));
        }
        CHECK(ok);
    }
    /* Only into an empty queue: unread events stop repeats piling up */
    for (i = 0; i < 10; i++) {
        nge_input_update();
    }
    CHECK(take(ev, 16) == 1);
    /* The last key pressed repeats, with the shift keys held when it went down */
    set_key(NGE_KEY_C, 1);
    set_key(NGE_KEY_CAPS, 1);
    nge_input_update();
    CHECK(take(ev, 16) == 1 && ev[0] == EV(NGE_KEY_C, NGE_MOD_CAPS));
    set_key(NGE_KEY_CAPS, 0);                                /* released: repeats keep CAPS */
    nge_input_update();
    nge_input_update();
    nge_input_update();
    CHECK(take(ev, 16) == 1 && ev[0] == EV(NGE_KEY_C, NGE_MOD_CAPS));
    /* Holding a key, then pressing a shift key: the repeats keep no shift (5 does not become LEFT) */
    release_all();
    nge_input_update();
    set_key(NGE_KEY_5, 1);
    nge_input_update();
    CHECK(take(ev, 16) == 1 && ev[0] == EV(NGE_KEY_5, 0));
    set_key(NGE_KEY_CAPS, 1);
    for (i = 0; i < 6; i++) {
        nge_input_update();
    }
    n = take(ev, 16);
    CHECK(n >= 1 && ev[0] == EV(NGE_KEY_5, 0) && nge_key_ascii(ev[0]) == '5');
    /* Released: no more repeats */
    release_all();
    for (i = 0; i < 10; i++) {
        nge_input_update();
    }
    CHECK(nge_key_get() == NGE_KEY_NONE);
    /* Delay 0: no repeat */
    nge_input_set_repeat(0, 1);
    set_key(NGE_KEY_X, 1);
    for (i = 0; i < 20; i++) {
        nge_input_update();
    }
    CHECK(take(ev, 16) == 1);
    release_all();
    nge_input_update();
    nge_input_set_repeat(NGE_KEY_REPEAT_DELAY, NGE_KEY_REPEAT_RATE);

    /* ---- ASCII ---- */
    CHECK(nge_key_ascii(EV(NGE_KEY_A, 0)) == 'a');
    CHECK(nge_key_ascii(EV(NGE_KEY_A, NGE_MOD_CAPS)) == 'A');
    CHECK(nge_key_ascii(EV(NGE_KEY_A, NGE_MOD_SYM)) == '~');
    CHECK(nge_key_ascii(EV(NGE_KEY_A, NGE_MOD_CAPS | NGE_MOD_SYM)) == 1);   /* CTRL-A */
    CHECK(nge_key_ascii(EV(NGE_KEY_0, 0)) == '0' && nge_key_ascii(EV(NGE_KEY_6, 0)) == '6');
    CHECK(nge_key_ascii(EV(NGE_KEY_0, NGE_MOD_CAPS)) == 12);                /* DELETE */
    CHECK(nge_key_ascii(EV(NGE_KEY_5, NGE_MOD_CAPS)) == 8);                 /* LEFT */
    CHECK(nge_key_ascii(EV(NGE_KEY_7, NGE_MOD_CAPS)) == 11);                /* UP */
    CHECK(nge_key_ascii(EV(NGE_KEY_ENTER, 0)) == 13 && nge_key_ascii(EV(NGE_KEY_SPACE, 0)) == ' ');
    CHECK(nge_key_ascii(EV(NGE_KEY_B, 0)) == 'b' && nge_key_ascii(EV(NGE_KEY_M, NGE_MOD_SYM)) == '.');
    CHECK(nge_key_ascii(EV(NGE_KEY_UP, 0)) == 11 && nge_key_ascii(EV(NGE_KEY_RIGHT, 0)) == 9);
    CHECK(nge_key_ascii(EV(NGE_KEY_SEMICOLON, 0)) == ';' && nge_key_ascii(EV(NGE_KEY_QUOTE, 0)) == '"');
    CHECK(nge_key_ascii(EV(NGE_KEY_DELETE, 0)) == 12 && nge_key_ascii(EV(NGE_KEY_EDIT, 0)) == 7);
    CHECK(nge_key_ascii(EV(NGE_KEY_EXTEND, 0)) == 0 && nge_key_ascii(EV(NGE_KEY_CAPS, 0)) == 0);
    CHECK(nge_key_ascii(EV(5, 0)) == 0 && nge_key_ascii(EV(NGE_KEY_COUNT, 0)) == 0);   /* no such key */
    CHECK(nge_key_ascii(NGE_KEY_NONE) == 0);

    /* ---- waiting ---- */
    row0_reads = 0;
    CHECK(nge_key_wait(3) == NGE_KEY_NONE && row0_reads == 3);   /* 3 frames, then the timeout */
    set_key(NGE_KEY_K, 1);
    nge_input_update();
    row0_reads = 0;
    CHECK(nge_key_wait(3) == EV(NGE_KEY_K, 0) && row0_reads == 0);   /* already waiting */
    release_all();
    nge_input_update();
    row0_reads = 0;
    press_after = 4;
    press_key = NGE_KEY_L;
    CHECK(nge_key_wait(0) == EV(NGE_KEY_L, 0) && row0_reads == 4);   /* no timeout */
    press_after = 0;
    release_all();
    nge_input_update();

    /* ---- init again: no keys held, no events waiting ---- */
    set_key(NGE_KEY_A, 1);
    nge_input_update();
    nge_input_set_repeat(3, 1);
    CHECK(nge_key_down(NGE_KEY_A) && nge_key_get() != NGE_KEY_NONE);
    nge_input_update();
    nge_input_init();
    CHECK(!nge_key_down(NGE_KEY_A) && !nge_key_pressed(NGE_KEY_A) && nge_key_get() == NGE_KEY_NONE);
    nge_input_update();                                      /* still held: a new press */
    CHECK(nge_key_pressed(NGE_KEY_A) && nge_key_get() != NGE_KEY_NONE);
    release_all();
    nge_input_update();

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(nge_key_down(NGE_KEY_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_key_pressed(NGE_KEY_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_input_set_repeat(1, 0));
    CHECK(test_halt_count == 1);

    return test_summary("input");
}
