#include "zxn_stream_dev.h"

#include <stddef.h>

static ZXN_FILE streams[ZXN_FILE_MAX];

ZXN_FILE *zxn_stream_alloc(zxn_put_fn put)
{
    ZXN_FILE *s = streams;
    uint8_t i;

    for (i = ZXN_FILE_MAX; i; i--, s++) {
        if (s->put == NULL) {
            s->put = put;
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

void zxn_close(ZXN_FILE *stream)
{
    zxn_util_assert(stream != NULL);

    stream->put = NULL;
}
