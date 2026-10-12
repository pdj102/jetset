// Banks that hold banked code (NGE_BANKED functions), and their origins.
//
// Read by the linker (-pragma-include in the Makefile). A banked call maps
// the page in bits 16-23 of the function's address into slot 6 and the next
// page into slot 7, so a bank's origin is (page << 16) | $C000, where bank n
// is pages 2n and 2n+1. scripts/check-banks.sh fails the build if banked
// code is in a bank missing from this list. See docs/memory_map.md.
//
// Bank 16 (pages 32-33): demo sprite data (mapped by hand, not listed here)
// Page 34: demo map (slot 1)

// Bank 20 (pages 40-41): nge output formatting and setup code
#pragma define CRT_ORG_BANK_20 = 0x28C000
