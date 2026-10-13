#ifndef NGE_BANK_H
#define NGE_BANK_H

/**
 * @file nge_bank.h
 * @brief Marking functions that live in banked memory.
 *
 * A banked function is declared with NGE_BANKED in its header, and its .c
 * file starts with the page its code, string literals and constants go in:
 *
 *     #ifdef __SDCC
 *     #pragma codeseg PAGE_42
 *     #pragma constseg PAGE_42
 *     #endif
 *
 * Its literals and constants may be passed to common code (the output path,
 * nge_util) or code in the same page, never to another bank, and must not be
 * kept after the call returns.
 *
 * Calls then go through z88dk's banked_call, which maps the page into slot 6
 * and the next page into slot 7 for the duration of the call. Use 8K
 * sections in even pages (PAGE_n, n even) or 16K sections (BANK_n). The bank must be listed in
 * src/nge/banks.inc. See "Banked code" in docs/memory_map.md for what may and
 * may not be banked.
 */

#ifdef __SDCC
#define NGE_BANKED  __banked
#else
#define NGE_BANKED                /* the PC tests call it directly */
#endif

#endif
