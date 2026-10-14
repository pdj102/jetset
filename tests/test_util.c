/* nge_util.c: assert, abort, info and debug output. */
#include "test.h"

#include <stdio.h>

#include "io/nge_cspect_stream.h"
#include "core/nge_util.h"

static NGE_FILE *cap;
static int nested;

static void assert_inside_output(void)
{
    if (nested) {
        nested = 0;
        nge_util_assert(0 && "nested");
    }
}

int main(void)
{
    unsigned line;

    test_init();
    cap = test_capture_open();
    nge_util_set_debug_stream(cap);
    CHECK(nge_util_get_debug_stream() == cap);

    test_capture_reset();
    nge_util_info("init done");
    CHECK_STR(test_capture_text(), "INFO: init done\n");

    test_capture_reset();
    nge_util_debugf("x=%d id=%u\n", -3, 7);
    CHECK_STR(test_capture_text(), "x=-3 id=7\n");

    /* Assert: one line with expression, file and line, then a halt */
    test_capture_reset();
    line = __LINE__ + 1;
    TEST_EXPECT_HALT(nge_util_assert(1 == 2));
    CHECK(test_halt_count == 1);
    {
        char expect[128];
        snprintf(expect, sizeof(expect), "ASSERT FAIL: 1 == 2 FILE: %s LINE: %u\n", __FILE__, line);
        CHECK_STR(test_capture_text(), expect);
    }

    /* Abort: same format; always compiled in */
    test_capture_reset();
    TEST_EXPECT_HALT(nge_util_abort("out of sprites"));
    CHECK(test_halt_count == 1);
    CHECK(strncmp(test_capture_text(), "ABORT: out of sprites FILE: ", 28) == 0);

    /* A passing assert does nothing */
    test_capture_reset();
    nge_util_assert(2 == 2);
    CHECK_STR(test_capture_text(), "");

    /* An assert while a report is being printed halts without recursing */
    nge_util_set_debug_stream(nge_open_cspect_debug());
    test_cspect_count = 0;
    test_cspect_hook = assert_inside_output;
    nested = 1;
    TEST_EXPECT_HALT(nge_util_assert(1 == 2));
    CHECK(test_halt_count == 1);
    CHECK(test_cspect_count == 1);
    test_cspect_hook = NULL;

    /* No debug stream: output is discarded, the halt still happens */
    nge_util_set_debug_stream(NULL);
    nge_util_info("x");
    nge_util_debugf("y %d\n", 1);
    TEST_EXPECT_HALT(nge_util_assert(0));
    CHECK(test_halt_count == 1);

    return test_summary("util");
}
