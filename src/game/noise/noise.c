/*
 * The noise module's common part: the mask and the event filter, called
 * for every event. Only events whose row has a loudness reach the banked
 * code (noise_bank.c, page 142).
 */
#include "game/noise/noise.h"

uint8_t noise_mask[(NOISE_EVENTS + 7) / 8];

void noise_on_event(const nge_event_t *e)
{
    uint8_t i = (uint8_t)(e->id - NGE_EVENT_USER);   /* below NGE_EVENT_USER wraps past the table */
    uint8_t bit = 1;
    uint8_t n;

    if (i >= NOISE_EVENTS) {
        return;
    }
    for (n = i & 7; n != 0; n--) {   /* a loop, not a variable shift: no library routine */
        bit <<= 1;
    }
    if (noise_mask[i >> 3] & bit) {
        noise_event(e);
    }
}
