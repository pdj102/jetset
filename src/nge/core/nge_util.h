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
#include "io/nge_printf.h"
#include "io/nge_stream.h"

/***************************************************
 * public defines
 ***************************************************/
/* The odd page, for slot 7, of the assert messages of common files: see
   "Assert messages" in docs/memory_map.md */
#define NGE_UTIL_ASSERT_PAGE 101

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

#if defined(__SDCC) && !defined(NDEBUG)
/**
 * @brief Report a failed assertion of a common file and halt.
 * @param site The message, "<expression> FILE: <file> LINE: <line>", in
 *             page NGE_UTIL_ASSERT_PAGE, which this maps into slot 7.
 */
void nge_util_assert_paged(const char *site) __z88dk_fastcall;
#endif

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
    #if defined(__SDCC) && defined(NGE_UTIL_ASSERT_PAGED)
        /*
         * A common file (the Makefile sets NGE_UTIL_ASSERT_PAGED for every
         * file without #pragma codeseg): the message goes in its page and
         * the site is 6 bytes. Written in assembly because C can't choose a
         * section for one literal. The label is the line number: two asserts
         * on one line stop the build. A " in the expression does too.
         */
        #define NGE_UTIL_STR2(x) #x
        #define NGE_UTIL_STR(x) NGE_UTIL_STR2(x)
        #define NGE_UTIL_ASSERT_SITE2(text, line) \
            __asm__("ld hl,l_nge_assert_" #line "\n" \
                    "call _nge_util_assert_paged\n" \
                    "section PAGE_" NGE_UTIL_STR(NGE_UTIL_ASSERT_PAGE) "\n" \
                    "l_nge_assert_" #line ":\n" \
                    "defm \"" text " FILE: " __FILE__ " LINE: " #line "\", 0\n" \
                    "section code_compiler\n")
        #define NGE_UTIL_ASSERT_SITE(text, line) NGE_UTIL_ASSERT_SITE2(text, line)
        #define nge_util_assert(_Expression) \
            do { \
                if (!(_Expression)) { \
                    NGE_UTIL_ASSERT_SITE(#_Expression, __LINE__); \
                } \
            } while(0)
    #else
        #define nge_util_assert(_Expression) \
            do { \
                if (!(_Expression)) \
                    nge_util_assert_f(#_Expression, __FILE__, __LINE__); \
            } while(0)
    #endif
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