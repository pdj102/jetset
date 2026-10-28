/*
 * The quest item's data in common memory: read by the actions (page 112),
 * the messages (page 118) and the game. Its functions are in quest_bank.c.
 */
#include "game/quest/quest.h"

nge_entity_id_t quest_item = NGE_ENTITY_INVALID;
