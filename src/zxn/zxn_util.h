/**
 * @file zxn_util.h
 * @author Paul Johnson
 * @brief Utility functions for the game engine.
 
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#include "zxn_printf.h"
#include "zxn_stream.h"

/***************************************************
 * public defines
 ***************************************************/

/***************************************************
 * public types
 ***************************************************/

/**
 * @brief Inclusive rectangle coordinates on the ZX Next tilemap.
 *
 * Used in pixels for the clip window and in tiles for tilemap areas.
 */
typedef struct zxn_rect {
    uint8_t x1;
    uint8_t x2;
    uint8_t y1;
    uint8_t y2;
} zxn_rect_t;

/***************************************************
 * public variables
 ***************************************************/

/***************************************************
 * function prototypes
 ***************************************************/
/**
 * @brief Set the stream that receives assert, abort, info and debug output.
 * @param stream Open stream, or NULL to discard the output.
 */
void zxn_util_set_debug_stream(ZXN_FILE *stream);

/**
 * @brief Get the debug output stream.
 * @return The stream set by zxn_util_set_debug_stream(), or NULL.
 */
ZXN_FILE *zxn_util_get_debug_stream(void);

/**
 * @brief Report a failed assertion on the debug stream and halt.
 * @param message Text of the failed expression.
 * @param file Source file name.
 * @param line Source line number.
 */
void zxn_util_assert_f(const char *message, const char *file, unsigned line);

/**
 * @brief Write an information message to the debug stream.
 * @param message Message text.
 */
void zxn_util_info_f(const char *message);

/**
 * @brief Report a fatal error on the debug stream and halt.
 * @param message Message text.
 * @param file Source file name.
 * @param line Source line number.
 */
void zxn_util_abort_f(const char *message, const char *file, unsigned line);

/***************************************************
 * macros
 ***************************************************/
/* If DEBUG defined, enable assert */
#ifdef NDEBUG
    #define zxn_util_assert(_Expression) ((void)0)
    #define zxn_util_info(message) ((void)0)
    #define zxn_util_debugf(...) ((void)0)
#else /* !defined (NDEBUG) */
    #define zxn_util_assert(_Expression) \
        do { \
            if (!(_Expression)) \
                zxn_util_assert_f(#_Expression, __FILE__, __LINE__); \
        } while(0)
    #define zxn_util_info(message) \
        do { \
            zxn_util_info_f((message)); \
        } while (0)
    /* Formatted debug output, e.g. zxn_util_debugf("x=%d\n", x) */
    #define zxn_util_debugf(...) \
        zxn_printf(zxn_util_get_debug_stream(), __VA_ARGS__)

#endif /* !defined (NDEBUG) */


#define zxn_util_abort(message) \
    do { \
        zxn_util_abort_f((message), __FILE__, __LINE__); \
    } while (0)


#endif //ZXN_UTIL_H