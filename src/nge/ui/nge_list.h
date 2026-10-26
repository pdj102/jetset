#ifndef NGE_LIST_H
#define NGE_LIST_H

/**
 * @file nge_list.h
 * @brief A list with a cursor, shown a window of rows at a time: which
 *        item is selected, and which items the rows show.
 *
 * The list only keeps the numbers; the caller draws the rows, and says
 * which keys move the cursor. Item i is shown on row i - top while
 * top <= i < top + rows:
 *
 *     nge_list_init(&l, count, 20);
 *     ...
 *     switch (nge_list_move(&l, 1)) {   // down one
 *     case NGE_LIST_MOVED:     draw the old and the new cursor rows
 *     case NGE_LIST_SCROLLED:  draw every row
 *     }
 *
 * Banking: common memory, so the list can be in a banked caller's page
 * data or on its stack.
 */

#include <stdint.h>

/* What a move changed: which rows need drawing again */
#define NGE_LIST_NONE      0   /**< Nothing: the cursor is already at that end, or the list is empty. */
#define NGE_LIST_MOVED     1   /**< The cursor moved within the rows shown: draw the old and new cursor rows. */
#define NGE_LIST_SCROLLED  2   /**< The rows show other items: draw them all. */

/** A list's numbers. */
typedef struct nge_list {
    uint8_t count;    /**< Items, 0 or more. */
    uint8_t rows;     /**< Rows shown at once, 1 or more. */
    uint8_t top;      /**< The item on the first row. */
    uint8_t cursor;   /**< The selected item (0 when the list is empty). */
} nge_list_t;

/** @brief Start a list of count items, shown rows at a time: the cursor on the first. */
void nge_list_init(nge_list_t *l, uint8_t count, uint8_t rows);

/**
 * @brief Move the cursor delta items (negative: up), stopping at the ends,
 *        and scroll to keep it shown.
 * @return NGE_LIST_NONE, NGE_LIST_MOVED or NGE_LIST_SCROLLED.
 */
uint8_t nge_list_move(nge_list_t *l, int8_t delta);

/**
 * @brief Put the cursor on item (clamped to the last), scrolling to show it.
 * @return NGE_LIST_NONE, NGE_LIST_MOVED or NGE_LIST_SCROLLED.
 */
uint8_t nge_list_set(nge_list_t *l, uint8_t item);

#endif
