#ifndef ZXN_CSPECT_STREAM_H
#define ZXN_CSPECT_STREAM_H

/**
 * @file zxn_cspect_stream.h
 * @brief Output streams on the CSpect emulator's DebugOut console.
 *
 * Output is line buffered: each line is sent to the console on '\n', when the
 * buffer is full, on zxn_flush() and on zxn_close(). Control codes are
 * dropped, and '%' is replaced by ZXN_CSPECT_PERCENT because CSpect cannot
 * print a literal '%'. All open debug streams share one line buffer.
 *
 * The device exists only when ZXN_CSPECT is defined; otherwise
 * zxn_open_cspect_debug() returns NULL.
 */

#include "zxn_stream.h"

/** Maximum characters sent to the console in one line. */
#ifndef ZXN_CSPECT_LINE_MAX
#define ZXN_CSPECT_LINE_MAX 80
#endif

/** Character printed in place of '%'. */
#ifndef ZXN_CSPECT_PERCENT
#define ZXN_CSPECT_PERCENT '#'
#endif

/**
 * @brief Open an output stream on the CSpect DebugOut console.
 * @return The stream, or NULL if no slot is free or ZXN_CSPECT is not defined.
 */
ZXN_FILE *zxn_open_cspect_debug(void);

#endif
