#ifndef NGE_BANK_H
#define NGE_BANK_H

/**
 * @file nge_bank.h
 * @brief Marking functions that live in banked memory.
 *
 * A banked function is declared with NGE_BANKED in its header, and its .c
 * file starts with the bank its code goes in:
 *
 *     #ifdef __SDCC
 *     #pragma codeseg BANK_20
 *     #endif
 *
 * Calls then go through z88dk's banked_call, which maps the bank into slots 6
 * and 7 for the duration of the call. The bank must be listed in
 * src/nge/banks.inc. See "Banked code" in docs/memory_map.md for what may and
 * may not be banked.
 */

#ifdef __SDCC
#define NGE_BANKED  __banked
#else
#define NGE_BANKED                /* the PC tests call it directly */
#endif

#endif
