// Page allocation, and the origins of pages that hold banked code.
//
// Read by the linker (-pragma-include in the Makefile). A banked call maps
// the page in bits 16-23 of the function's address into slot 6 and the next
// page into slot 7, so an origin is (page << 16) | $C000: for an 8K section
// PAGE_n (n even) the page is n, for a 16K section BANK_n it is 2n.
// scripts/check-banks.sh fails the build if banked code is in a section
// missing from this list. See "Page allocation" in docs/memory_map.md.
//
// Target: 2MB Next, pages 0-223. Each module with banked code, nge's or the
// game's, gets a pair of pages: the even page for its banked code and its
// string literals and constants (8K, codeseg and constseg PAGE_n), the odd
// page for large variables and tables private to it (its page data file,
// nge_<module>_pdata.c or, in the game, <module>_pdata.c, with the marker
// /* nge-dataseg: PAGE_n */), which is mapped into slot 7 whenever its code
// runs.
//
//   0-31    system: program (4-5), video (10-11), bank 7 (14-15), Layer 2 (16-31)
//   32-39   spare
//   40/41   nge output (reserved: the output path stays in common memory)
//   42/43   nge sprite
//   44/45   nge map: set-up code / (unused); its data is in page 51
//   46/47   nge event (reserved)
//   48-87   nge ecs (20 pairs)
//             48/49  entity and owner: lifecycle and ownership code /
//                    lifecycle bookkeeping
//             50     ECS data, mapped permanently into slot 0 ($0100-$1FFF;
//                    $0000-$00FF is a guard area against null pointers)
//             51     map data, mapped permanently into slot 1 ($2000-$3FFF):
//                    the map's state, tiles and block table (nge_map_cdata.c)
//             52/53  render system: code / sprite component and working data
//             54/55  timer system: code / (unused); its data is in page 50
//   88-99   further nge modules (6 pairs)
//             88/89  input: setup, ASCII tables and waiting / (unused)
//   100-223 game
//             100      free (game assets)
//             101      sprite patterns and palette (slot 7, via the loaders)
//             102-131  game code pairs (15), from 102 up:
//                        102/103  level: setting up the map and generating levels /
//                                 the room list
//                        104/105  title: the title state / (unused)
//                        106/107  gen creature: creature generator / base creatures
//                        108/109  gen item: item generator / base items
//                        110/111  gen shared: generator helpers and game components'
//                                 add functions / (unused)
//                        112/113  actions: the actions / (unused)
//                        114/115  player: commands and screens / (unused)
//             132-223  game assets (maps, graphics, level data)

// nge sprite: setup functions and loaders
#pragma define CRT_ORG_PAGE_42 = 0x2AC000

// nge ecs entity and owner: lifecycle and ownership functions
#pragma define CRT_ORG_PAGE_48 = 0x30C000

// nge ecs data: slot 0, above the guard area. Data only, so no page in the
// address; scripts/check-banks.sh checks it stays in $0100-$1FFF.
#pragma define CRT_ORG_PAGE_50 = 0x0100

// nge ecs render system
#pragma define CRT_ORG_PAGE_52 = 0x34C000

// nge ecs timer system
#pragma define CRT_ORG_PAGE_54 = 0x36C000

// nge input: setup, ASCII tables and waiting
#pragma define CRT_ORG_PAGE_88 = 0x58C000

// game level: setting up and building a level
#pragma define CRT_ORG_PAGE_102 = 0x66C000

// game title: the title state
#pragma define CRT_ORG_PAGE_104 = 0x68C000

// game gen: the creature generator
#pragma define CRT_ORG_PAGE_106 = 0x6AC000

// game gen: the item generator
#pragma define CRT_ORG_PAGE_108 = 0x6CC000

// game gen: helpers shared by the generators, and game components' add functions
#pragma define CRT_ORG_PAGE_110 = 0x6EC000

// game actions: what entities do (game/actions)
#pragma define CRT_ORG_PAGE_112 = 0x70C000

// game player: the player's commands and screens (game/player)
#pragma define CRT_ORG_PAGE_114 = 0x72C000

// nge map: set-up functions
#pragma define CRT_ORG_PAGE_44 = 0x2CC000

// nge map data: slot 1. Data only, so no page in the address;
// scripts/check-banks.sh checks it stays in $2000-$3FFF.
#pragma define CRT_ORG_PAGE_51 = 0x2000
