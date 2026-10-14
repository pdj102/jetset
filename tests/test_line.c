/* nge_line.c, nge_kbd_stream.c, nge_getc(): reading from input streams. */
#include "test.h"

#include "input/nge_input.h"
#include "io/nge_kbd_stream.h"
#include "io/nge_line.h"
#include "io/nge_stream_dev.h"
#include "io/nge_tm_stream.h"

/* A scripted input stream: the characters of script, then NGE_EOF (and -2 in the script is one NGE_EOF) */
static const int *script;
static int script_pos;
static int eofs;

static int script_get(NGE_FILE *stream)
{
    int c;

    (void)stream;
    c = script[script_pos];
    if (c == 0) {
        eofs++;
        return NGE_EOF;
    }
    script_pos++;
    if (c == -2) {
        eofs++;
        return NGE_EOF;
    }
    return c;
}

static void play(const int *s)
{
    script = s;
    script_pos = 0;
    eofs = 0;
}

/* The keyboard, for the keyboard stream: keys held per half-row (1 = down) */
static uint8_t rows[8];
static uint8_t ext[2];
static uint8_t line_low;

static uint8_t read_port(uint16_t port)
{
    int r;

    for (r = 0; r < 8; r++) {
        if ((uint8_t)(port >> 8) == (uint8_t)~(1 << r)) {
            return (uint8_t)~rows[r];
        }
    }
    return 0xFF;
}

static uint8_t read_reg(uint8_t reg)
{
    if (reg == 0xB0 || reg == 0xB1) {
        return ext[reg - 0xB0];
    }
    if (reg == 0x1F) {
        line_low = line_low ? 0 : 200;
        return line_low;
    }
    return reg == 0x1E ? 0 : test_regs[reg];
}

static void key(uint8_t k, uint8_t down)
{
    uint8_t *row = k < 64 ? &rows[k >> 3] : &ext[(k >> 3) - 8];
    uint8_t mask = (uint8_t)(1 << (k & 7));

    *row = down ? (uint8_t)(*row | mask) : (uint8_t)(*row & ~mask);
}

int main(void)
{
    static const int typed[] = { 'a', 'b', 0 };
    static const int deleted[] = { 12, 12, 12, 'x', 8, 'y', 0 };
    static const int ignored[] = { 7, 200, 127, 31, 'z', 0 };
    static const int entered[] = { '!', 13, 'q', 0 };
    static const int too_long[] = { 'a', 'b', 'c', 'd', 'e', 13, 0 };
    static const int waiting[] = { 'h', -2, -2, 'i', 13, 0 };
    NGE_FILE *in;
    NGE_FILE *out;
    NGE_FILE *kbd;
    nge_line_t line;
    char buf[8];
    char small[4];

    test_init();
    test_reg_read_hook = read_reg;
    test_port_read_hook = read_port;

    in = nge_stream_alloc(NULL, script_get, NULL);
    out = test_capture_open();
    CHECK(in != NULL && out != NULL);

    /* ---- nge_getc on output-only, input-only and NULL streams ---- */
    play(typed);
    CHECK(nge_getc(in) == 'a');
    CHECK(nge_getc(out) == NGE_EOF && nge_getc(NULL) == NGE_EOF);
    CHECK(nge_putc('x', in) == NGE_EOF);

    /* ---- line editor: typing, echoed ---- */
    test_capture_reset();
    nge_line_begin(&line, buf, sizeof(buf), in, out);
    CHECK(buf[0] == 0);
    play(typed);
    CHECK(nge_line_update(&line) == 0);
    CHECK(strcmp(buf, "ab") == 0 && strcmp(test_capture_text(), "ab") == 0);

    /* DELETE and backspace remove the last character and its echo; nothing to remove is a no-op */
    test_capture_reset();
    play(deleted);
    CHECK(nge_line_update(&line) == 0);
    CHECK(strcmp(buf, "y") == 0);
    CHECK(strcmp(test_capture_text(), "\b \b\b \bx\b \by") == 0);

    /* Control codes and characters outside 32-126 are ignored */
    test_capture_reset();
    play(ignored);
    nge_line_update(&line);
    CHECK(strcmp(buf, "yz") == 0 && strcmp(test_capture_text(), "z") == 0);

    /* ENTER ends the line, unechoed; the rest stays in the stream */
    test_capture_reset();
    play(entered);
    CHECK(nge_line_update(&line) == 1);
    CHECK(strcmp(buf, "yz!") == 0 && strcmp(test_capture_text(), "!") == 0);
    CHECK(nge_getc(in) == 'q');

    /* The buffer's size limits the line; no echo is fine */
    nge_line_begin(&line, small, sizeof(small), in, NULL);
    play(too_long);
    CHECK(nge_line_update(&line) == 1 && strcmp(small, "abc") == 0);

    /* ---- blocking: nge_getc_wait and nge_gets run frames until input ---- */
    play(waiting);
    CHECK(nge_getc_wait(in) == 'h' && eofs == 0);
    CHECK(nge_getc_wait(in) == 'i' && eofs == 2);
    test_capture_reset();
    play(waiting);
    nge_gets(buf, sizeof(buf), in, out);
    CHECK(strcmp(buf, "hi") == 0 && strcmp(test_capture_text(), "hi") == 0 && eofs == 2);

    /* ---- the keyboard stream: characters of key events ---- */
    nge_input_init();
    kbd = nge_open_keyboard();
    CHECK(kbd != NULL);
    CHECK(nge_getc(kbd) == NGE_EOF);
    key(NGE_KEY_H, 1);
    nge_input_update();
    CHECK(nge_getc(kbd) == 'h' && nge_getc(kbd) == NGE_EOF);
    key(NGE_KEY_H, 0);
    key(NGE_KEY_EXTEND, 1);                                 /* no character: skipped */
    key(NGE_KEY_CAPS, 1);
    key(NGE_KEY_0, 1);                                      /* CAPS+0: DELETE */
    nge_input_update();
    CHECK(nge_getc(kbd) == 12 && nge_getc(kbd) == NGE_EOF);
    CHECK(nge_putc('x', kbd) == NGE_EOF);
    nge_close(kbd);

    /* ---- asserts ---- */
    TEST_EXPECT_HALT(nge_getc_wait(out));                  /* not an input stream: would never return */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_line_begin(&line, NULL, 4, in, out));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_stream_alloc(NULL, NULL, NULL));   /* neither output nor input */
    CHECK(test_halt_count == 1);

    return test_summary("line");
}
