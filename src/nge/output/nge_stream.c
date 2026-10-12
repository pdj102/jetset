#include "output/nge_stream_dev.h"

#include <stddef.h>

static NGE_FILE streams[NGE_FILE_MAX];

NGE_FILE *nge_stream_alloc(nge_put_fn put, nge_flush_fn flush)
{
    NGE_FILE *s = streams;
    uint8_t i;

    for (i = NGE_FILE_MAX; i; i--, s++) {
        if (s->put == NULL) {
            s->put = put;
            s->flush = flush;
            return s;
        }
    }
    return NULL;
}

int nge_putc(uint8_t c, NGE_FILE *stream)
{
    if (stream == NULL || stream->put == NULL) {
        return NGE_EOF;
    }
    return stream->put(c, stream);
}

void nge_flush(NGE_FILE *stream)
{
    if (stream != NULL && stream->put != NULL && stream->flush != NULL) {
        stream->flush(stream);
    }
}

void nge_close(NGE_FILE *stream)
{
    /* NULL is allowed, so streams that failed to open need no check */
    if (stream == NULL) {
        return;
    }
    nge_flush(stream);
    stream->put = NULL;
}
