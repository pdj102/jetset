/**
 * @file nge_util.h
 * @author Paul Johnson
 * @brief Utility functions for the game engine.
 
 */

#ifndef NGE_UTIL_H
#define NGE_UTIL_H

#include <stdint.h>

#include "core/nge_types.h"
/* The assert and debug output is printed to a stream: the one place where
   core depends on a higher layer (see src/nge/README.md) */
#include "output/nge_printf.h"
#include "output/nge_stream.h"

/***************************************************
 * public defines
 ***************************************************/

/***************************************************
 * public types
 ***************************************************/

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
void nge_util_set_debug_stream(NGE_FILE *stream);

/**
 * @brief Get the debug output stream.
 * @return The stream set by nge_util_set_debug_stream(), or NULL.
 */
NGE_FILE *nge_util_get_debug_stream(void);

/**
 * @brief Report a failed assertion on the debug stream and halt.
 * @param message Text of the failed expression.
 * @param file Source file name.
 * @param line Source line number.
 */
void nge_util_assert_f(const char *message, const char *file, unsigned line);

/**
 * @brief Write an information message to the debug stream.
 * @param message Message text.
 */
void nge_util_info_f(const char *message);

/**
 * @brief Report a fatal error on the debug stream and halt.
 * @param message Message text.
 * @param file Source file name.
 * @param line Source line number.
 */
void nge_util_abort_f(const char *message, const char *file, unsigned line);

/***************************************************
 * macros
 ***************************************************/
/* If DEBUG defined, enable assert */
#ifdef NDEBUG
    #define nge_util_assert(_Expression) ((void)0)
    #define nge_util_info(message) ((void)0)
    #define nge_util_debugf(...) ((void)0)
#else /* !defined (NDEBUG) */
    #define nge_util_assert(_Expression) \
        do { \
            if (!(_Expression)) \
                nge_util_assert_f(#_Expression, __FILE__, __LINE__); \
        } while(0)
    #define nge_util_info(message) \
        do { \
            nge_util_info_f((message)); \
        } while (0)
    /* Formatted debug output, e.g. nge_util_debugf("x=%d\n", x) */
    #define nge_util_debugf(...) \
        nge_printf(nge_util_get_debug_stream(), __VA_ARGS__)

#endif /* !defined (NDEBUG) */


#define nge_util_abort(message) \
    do { \
        nge_util_abort_f((message), __FILE__, __LINE__); \
    } while (0)


#endif //NGE_UTIL_H