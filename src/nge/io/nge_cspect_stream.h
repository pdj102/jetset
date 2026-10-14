#ifndef NGE_CSPECT_STREAM_H
#define NGE_CSPECT_STREAM_H

/**
 * @file nge_cspect_stream.h
 * @brief Output streams on the CSpect emulator's DebugOut console.
 *
 * Output is line buffered: each line is sent to the console on '\n', when the
 * buffer is full, on nge_flush() and on nge_close(). Control codes are
 * dropped, and '%' is replaced by NGE_CSPECT_PERCENT because CSpect cannot
 * print a literal '%'. All open debug streams share one line buffer.
 *
 * The device exists only when NGE_CSPECT is defined; otherwise
 * nge_open_cspect_debug() returns NULL.
 */

#include "io/nge_stream.h"

/** Maximum characters sent to the console in one line. */
#ifndef NGE_CSPECT_LINE_MAX
#define NGE_CSPECT_LINE_MAX 80
#endif

/** Character printed in place of '%'. */
#ifndef NGE_CSPECT_PERCENT
#define NGE_CSPECT_PERCENT '#'
#endif

/**
 * @brief Open an output stream on the CSpect DebugOut console.
 * @return The stream, or NULL if no slot is free or NGE_CSPECT is not defined.
 */
NGE_FILE *nge_open_cspect_debug(void);

#endif
