// Pages that hold banked code (NGE_BANKED functions), and their origins.
//
// Read by the linker (-pragma-include in the Makefile). A banked call maps
// the page in bits 16-23 of the function's address into slot 6 and the next
// page into slot 7, so an origin is (page << 16) | $C000: for an 8K section
// PAGE_n (n even) the page is n, for a 16K section BANK_n it is 2n.
// scripts/check-banks.sh fails the build if banked code is in a section
// missing from this list. See docs/memory_map.md.
//
// Page 34: demo map (slot 1)

// Page 40: nge banked code (output formatting, sprite setup). 8K: the linker
// stops the build if it grows into page 41.
#pragma define CRT_ORG_PAGE_40 = 0x28C000

// Page 41: sprite patterns and palette (sprite_data.asm), assembled for slot
// 7 ($E000). Mapped by the nge sprite loaders, which run from page 40.
