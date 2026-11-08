/*
 * VFX: banked in page 92, with the slots and sprite records as page data
 * in page 93 (nge_vfx_pdata.c), which banked_call maps into slot 7 for
 * each call. It reads the map's camera and window in common memory, and
 * calls the sprite module's common functions with its records in page 93.
 */
#ifdef __SDCC
#pragma codeseg PAGE_92
#pragma constseg PAGE_92
#endif

#include "vfx/nge_vfx_internal.h"

#include "core/nge_util.h"
#include "map/nge_map.h"
#include "tilemap/nge_tilemap.h"

typedef char nge_vfx_slots_check[(NGE_VFX_SLOTS >= 1 && NGE_VFX_SLOTS < NGE_VFX_NONE) ? 1 : -1];

#define SPRITE_PIXELS  16
#define TURN_FLAGS     (NGE_SPRITE_MIRROR_X | NGE_SPRITE_MIRROR_Y | NGE_SPRITE_ROTATE)

/* Free a slot: its sprite is hidden by the update that frees it */
static void release(uint8_t i)
{
    nge_vfx_kind[i] = NGE_VFX_FREE;
    nge_vfx_active--;
}

void nge_vfx_clear(void) NGE_BANKED
{
    uint8_t i;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        nge_vfx_kind[i] = NGE_VFX_FREE;
    }
    nge_vfx_active = 0;
    /* Every record hidden, and counted as shown: the next upload hides them all */
    nge_sprite_clear(nge_vfx_attrs, NGE_VFX_SLOTS);
    nge_vfx_drawn = 0;
    nge_vfx_shown = NGE_VFX_SLOTS;
}

void nge_vfx_init(uint8_t first_sprite) NGE_BANKED
{
    nge_util_assert((uint16_t)first_sprite + NGE_VFX_SLOTS <= NGE_SPRITE_MAX);

    nge_vfx_sprite = first_sprite;
    nge_vfx_clear();
}

/* A sprite at map pixel (x, y) would show at least partly in the view: rx and ry are its place relative to the view */
static uint8_t on_view(uint16_t x, uint16_t y, int16_t *rx, int16_t *ry)
{
    int16_t vx = (int16_t)(x - NGE_MAP_CAMERA_PX());
    int16_t vy = (int16_t)(y - NGE_MAP_CAMERA_PY());

    *rx = vx;
    *ry = vy;
    return vx > -SPRITE_PIXELS && vx < (int16_t)nge_map.view_w * NGE_MAP_TILE_PIXELS &&
           vy > -SPRITE_PIXELS && vy < (int16_t)nge_map.view_h * NGE_MAP_TILE_PIXELS;
}

static uint8_t in_view(uint16_t x, uint16_t y)
{
    int16_t rx;
    int16_t ry;

    return on_view(x, y, &rx, &ry);
}

/* A free slot, or NGE_VFX_NONE */
static uint8_t take(void)
{
    uint8_t i;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        if (nge_vfx_kind[i] == NGE_VFX_FREE) {
            return i;
        }
    }
    return NGE_VFX_NONE;
}

/* Fill a slot's pattern run from a look, at its first pattern */
static void set_look(uint8_t i, const nge_vfx_look_t *look)
{
    /* Locals, not fields: see T-ZSDCC-1 */
    uint8_t count = look->count;
    uint8_t rate = look->rate;

    nge_util_assert(count >= 1 && rate >= 1);

    nge_vfx_first[i] = look->first;
    nge_vfx_count[i] = count;
    nge_vfx_rate[i] = rate;
    nge_vfx_flags[i] = look->flags;
    nge_vfx_mode[i] = look->mode;
    nge_vfx_tick[i] = 0;
    nge_vfx_frame[i] = 0;
    nge_vfx_delay[i] = 0;
}

/*
 * Updates after the next one until the bolts flying to (x, y) have all
 * arrived: a burst with this delay first shows on the update that frees
 * the last of them. 0 if none is flying there
 */
static uint8_t arrival(uint16_t x, uint16_t y)
{
    uint8_t i;
    uint8_t speed;
    uint16_t frames;
    uint8_t most = 0;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        if (nge_vfx_kind[i] != NGE_VFX_BOLT || nge_vfx_ex[i] != x || nge_vfx_ey[i] != y) {
            continue;
        }
        speed = nge_vfx_speed[i];
        frames = (nge_vfx_left[i] + speed - 1) / speed;   /* the update that frees it */
        frames--;
        if (frames > 0xFF) {
            frames = 0xFF;
        }
        if ((uint8_t)frames > most) {
            most = (uint8_t)frames;
        }
    }
    return most;
}

uint8_t nge_vfx_burst(uint16_t x, uint16_t y, const nge_vfx_look_t *look, uint8_t delay) NGE_BANKED
{
    uint8_t i;
    uint8_t wait;

    if (!in_view(x, y)) {
        return NGE_VFX_NONE;
    }
    i = take();
    if (i == NGE_VFX_NONE) {
        return NGE_VFX_NONE;
    }
    wait = arrival(x, y);   /* an impact waits for its bolt (D4) */
    set_look(i, look);
    nge_vfx_kind[i] = NGE_VFX_BURST;
    nge_vfx_x[i] = x;
    nge_vfx_y[i] = y;
    nge_vfx_delay[i] = delay > wait ? delay : wait;
    nge_vfx_active++;
    return i;
}

/*
 * A directed bolt's rotate and mirror flags for the nearest of the 8
 * directions to (dx, dy), from a pattern pointing right and one pointing
 * up-right (*diagonal set for the second). Rotation is clockwise, before
 * the mirrors
 */
static uint8_t dir_flags(int16_t dx, int16_t dy, uint8_t *diagonal)
{
    uint16_t adx = (uint16_t)(dx < 0 ? -dx : dx);
    uint16_t ady = (uint16_t)(dy < 0 ? -dy : dy);

    *diagonal = 0;
    if (2 * ady < adx) {
        return dx < 0 ? NGE_SPRITE_MIRROR_X : 0;                     /* left, right */
    }
    if (2 * adx < ady) {
        return dy < 0 ? (NGE_SPRITE_ROTATE | NGE_SPRITE_MIRROR_Y)    /* up */
                      : NGE_SPRITE_ROTATE;                           /* down */
    }
    *diagonal = 1;
    return (uint8_t)((dx < 0 ? NGE_SPRITE_MIRROR_X : 0) | (dy > 0 ? NGE_SPRITE_MIRROR_Y : 0));
}

uint8_t nge_vfx_bolt(uint16_t x, uint16_t y, uint16_t ex, uint16_t ey, uint8_t speed,
                     const nge_vfx_look_t *look) NGE_BANKED
{
    int16_t dx = (int16_t)(ex - x);
    int16_t dy = (int16_t)(ey - y);
    uint16_t adx;
    uint16_t ady;
    uint8_t i;
    uint8_t diagonal;
    /* Locals, not fields: see T-ZSDCC-1 */
    uint8_t mode = look->mode;
    uint8_t first = look->first;
    uint8_t count = look->count;
    uint8_t flags = look->flags;

    nge_util_assert(speed >= 1);

    if ((dx == 0 && dy == 0) || (!in_view(x, y) && !in_view(ex, ey))) {
        return NGE_VFX_NONE;
    }
    i = take();
    if (i == NGE_VFX_NONE) {
        return NGE_VFX_NONE;
    }
    set_look(i, look);
    if (mode & NGE_VFX_DIRECTED) {
        nge_vfx_flags[i] = (uint8_t)((flags & (uint8_t)~TURN_FLAGS) | dir_flags(dx, dy, &diagonal));
        if (diagonal) {
            nge_vfx_first[i] = (uint8_t)(first + count);
        }
    }
    adx = (uint16_t)(dx < 0 ? -dx : dx);
    ady = (uint16_t)(dy < 0 ? -dy : dy);
    nge_vfx_kind[i] = NGE_VFX_BOLT;
    nge_vfx_x[i] = x;
    nge_vfx_y[i] = y;
    nge_vfx_ex[i] = ex;
    nge_vfx_ey[i] = ey;
    nge_vfx_dx[i] = adx;
    nge_vfx_dy[i] = ady;
    nge_vfx_sx[i] = dx < 0 ? -1 : dx > 0 ? 1 : 0;
    nge_vfx_sy[i] = dy < 0 ? -1 : dy > 0 ? 1 : 0;
    nge_vfx_left[i] = adx >= ady ? adx : ady;
    nge_vfx_err[i] = (int16_t)(nge_vfx_left[i] / 2);
    nge_vfx_speed[i] = speed;
    nge_vfx_active++;
    return i;
}

uint8_t nge_vfx_arrival(uint16_t x, uint16_t y) NGE_BANKED
{
    return arrival(x, y);
}

uint8_t nge_vfx_busy(void) NGE_BANKED
{
    return nge_vfx_active != 0;
}

/* Move a bolt speed pixels along its line: 1 when it has arrived */
static uint8_t fly(uint8_t i)
{
    uint8_t n = nge_vfx_speed[i];
    uint16_t adx = nge_vfx_dx[i];
    uint16_t ady = nge_vfx_dy[i];
    uint16_t left = nge_vfx_left[i];
    int16_t err = nge_vfx_err[i];
    uint16_t x = nge_vfx_x[i];
    uint16_t y = nge_vfx_y[i];
    int8_t sx = nge_vfx_sx[i];
    int8_t sy = nge_vfx_sy[i];

    while (n-- > 0 && left > 0) {
        left--;
        if (adx >= ady) {
            x += sx;
            err -= (int16_t)ady;
            if (err < 0) {
                y += sy;
                err += (int16_t)adx;
            }
        } else {
            y += sy;
            err -= (int16_t)adx;
            if (err < 0) {
                x += sx;
                err += (int16_t)ady;
            }
        }
    }
    nge_vfx_x[i] = x;
    nge_vfx_y[i] = y;
    nge_vfx_left[i] = left;
    nge_vfx_err[i] = err;
    return left == 0;
}

/*
 * Advance a slot's pattern run by a frame: a burst plays it once (1 when
 * it has ended), a bolt loops it. Each pattern shows for rate frames
 */
static uint8_t animate(uint8_t i, uint8_t once)
{
    if (nge_vfx_tick[i] >= nge_vfx_rate[i]) {
        nge_vfx_tick[i] = 0;
        if (++nge_vfx_frame[i] >= nge_vfx_count[i]) {
            if (once) {
                return 1;
            }
            nge_vfx_frame[i] = 0;
        }
    }
    nge_vfx_tick[i]++;
    return 0;
}

/* Advance a slot by a frame: 1 if it shows a sprite this frame */
static uint8_t advance(uint8_t i)
{
    uint8_t kind = nge_vfx_kind[i];

    if (nge_vfx_delay[i] != 0) {
        nge_vfx_delay[i]--;
        return (nge_vfx_mode[i] & NGE_VFX_HOLD) != 0;   /* a stand-in shows while it waits */
    }
    if (kind == NGE_VFX_BURST && (nge_vfx_mode[i] & NGE_VFX_HOLD)) {
        release(i);   /* its wait is over */
        return 0;
    }
    if (kind == NGE_VFX_BOLT && fly(i)) {
        release(i);   /* arrived */
        return 0;
    }
    if (animate(i, kind == NGE_VFX_BURST)) {
        release(i);
        return 0;
    }
    return 1;
}

void nge_vfx_update(void) NGE_BANKED
{
    uint16_t base_x = (uint16_t)nge_map.window.x1 * NGE_TILEMAP_TILE_PIXELS;
    uint16_t base_y = (uint16_t)nge_map.window.y1 * NGE_TILEMAP_TILE_PIXELS;
    uint8_t i;
    uint8_t drawn = 0;
    int16_t rx;
    int16_t ry;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        if (nge_vfx_kind[i] == NGE_VFX_FREE || !advance(i) ||
            !on_view(nge_vfx_x[i], nge_vfx_y[i], &rx, &ry)) {
            nge_sprite_clear(&nge_vfx_attrs[i], 1);
            continue;
        }
        /* Left of or above the window wraps round to the 9-bit coordinates' far side, as in the render system */
        nge_sprite_set(&nge_vfx_attrs[i], base_x + (uint16_t)rx, base_y + (uint16_t)ry,
                       (uint8_t)(nge_vfx_first[i] + nge_vfx_frame[i]), nge_vfx_flags[i], nge_vfx_mode[i]);
        drawn++;
    }
    nge_vfx_drawn = drawn;
}

void nge_vfx_upload(void) NGE_BANKED
{
    if (nge_vfx_drawn != 0 || nge_vfx_shown != 0) {
        nge_sprite_upload(nge_vfx_sprite, nge_vfx_attrs, NGE_VFX_SLOTS);
    }
    nge_vfx_shown = nge_vfx_drawn;
}
