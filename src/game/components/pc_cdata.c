/* nge-dataseg: PAGE_50 */
/*
 * The player character component's variables in slot 0 (the ECS data
 * page, see ecs/nge_ecs.h): one of each, as only the player has the
 * component. Uninitialised: set by pc_add(). See "ECS data in slot 0" in
 * docs/memory_map.md.
 */
#include "game/components/pc.h"

char     pc_name[PC_NAME_MAX + 1];
uint8_t  pc_class;
uint8_t  pc_level;
uint16_t pc_xp;
uint16_t pc_gold;
uint8_t  pc_depth_max;
