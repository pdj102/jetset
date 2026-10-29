/*
 * The AI's way to act: banked in page 130 with the rest of the module.
 * Apart from ai_bank.c so that code needing only the component (the
 * generators and their tests) doesn't need the actions.
 */
#ifdef __SDCC
#pragma codeseg PAGE_130
#pragma constseg PAGE_130
#endif

#include "game/ai/ai_internal.h"

uint8_t ai_try(uint8_t action, act_t *a) NGE_BANKED
{
    if (act_do(action, a, ACT_CHECK) == ACT_SUCCESS) {
        act_do(action, a, ACT_DO);
    }
    return a->used;
}
