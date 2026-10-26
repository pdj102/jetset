/*
 * Lists: common memory, so a banked caller's list can be in its page data.
 */
#include "ui/nge_list.h"

#include "core/nge_util.h"

void nge_list_init(nge_list_t *l, uint8_t count, uint8_t rows)
{
    nge_util_assert(rows != 0);

    l->count = count;
    l->rows = rows;
    l->top = 0;
    l->cursor = 0;
}

uint8_t nge_list_set(nge_list_t *l, uint8_t item)
{
    /* The fields in locals: see T-ZSDCC-1 */
    uint8_t count = l->count;
    uint8_t rows = l->rows;
    uint8_t top = l->top;

    if (count == 0) {
        return NGE_LIST_NONE;
    }
    if (item >= count) {
        item = (uint8_t)(count - 1);
    }
    if (item == l->cursor) {
        return NGE_LIST_NONE;
    }
    l->cursor = item;
    if (item < top) {
        l->top = item;
        return NGE_LIST_SCROLLED;
    }
    if (item >= top + rows) {
        l->top = (uint8_t)(item - rows + 1);
        return NGE_LIST_SCROLLED;
    }
    return NGE_LIST_MOVED;
}

uint8_t nge_list_move(nge_list_t *l, int8_t delta)
{
    int16_t item = (int16_t)l->cursor + delta;

    if (item < 0) {
        item = 0;
    }
    return nge_list_set(l, (item > 255) ? 255 : (uint8_t)item);
}
