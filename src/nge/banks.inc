// Page allocation, and the origins of pages that hold banked code.
//
// Read by the linker (-pragma-include in the Makefile). A banked call maps
// the page in bits 16-23 of the function's address into slot 6 and the next
// page into slot 7, so an origin is (page << 16) | $C000: for an 8K section
// PAGE_n (n even) the page is n, for a 16K section BANK_n it is 2n.
// scripts/check-banks.sh fails the build if banked code is in a section
// missing from this list. See "Page allocation" in docs/memory_map.md.
//
// Target: 2MB Next, pages 0-223. Each module gets a pair of pages: the even
// page for its banked code and its string literals and constants (8K,
// codeseg and constseg PAGE_n), the odd page for large variables and tables
// private to it (its page data file, nge_<module>_pdata.c, with the marker
// /* nge-dataseg: PAGE_n */), which is mapped into slot 7 whenever its code
// runs.
//
//   0-31    system: program (4-5), video (10-11), bank 7 (14-15), Layer 2 (16-31)
//   32-39   spare
//   40/41   nge output (reserved: the output path stays in common memory)
//   42/43   nge sprite
//   44/45   nge map (reserved: map code is per-frame and in common memory)
//   46/47   nge event (reserved)
//   48-67   nge ecs (10 pairs)
//             48/49  entity: lifecycle code / lifecycle bookkeeping
//   68-87   further nge modules (reserved: 10 pairs)
//   88-223  game code and assets
//             88  demo map (slot 1)
//             89  demo sprite patterns and palette (slot 7, via the loaders)

// nge sprite: setup functions and loaders
#pragma define CRT_ORG_PAGE_42 = 0x2AC000

// nge ecs entity: lifecycle functions
#pragma define CRT_ORG_PAGE_48 = 0x30C000
