#ifndef NGE_CSPECT_H
#define NGE_CSPECT_H

/**
 * @file nge_cspect.h
 * @brief CSpect emulator DebugOut extension.
 *
 * Only available when NGE_CSPECT is defined. On real hardware RST $18 calls
 * the ROM instead, so release builds must not define NGE_CSPECT.
 */

#ifdef NGE_CSPECT

/**
 * @brief Print a NUL-terminated string to the CSpect console.
 *
 * Executes RST $18 with HL pointing at the string. Each call is printed on
 * its own console line. CSpect interprets '%' sequences in the string and
 * has no escape for a literal '%' ("%%" prints nothing), so the string must
 * not contain '%'.
 *
 * @param s String to print.
 */
void nge_cspect_print(const char *s) __z88dk_fastcall;

#endif

#endif
