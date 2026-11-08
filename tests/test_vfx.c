/* nge_vfx.c: bolts and bursts, their timing, impacts waiting for bolts, directions, the view and the pool. */
#include "test.h"

#include <arch/zxn.h>

#include "map/nge_map.h"
#include "sprite/nge_sprite.h"
#include "vfx/nge_vfx.h"
#include "vfx/nge_vfx_internal.h"

#define ATTR_BYTES  5
#define FIRST       68     /* the first hardware sprite: after the render system's and the game's overlay sprites */

/* The whole tilemap: a view of 20 x 15 map tiles, from the window's top-left at (0, 0) */
static const nge_rect_t window = { .x1 = 0, .x2 = 39, .y1 = 0, .y2 = 29 };

/* A run of 3 patterns from 10, 2 frames each */
static const nge_vfx_look_t splash = { 10, 3, 2, NGE_SPRITE_PALETTE(4), NGE_SPRITE_PATTERN4 };
/* One pattern pointing right at 20, up-right at 21 */
static const nge_vfx_look_t arrow = { 20, 1, 1, NGE_SPRITE_PALETTE(3), NGE_SPRITE_PATTERN4 | NGE_VFX_DIRECTED };
/* A round orb: no direction */
static const nge_vfx_look_t orb = { 30, 1, 1, NGE_SPRITE_PALETTE(10), NGE_SPRITE_PATTERN4 };

/* Update and upload, returning the number of records sent (their bytes start at test_port_log_val[1]) */
static int frame(void)
{
    nge_vfx_update();
    test_port_reset();
    nge_vfx_upload();
    if (test_port_log_count == 0) {
        return 0;
    }
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == FIRST);
    CHECK((test_port_log_count - 1) % ATTR_BYTES == 0);
    return (test_port_log_count - 1) / ATTR_BYTES;
}

/* Byte n of a slot's sprite record, in the hardware's order */
static uint8_t attr(int slot, int n)
{
    return ((const uint8_t *)&nge_vfx_attrs[slot])[n];
}

static int visible(int slot)
{
    return (attr(slot, 3) & 0x80) != 0;
}

/* A 4-bit pattern's number, from attr3 and attr4's N6 */
static int pattern(int slot)
{
    return ((attr(slot, 3) & 0x3F) << 1) | ((attr(slot, 4) & 0x40) ? 1 : 0);
}

static int sprite_x(int slot)
{
    return attr(slot, 0) | ((attr(slot, 2) & 0x01) << 8);
}

static int sprite_y(int slot)
{
    return attr(slot, 1) | ((attr(slot, 4) & 0x01) << 8);
}

static int sprite_flags(int slot)
{
    return attr(slot, 2) & 0xFE;
}

static int any_visible(void)
{
    int i;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        if (visible(i)) {
            return 1;
        }
    }
    return 0;
}

static void reset(void)
{
    nge_map_init(40, 40, &window);
    nge_map_set_camera(0, 0);
    nge_vfx_init(FIRST);
    frame();
}

/* Run updates until nothing is busy: how many it took (at most 100) */
static int settle(void)
{
    int n;

    for (n = 0; n < 100 && nge_vfx_busy(); n++) {
        frame();
    }
    return n;
}

/* A directed bolt from the middle of the view towards (dx, dy): its first pattern and flags, without the palette */
static void direction(int dx, int dy, int *pat, int *flags)
{
    uint8_t s;

    reset();
    s = nge_vfx_bolt(160, 112, (uint16_t)(160 + dx), (uint16_t)(112 + dy), 8, &arrow);
    *pat = nge_vfx_first[s];
    *flags = nge_vfx_flags[s] & (NGE_SPRITE_MIRROR_X | NGE_SPRITE_MIRROR_Y | NGE_SPRITE_ROTATE);
    CHECK((nge_vfx_flags[s] & 0xF0) == NGE_SPRITE_PALETTE(3));   /* the look's palette kept */
}

int main(void)
{
    uint8_t s;
    uint8_t t;
    int n;
    int ok;
    int pat;
    int flags;

    test_init();

    /* ---- init: the first upload hides the module's sprites, from the first one named ---- */
    nge_map_init(40, 40, &window);
    nge_vfx_init(FIRST);
    CHECK(!nge_vfx_busy());
    CHECK(frame() == NGE_VFX_SLOTS && !any_visible());
    CHECK(frame() == 0);                                 /* nothing shown, nothing sent */

    /* ---- a burst plays its patterns once at its rate, at its point less the camera ---- */
    reset();
    nge_map_set_camera(2, 1);                            /* the view's top-left at map pixel (32, 16) */
    s = nge_vfx_burst(64, 48, &splash, 0);
    CHECK(s == 0 && nge_vfx_busy());
    ok = 1;
    for (n = 0; n < 6; n++) {
        ok = ok && frame() == NGE_VFX_SLOTS && visible(s) && pattern(s) == 10 + n / 2;
    }
    CHECK(ok);
    CHECK(sprite_x(s) == 32 && sprite_y(s) == 32 && sprite_flags(s) == NGE_SPRITE_PALETTE(4));
    CHECK(nge_vfx_busy());                               /* its last frame is showing */
    CHECK(frame() == NGE_VFX_SLOTS && !visible(s));      /* the update after: freed and hidden */
    CHECK(!nge_vfx_busy());
    CHECK(frame() == 0);

    /* ---- a delay: hidden, and busy, for that many updates ---- */
    reset();
    s = nge_vfx_burst(64, 48, &splash, 2);
    CHECK(nge_vfx_busy());
    frame();
    CHECK(!visible(s) && nge_vfx_busy());
    frame();
    CHECK(!visible(s));
    frame();
    CHECK(visible(s) && pattern(s) == 10);
    CHECK(settle() == 6);

    /* ---- a hold shows its first pattern during its delay, then ends ---- */
    reset();
    {
        nge_vfx_look_t hold = splash;

        hold.mode |= NGE_VFX_HOLD;
        s = nge_vfx_burst(64, 48, &hold, 3);
    }
    ok = 1;
    for (n = 0; n < 3; n++) {
        frame();
        ok = ok && visible(s) && pattern(s) == 10;
    }
    CHECK(ok);
    frame();
    CHECK(!visible(s) && !nge_vfx_busy());

    /* ---- a bolt moves speed pixels a frame and frees its slot on arriving ---- */
    reset();
    s = nge_vfx_bolt(0, 0, 64, 0, 8, &orb);
    CHECK(s != NGE_VFX_NONE);
    ok = 1;
    for (n = 1; n <= 7; n++) {
        frame();
        ok = ok && visible(s) && sprite_x(s) == 8 * n && sprite_y(s) == 0 && pattern(s) == 30;
    }
    CHECK(ok);
    frame();
    CHECK(!visible(s) && !nge_vfx_busy());               /* 64 pixels at 8: freed on the 8th update */

    /* ---- every direction and a knight's move reach the end exactly ---- */
    {
        static const int16_t ends[][2] = {
            { 48, 0 }, { -48, 0 }, { 0, 48 }, { 0, -48 }, { 48, 48 }, { -48, 48 }, { 48, -48 }, { -48, -48 },
            { 32, 16 }, { -16, 32 }, { 5, 3 },
        };

        ok = 1;
        for (n = 0; n < (int)(sizeof(ends) / sizeof(ends[0])); n++) {
            reset();
            s = nge_vfx_bolt(160, 112, (uint16_t)(160 + ends[n][0]), (uint16_t)(112 + ends[n][1]), 4, &orb);
            settle();
            ok = ok && nge_vfx_x[s] == (uint16_t)(160 + ends[n][0]) && nge_vfx_y[s] == (uint16_t)(112 + ends[n][1]);
        }
        CHECK(ok);
    }
    reset();
    s = nge_vfx_bolt(0, 0, 32, 16, 4, &orb);
    CHECK(settle() == 8);                                /* the long axis decides: 32 pixels at 4 */

    /* ---- the same point is no bolt ---- */
    reset();
    CHECK(nge_vfx_bolt(16, 16, 16, 16, 4, &orb) == NGE_VFX_NONE && !nge_vfx_busy());

    /* ---- an impact waits for the bolt flying to its point ---- */
    reset();
    s = nge_vfx_bolt(0, 0, 64, 0, 8, &orb);              /* freed on the 8th update */
    t = nge_vfx_burst(64, 0, &splash, 0);
    CHECK(nge_vfx_delay[t] == 7);
    for (n = 1; n <= 7; n++) {
        frame();
    }
    CHECK(visible(s) && !visible(t));                    /* the bolt's last frame, the splash waiting */
    frame();
    CHECK(!visible(s) && visible(t) && pattern(t) == 10);   /* it lands: the splash starts */
    /* A longer delay of its own wins */
    reset();
    nge_vfx_bolt(0, 0, 64, 0, 8, &orb);
    t = nge_vfx_burst(64, 0, &splash, 10);
    CHECK(nge_vfx_delay[t] == 10);
    /* A burst elsewhere doesn't wait */
    t = nge_vfx_burst(48, 0, &splash, 0);
    CHECK(nge_vfx_delay[t] == 0);
    /* Two bolts to the same point: the later arrival */
    reset();
    nge_vfx_bolt(48, 0, 64, 0, 8, &orb);                 /* freed on the 2nd update */
    nge_vfx_bolt(0, 0, 64, 0, 4, &orb);                  /* freed on the 16th */
    t = nge_vfx_burst(64, 0, &splash, 0);
    CHECK(nge_vfx_delay[t] == 15);
    CHECK(nge_vfx_arrival(64, 0) == 15 && nge_vfx_arrival(48, 0) == 0);
    /* A bolt part-way: the frames it still needs */
    reset();
    nge_vfx_bolt(0, 0, 64, 0, 8, &orb);
    frame();
    frame();
    frame();
    t = nge_vfx_burst(64, 0, &splash, 0);
    CHECK(nge_vfx_delay[t] == 4);
    /* A hold at the point is shown until the bolt arrives */
    reset();
    s = nge_vfx_bolt(0, 0, 32, 0, 8, &orb);              /* freed on the 4th update */
    {
        nge_vfx_look_t hold = orb;

        hold.first = 40;
        hold.mode |= NGE_VFX_HOLD;
        t = nge_vfx_burst(32, 0, &hold, 0);
    }
    frame();
    CHECK(visible(t) && pattern(t) == 40);
    frame();
    frame();
    CHECK(visible(t));
    frame();
    CHECK(!visible(t) && !visible(s) && !nge_vfx_busy());

    /* ---- a directed bolt points the nearest of the 8 ways ---- */
    direction(48, 0, &pat, &flags);
    CHECK(pat == 20 && flags == 0);
    direction(-48, 0, &pat, &flags);
    CHECK(pat == 20 && flags == NGE_SPRITE_MIRROR_X);
    direction(0, 48, &pat, &flags);
    CHECK(pat == 20 && flags == NGE_SPRITE_ROTATE);
    direction(0, -48, &pat, &flags);
    CHECK(pat == 20 && flags == (NGE_SPRITE_ROTATE | NGE_SPRITE_MIRROR_Y));
    direction(48, -48, &pat, &flags);
    CHECK(pat == 21 && flags == 0);
    direction(48, 48, &pat, &flags);
    CHECK(pat == 21 && flags == NGE_SPRITE_MIRROR_Y);
    direction(-48, -48, &pat, &flags);
    CHECK(pat == 21 && flags == NGE_SPRITE_MIRROR_X);
    direction(-48, 48, &pat, &flags);
    CHECK(pat == 21 && flags == (NGE_SPRITE_MIRROR_X | NGE_SPRITE_MIRROR_Y));
    direction(48, 16, &pat, &flags);                     /* 3 to 1: nearer straight */
    CHECK(pat == 20 && flags == 0);
    direction(32, 16, &pat, &flags);                     /* a knight's move: nearer diagonal */
    CHECK(pat == 21 && flags == NGE_SPRITE_MIRROR_Y);
    direction(-16, -48, &pat, &flags);
    CHECK(pat == 20 && flags == (NGE_SPRITE_ROTATE | NGE_SPRITE_MIRROR_Y));
    /* An undirected look keeps its flags */
    reset();
    s = nge_vfx_bolt(160, 112, 112, 160, 8, &orb);
    CHECK(nge_vfx_first[s] == 30 && nge_vfx_flags[s] == NGE_SPRITE_PALETTE(10));

    /* ---- out of view ---- */
    reset();                                             /* the view: map pixels (0, 0) to (319, 239) */
    CHECK(nge_vfx_burst(320, 0, &splash, 0) == NGE_VFX_NONE);
    CHECK(nge_vfx_burst(0, 240, &splash, 0) == NGE_VFX_NONE);
    CHECK(nge_vfx_bolt(400, 0, 480, 0, 8, &orb) == NGE_VFX_NONE);
    CHECK(!nge_vfx_busy());
    CHECK(nge_vfx_burst(304, 224, &splash, 0) != NGE_VFX_NONE);   /* the last tile in view */
    reset();
    nge_map_set_camera(2, 0);                            /* the view's left edge at map pixel 32 */
    CHECK(nge_vfx_burst(17, 0, &splash, 0) != NGE_VFX_NONE);      /* partly in view */
    CHECK(nge_vfx_burst(16, 0, &splash, 0) == NGE_VFX_NONE);      /* just out */
    reset();
    s = nge_vfx_bolt(400, 0, 288, 0, 8, &orb);           /* from outside into the view */
    CHECK(s != NGE_VFX_NONE);
    frame();
    CHECK(!visible(s) && nge_vfx_busy());                /* outside the view: hidden, still flying */
    for (n = 0; n < 10; n++) {
        frame();
    }
    CHECK(visible(s) && sprite_x(s) == 400 - 8 * 11);
    settle();

    /* ---- a full pool starts nothing ---- */
    reset();
    ok = 1;
    for (n = 0; n < NGE_VFX_SLOTS; n++) {
        ok = ok && nge_vfx_burst((uint16_t)(16 * n), 0, &splash, 0) == n;
    }
    CHECK(ok);
    CHECK(nge_vfx_burst(0, 16, &splash, 0) == NGE_VFX_NONE);
    CHECK(nge_vfx_bolt(0, 16, 64, 16, 8, &orb) == NGE_VFX_NONE);
    CHECK(frame() == NGE_VFX_SLOTS);
    ok = 1;
    for (n = 0; n < NGE_VFX_SLOTS; n++) {
        ok = ok && visible(n);
    }
    CHECK(ok);

    /* ---- clear ends them all; the next upload hides them ---- */
    nge_vfx_clear();
    CHECK(!nge_vfx_busy());
    test_port_reset();
    nge_vfx_upload();
    CHECK(test_port_log_count == 1 + NGE_VFX_SLOTS * ATTR_BYTES && !any_visible());
    CHECK(nge_vfx_burst(0, 16, &splash, 0) == 0);        /* every slot free again */

    /* ---- init again (a new game): the same state as the first ---- */
    nge_vfx_init(FIRST);
    CHECK(!nge_vfx_busy() && nge_vfx_active == 0 && nge_vfx_drawn == 0 && nge_vfx_shown == NGE_VFX_SLOTS);
    ok = 1;
    for (n = 0; n < NGE_VFX_SLOTS; n++) {
        ok = ok && nge_vfx_kind[n] == NGE_VFX_FREE;
    }
    CHECK(ok);

    return test_summary("vfx");
}
