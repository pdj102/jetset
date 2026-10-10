#include "zxn_sprite.h"

#include <arch/zxn.h>
#include <string.h>

/* Register 0x15 bit 5: apply the clip window in over-border mode (X in 2-pixel units) */
#define ZXN_RSLS_CLIP_OVER_BORDER  0x20

/* attr2 */
#define ATTR2_X8     0x01  /* X bit 8 */
#define ATTR2_FLAGS  0xFE  /* palette offset, mirror, rotate */
/* attr3 */
#define ATTR3_VISIBLE  0x80
#define ATTR3_ATTR4    0x40  /* attr4 is present: always set, so every sprite is 5 bytes */
#define ATTR3_PATTERN  0x3F
/* attr4 (anchor sprite) */
#define ATTR4_4BIT     0x80  /* H: 4-bit pattern */
#define ATTR4_N6       0x40  /* second 128 bytes of the 256-byte pattern slot */
#define ATTR4_SCALE    0x1E
#define ATTR4_Y8       0x01  /* Y bit 8 */

/* One sprite's attributes, in the order they are sent to the hardware */
typedef struct zxn_sprite {
    uint8_t x;
    uint8_t y;
    uint8_t attr2;
    uint8_t attr3;
    uint8_t attr4;
} zxn_sprite_t;

static zxn_sprite_t sprites[ZXN_SPRITE_MAX];

/* Sprites 0 to count-1 are sent by zxn_sprite_update() */
static uint8_t count;

/* Get a sprite for changing; it is sent from now on. */
static zxn_sprite_t *sprite_use(uint8_t id)
{
    zxn_util_assert(id < ZXN_SPRITE_MAX);

    if (id >= count) {
        count = id + 1;
    }
    return &sprites[id];
}

static void sprite_place(zxn_sprite_t *s, uint16_t x, uint16_t y)
{
    s->x = (uint8_t)x;
    s->y = (uint8_t)y;
    s->attr2 = (s->attr2 & ATTR2_FLAGS) | (uint8_t)((x >> 8) & ATTR2_X8);
    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_Y8) | (uint8_t)((y >> 8) & ATTR4_Y8);
}

static void sprite_pattern(zxn_sprite_t *s, uint8_t pattern)
{
    zxn_util_assert(pattern < ZXN_SPRITE_PATTERNS);

    s->attr3 = (s->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | pattern;
    s->attr4 &= (uint8_t)~(ATTR4_4BIT | ATTR4_N6);
}

static void sprite_pattern4(zxn_sprite_t *s, uint8_t pattern)
{
    zxn_util_assert(pattern < ZXN_SPRITE_PATTERNS4);

    s->attr3 = (s->attr3 & ATTR3_VISIBLE) | ATTR3_ATTR4 | (pattern >> 1);
    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_N6) | ATTR4_4BIT | (uint8_t)((pattern & 1) ? ATTR4_N6 : 0);
}

void zxn_sprite_init(void)
{
    memset(sprites, 0, sizeof(sprites));
    for (count = 0; count < ZXN_SPRITE_MAX; count++) {
        sprites[count].attr3 = ATTR3_ATTR4;
    }
    zxn_sprite_update();
    count = 0;

    zxn_sprite_set_transparency(ZXN_SPRITE_TRANSPARENT);
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) | RSLS_SPRITES_VISIBLE | RSLS_SPRITES_OVER_BORDER);
}

void zxn_sprite_set_transparency(uint8_t index)
{
    ZXN_WRITE_REG(REG_SPRITE_TRANSPARENCY_INDEX, index);
}

void zxn_sprite_load_patterns(uint8_t first, const uint8_t *data, uint8_t n)
{
    uint16_t bytes = (uint16_t)n * ZXN_SPRITE_PATTERN_SIZE;

    zxn_util_assert(data != NULL);
    zxn_util_assert((uint16_t)first + n <= ZXN_SPRITE_PATTERNS);

    IO_SPRITE_SLOT = first;
    while (bytes--) {
        IO_SPRITE_PATTERN = *data++;
    }
}

void zxn_sprite_load_patterns4(uint8_t first, const uint8_t *data, uint8_t n)
{
    uint16_t bytes = (uint16_t)n * ZXN_SPRITE_PATTERN4_SIZE;

    zxn_util_assert(data != NULL);
    zxn_util_assert((uint16_t)first + n <= ZXN_SPRITE_PATTERNS4);

    /* Bit 7 starts the upload half way into the 256-byte slot */
    IO_SPRITE_SLOT = (first >> 1) | (uint8_t)((first & 1) << 7);
    while (bytes--) {
        IO_SPRITE_PATTERN = *data++;
    }
}

void zxn_sprite_set(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags)
{
    zxn_sprite_t *s = sprite_use(id);

    s->attr2 = flags & ATTR2_FLAGS;
    sprite_place(s, x, y);
    sprite_pattern(s, pattern);
    s->attr3 |= ATTR3_VISIBLE;
}

void zxn_sprite_set4(uint8_t id, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags)
{
    zxn_sprite_t *s = sprite_use(id);

    s->attr2 = flags & ATTR2_FLAGS;
    sprite_place(s, x, y);
    sprite_pattern4(s, pattern);
    s->attr3 |= ATTR3_VISIBLE;
}

void zxn_sprite_move(uint8_t id, uint16_t x, uint16_t y)
{
    sprite_place(sprite_use(id), x, y);
}

void zxn_sprite_set_pattern(uint8_t id, uint8_t pattern)
{
    sprite_pattern(sprite_use(id), pattern);
}

void zxn_sprite_set_pattern4(uint8_t id, uint8_t pattern)
{
    sprite_pattern4(sprite_use(id), pattern);
}

void zxn_sprite_set_flags(uint8_t id, uint8_t flags)
{
    zxn_sprite_t *s = sprite_use(id);

    s->attr2 = (s->attr2 & ATTR2_X8) | (flags & ATTR2_FLAGS);
}

void zxn_sprite_set_scale(uint8_t id, uint8_t scale)
{
    zxn_sprite_t *s = sprite_use(id);

    s->attr4 = (s->attr4 & (uint8_t)~ATTR4_SCALE) | (scale & ATTR4_SCALE);
}

void zxn_sprite_show(uint8_t id, uint8_t on)
{
    zxn_sprite_t *s = sprite_use(id);

    if (on) {
        s->attr3 |= ATTR3_VISIBLE;
    } else {
        s->attr3 &= (uint8_t)~ATTR3_VISIBLE;
    }
}

void zxn_sprite_hide_all(void)
{
    uint8_t i;

    for (i = 0; i < count; i++) {
        sprites[i].attr3 &= (uint8_t)~ATTR3_VISIBLE;
    }
}

void zxn_sprite_update(void)
{
    const uint8_t *p = (const uint8_t *)sprites;
    uint16_t bytes = (uint16_t)count * sizeof(zxn_sprite_t);

    IO_SPRITE_SLOT = 0;
    while (bytes--) {
        IO_SPRITE_ATTRIBUTE = *p++;
    }
}

void zxn_sprite_set_clip(const zxn_rect_t *r)
{
    zxn_util_assert(r != NULL);
    zxn_util_assert(r->x1 <= r->x2 && r->y1 <= r->y2);

    ZXN_WRITE_REG(REG_CLIP_WINDOW_CONTROL, RCWC_RESET_SPRITE_CLIP_INDEX);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->x1);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->x2);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->y1);
    ZXN_WRITE_REG(REG_CLIP_WINDOW_SPRITES, r->y2);
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) | ZXN_RSLS_CLIP_OVER_BORDER);
}

void zxn_sprite_clear_clip(void)
{
    ZXN_WRITE_REG(REG_SPRITE_LAYER_SYSTEM,
                  ZXN_READ_REG(REG_SPRITE_LAYER_SYSTEM) & (uint8_t)~ZXN_RSLS_CLIP_OVER_BORDER);
}

uint8_t zxn_sprite_status(void)
{
    return IO_SPRITE_FLAGS & (ZXN_SPRITE_STATUS_COLLISION | ZXN_SPRITE_STATUS_LINE_FULL);
}
