/**
 * @file nge_util.c
 * @author Paul  Johnson
 * @brief 
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "core/nge_util.h"

#include <stdlib.h>
#include <stdint.h>
#if defined(__SDCC) && !defined(NDEBUG)
#include <arch/zxn.h>
#endif

/***************************************************
 * private defines
 ***************************************************/

/* Stop after a fatal error; the PC tests override this */
#ifndef NGE_UTIL_HALT
#define NGE_UTIL_HALT() for (;;) {}
#endif

/***************************************************
 * private types
 * ***************************************************/

/***************************************************
 * private variables
 * ***************************************************/
static NGE_FILE *debug_stream;

/* Set while reporting, so a failure in the output path cannot recurse */
static uint8_t reporting;

/***************************************************
 * private function prototypes
 ***************************************************/

/***************************************************
 * functions
 ***************************************************/
void nge_util_set_debug_stream(NGE_FILE *stream)
{
    debug_stream = stream;
}

NGE_FILE *nge_util_get_debug_stream(void)
{
    return debug_stream;
}

/* Write "<kind>: <message> FILE: <file> LINE: <line>", then halt. */
static void report_fatal(const char *kind, const char *message, const char *file, unsigned line)
{
    if (!reporting) {
        reporting = 1;
        nge_printf(debug_stream, "%s: %s FILE: %s LINE: %u\n", kind, message, file, line);
        nge_flush(debug_stream);
        /* Never reached on the Next; lets the PC tests report again */
        reporting = 0;
    }
    NGE_UTIL_HALT();
}

void nge_util_assert_f(const char *message, const char *file, unsigned line)
{
    report_fatal("ASSERT FAIL", message, file, line);
}

#if defined(__SDCC) && !defined(NDEBUG)
void nge_util_assert_paged(const char *site) __z88dk_fastcall
{
    if (!reporting) {
        reporting = 1;
        /* Over whatever slot 7 holds: this never returns */
        ZXN_WRITE_REG(REG_MMU7, NGE_UTIL_ASSERT_PAGE);
        nge_printf(debug_stream, "ASSERT FAIL: %s\n", site);
        nge_flush(debug_stream);
    }
    NGE_UTIL_HALT();
}
#endif

void nge_util_info_f(const char *message)
{
    if (!reporting) {
        reporting = 1;
        nge_printf(debug_stream, "INFO: %s\n", message);
        reporting = 0;
    }
}

void nge_util_abort_f(const char *message, const char *file, unsigned line)
{
    report_fatal("ABORT", message, file, line);
}
