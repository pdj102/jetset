/**
 * @file zxn_util.c
 * @author Paul  Johnson
 * @brief 
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "zxn_util.h"

#include <stdlib.h>
#include <stdint.h>

/***************************************************
 * private defines
 ***************************************************/

/* Stop after a fatal error; the PC tests override this */
#ifndef ZXN_UTIL_HALT
#define ZXN_UTIL_HALT() for (;;) {}
#endif

/***************************************************
 * private types
 * ***************************************************/

/***************************************************
 * private variables
 * ***************************************************/
static ZXN_FILE *debug_stream;

/* Set while reporting, so a failure in the output path cannot recurse */
static uint8_t reporting;

/***************************************************
 * private function prototypes
 ***************************************************/

/***************************************************
 * functions
 ***************************************************/
void zxn_util_set_debug_stream(ZXN_FILE *stream)
{
    debug_stream = stream;
}

ZXN_FILE *zxn_util_get_debug_stream(void)
{
    return debug_stream;
}

/* Write "<kind>: <message> FILE: <file> LINE: <line>", then halt. */
static void report_fatal(const char *kind, const char *message, const char *file, unsigned line)
{
    if (!reporting) {
        reporting = 1;
        zxn_printf(debug_stream, "%s: %s FILE: %s LINE: %u\n", kind, message, file, line);
        zxn_flush(debug_stream);
        /* Never reached on the Next; lets the PC tests report again */
        reporting = 0;
    }
    ZXN_UTIL_HALT();
}

void zxn_util_assert_f(const char *message, const char *file, unsigned line)
{
    report_fatal("ASSERT FAIL", message, file, line);
}

void zxn_util_info_f(const char *message)
{
    if (!reporting) {
        reporting = 1;
        zxn_printf(debug_stream, "INFO: %s\n", message);
        reporting = 0;
    }
}

void zxn_util_abort_f(const char *message, const char *file, unsigned line)
{
    report_fatal("ABORT", message, file, line);
}
