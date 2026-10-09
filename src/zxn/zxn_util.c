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

/***************************************************
 * private types
 * ***************************************************/

/***************************************************
 * private variables
 * ***************************************************/

/***************************************************
 * private function prototypes
 ***************************************************/

/***************************************************
 * functions
 ***************************************************/
void zxn_util_assert_f(const char *message, const char *file, unsigned line)
{
    // TODO  text_printf(&g.msg_win, "\nASSERT FAIL: %s FILE:%s LINE:%l", message, file, (unsigned long)line);
    while(1);
}

void zxn_util_info_f(const char *message)
{
    // TODO text_printf(&g.msg_win, "\nINFO: %s", message);
}

void zxn_util_abort_f(const char *message, const char *file, unsigned line)
{
    // TODO text_printf(&g.msg_win, "\nABORT: %s FILE:%s LINE:%l", message, file, (unsigned long)line);
    while(1);
}