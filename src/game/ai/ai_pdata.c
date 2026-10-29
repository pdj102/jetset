/* nge-dataseg: PAGE_131 */
/*
 * The AI module's page data, in page 131 (slot 7 while its code in page
 * 130 runs): each AI entity's state and brain (docs/ai.md, D4).
 * Uninitialised: ai_add() sets an entity's. See "Page data" in
 * docs/memory_map.md.
 */
#include "game/ai/ai_internal.h"

ai_state_t ai_state[NGE_ENTITY_MAX];
ai_brain_t ai_brain[NGE_ENTITY_MAX];
