/**
 * @file zxn_util.h
 * @author Paul Johnson
 * @brief Utility functions for the game engine.
 
 */

#ifndef UTIL_H
#define UTIL_H

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
void zxn_util_assert_f(const char *message, const char *file, unsigned line);

void zxn_util_info_f(const char *message);

void zxn_util_abort_f(const char *message, const char *file, unsigned line);

/***************************************************
 * macros
 ***************************************************/
/* If DEBUG defined, enable assert */
#ifdef NDEBUG
    #define zxn_util_assert(_Expression) ((void)0)
    #define zxn_util_info(message) ((void)0)
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

#endif /* !defined (NDEBUG) */


#define zxn_util_abort(message) \
    do { \
        zxn_util_abort_f((message), __FILE__, __LINE__); \
    } while (0)


#endif //ZXN_UTIL_H