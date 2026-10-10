#include "zxn_stream_dev.h"

#include <stddef.h>

static ZXN_FILE streams[ZXN_FILE_MAX];

ZXN_FILE *zxn_stream_alloc(zxn_put_fn put, zxn_flush_fn flush)
{
    ZXN_FILE *s = streams;
    uint8_t i;

    for (i = ZXN_FILE_MAX; i; i--, s++) {
        if (s->put == NULL) {
            s->put = put;
            s->flush = flush;
            return s;
        }
    }
    return NULL;
}

int zxn_putc(uint8_t c, ZXN_FILE *stream)
{
    if (stream == NULL || stream->put == NULL) {
        return ZXN_EOF;
    }
    return stream->put(c, stream);
}

void zxn_flush(ZXN_FILE *stream)
{
    if (stream != NULL && stream->put != NULL && stream->flush != NULL) {
        stream->flush(stream);
    }
}

void zxn_close(ZXN_FILE *stream)
{
    /* NULL is allowed, so streams that failed to open need no check */
    if (stream == NULL) {
        return;
    }
    zxn_flush(stream);
    stream->put = NULL;
}
