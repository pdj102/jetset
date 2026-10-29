/* nge-dataseg: PAGE_131 */
/*
 * The AI module's page data, in page 131 (slot 7 while its code in page
 * 130 runs): each AI entity's state and brain (docs/ai.md, D4), and its
 * target and goal (docs/ai_default_brain.md, D7).
 * Uninitialised: ai_add() sets an entity's. See "Page data" in
 * docs/memory_map.md.
 */
#include "game/ai/ai_internal.h"

ai_state_t ai_state[NGE_ENTITY_MAX];
ai_brain_t ai_brain[NGE_ENTITY_MAX];
nge_entity_id_t ai_target[NGE_ENTITY_MAX];
uint8_t ai_goal_x[NGE_ENTITY_MAX];
uint8_t ai_goal_y[NGE_ENTITY_MAX];
uint8_t ai_track[NGE_ENTITY_MAX];
