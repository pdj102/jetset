/* nge_printf.c, checked against the C library's snprintf. */
#include "test.h"

#include <stdio.h>

#include "io/nge_printf.h"

static NGE_FILE *cap;

/* Format with both and compare text and return value */
#define SAME(...) \
    do { \
        char ref_[128]; \
        int ref_ret_ = snprintf(ref_, sizeof(ref_), __VA_ARGS__); \
        int ret_; \
        test_capture_reset(); \
        ret_ = nge_printf(cap, __VA_ARGS__); \
        CHECK_STR(test_capture_text(), ref_); \
        CHECK(ret_ == ref_ret_); \
    } while (0)

int main(void)
{
    test_init();
    cap = test_capture_open();

    SAME("%d", 0);
    SAME("%d", -32768);
    SAME("%d", 32767);
    SAME("%i", -5);
    SAME("%u", 65535);
    SAME("%u", 0);
    SAME("%5u", 42);
    SAME("%05u", 42);
    SAME("%-5u|", 42);
    SAME("%5d", -42);
    SAME("%05d", -42);
    SAME("%-5d|", -42);
    SAME("%2d", 12345);
    SAME("%x", 0xbeef);
    SAME("%X", 0xBEEF);
    SAME("%04X", 0xA);
    SAME("%x", 0);
    SAME("%c%c", 'h', 'i');
    SAME("%3c|", 'z');
    SAME("%s", "hello");
    SAME("%-8s|", "hi");
    SAME("%8s|", "hi");
    SAME("100%% %s", "done");
    SAME("x=%d y=%d", 3, -4);
    SAME("%s:%u", "main.c", 123);

    /* Behaviour where snprintf differs or is undefined */
    test_capture_reset();
    CHECK(nge_printf(cap, "%q") == 1);
    CHECK_STR(test_capture_text(), "q");
    test_capture_reset();
    CHECK(nge_printf(cap, "end%") == 4);
    CHECK_STR(test_capture_text(), "end%");
    test_capture_reset();
    nge_printf(cap, "%s", (char *)NULL);
    CHECK_STR(test_capture_text(), "(null)");

    return test_summary("printf");
}
