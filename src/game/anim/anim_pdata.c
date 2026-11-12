/* nge-dataseg: PAGE_127 */
/*
 * The anim module's page data, in page 127 (slot 7 while its code in page
 * 126 runs): the creature kinds' sequences, a copy of their kind records'
 * (docs/animations.md, D8), and an area's remembered effect and centre.
 * Uninitialised: anim_init() sets them. See "Page data" in
 * docs/memory_map.md.
 */
#include "game/anim/anim_internal.h"

gen_seq_t anim_seqs[CR_COUNT][GEN_SEQ_COUNT];

uint8_t anim_area_ef;
uint8_t anim_area_held;
uint16_t anim_area_x;
uint16_t anim_area_y;
