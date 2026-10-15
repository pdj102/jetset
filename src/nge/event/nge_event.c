/*
 * Events: a ring buffer of NGE_EVENT_QUEUE events in common memory, and the
 * processing loop. Everything here is in common memory.
 */
#include "event/nge_event.h"

#include <stddef.h>

#include "core/nge_util.h"

typedef char nge_event_queue_check[(NGE_EVENT_QUEUE & (NGE_EVENT_QUEUE - 1)) == 0 &&
                                   NGE_EVENT_QUEUE <= 128 ? 1 : -1];

static nge_event_t queue[NGE_EVENT_QUEUE];
static uint8_t head;    /* oldest event */
static uint8_t count;   /* events waiting */

void nge_event_init(void)
{
    head = 0;
    count = 0;
}

void nge_event_post(uint8_t target, nge_event_id_t id, uint8_t source, uint8_t p1, uint8_t p2)
{
    nge_event_t *e;

    nge_util_assert(count < NGE_EVENT_QUEUE);   /* queue full */
    if (count >= NGE_EVENT_QUEUE) {
        return;   /* NDEBUG: dropped */
    }
    e = &queue[(head + count) & (NGE_EVENT_QUEUE - 1)];
    e->target = target;
    e->id = id;
    e->source = source;
    e->p1 = p1;
    e->p2 = p2;
    count++;
}

uint8_t nge_event_count(void)
{
    return count;
}

uint8_t nge_event_process(nge_event_dispatch_fn dispatch)
{
    nge_event_t e;
    uint8_t n = 0;

    nge_util_assert(dispatch != NULL);

    while (count != 0) {
        if (n == NGE_EVENT_PROCESS_MAX) {
            nge_util_assert(n < NGE_EVENT_PROCESS_MAX);   /* handlers keep posting events */
            break;
        }
        /* A copy, taken off the queue first: handlers may post more */
        e = queue[head];
        head = (head + 1) & (NGE_EVENT_QUEUE - 1);
        count--;
        dispatch(&e);
        n++;
    }
    return n;
}

void nge_event_flush(void)
{
    count = 0;
}
