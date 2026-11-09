/* nge-dataseg: PAGE_139 */
/*
 * The attributes module's page data, in page 139 (slot 7 while its code
 * in page 138 runs): each attribute's base (attrs.h), moved out of slot 0
 * as only this module reads them every turn (docs/slot0_tables.md, D1).
 * Uninitialised: the components' add functions set an entity's through
 * effects_put_base(). See "Page data" in docs/memory_map.md.
 *
 * mods_of() (attrs_bank.c) maps page 121 into slot 7 over these tables
 * while it copies a definition, and maps them back.
 */
#include "game/attrs/attrs_internal.h"

uint8_t health_hp_max_base[NGE_ENTITY_MAX];
uint8_t health_immune_base[NGE_ENTITY_MAX];
uint8_t health_resist_base[NGE_ENTITY_MAX];
uint8_t health_vuln_base[NGE_ENTITY_MAX];
uint8_t health_ac_base[NGE_ENTITY_MAX];
uint8_t stats_speed_base[NGE_ENTITY_MAX];
uint8_t stats_ability_base[ABIL_COUNT][NGE_ENTITY_MAX];
uint8_t mana_mp_max_base[NGE_ENTITY_MAX];
