/* ui/nge_list: the cursor, its ends and scrolling to keep it shown. */
#include "test.h"

#include "ui/nge_list.h"

int main(void)
{
    nge_list_t l;

    test_init();

    /* ---- a list longer than its rows: 10 items, 4 rows ---- */
    nge_list_init(&l, 10, 4);
    CHECK(l.top == 0 && l.cursor == 0);
    CHECK(nge_list_move(&l, -1) == NGE_LIST_NONE && l.cursor == 0);    /* at the top already */
    CHECK(nge_list_move(&l, 1) == NGE_LIST_MOVED && l.cursor == 1 && l.top == 0);
    CHECK(nge_list_move(&l, 2) == NGE_LIST_MOVED && l.cursor == 3 && l.top == 0);   /* the last row */
    CHECK(nge_list_move(&l, 1) == NGE_LIST_SCROLLED && l.cursor == 4 && l.top == 1);
    CHECK(nge_list_move(&l, 100) == NGE_LIST_SCROLLED && l.cursor == 9 && l.top == 6);   /* stops at the end */
    CHECK(nge_list_move(&l, 1) == NGE_LIST_NONE && l.cursor == 9 && l.top == 6);
    CHECK(nge_list_move(&l, -3) == NGE_LIST_MOVED && l.cursor == 6 && l.top == 6);   /* the first row */
    CHECK(nge_list_move(&l, -1) == NGE_LIST_SCROLLED && l.cursor == 5 && l.top == 5);
    CHECK(nge_list_move(&l, -100) == NGE_LIST_SCROLLED && l.cursor == 0 && l.top == 0);

    /* ---- set: anywhere, clamped to the last ---- */
    CHECK(nge_list_set(&l, 2) == NGE_LIST_MOVED && l.cursor == 2 && l.top == 0);
    CHECK(nge_list_set(&l, 2) == NGE_LIST_NONE);
    CHECK(nge_list_set(&l, 200) == NGE_LIST_SCROLLED && l.cursor == 9 && l.top == 6);

    /* ---- a list shorter than its rows never scrolls ---- */
    nge_list_init(&l, 3, 20);
    CHECK(nge_list_move(&l, 5) == NGE_LIST_MOVED && l.cursor == 2 && l.top == 0);

    /* ---- an empty list: nothing moves ---- */
    nge_list_init(&l, 0, 4);
    CHECK(nge_list_move(&l, 1) == NGE_LIST_NONE && l.cursor == 0);
    CHECK(nge_list_set(&l, 3) == NGE_LIST_NONE && l.cursor == 0);

    /* ---- one row ---- */
    nge_list_init(&l, 3, 1);
    CHECK(nge_list_move(&l, 1) == NGE_LIST_SCROLLED && l.top == 1);

    TEST_EXPECT_HALT(nge_list_init(&l, 3, 0));
    CHECK(test_halt_count == 1);

    return test_summary("list");
}
