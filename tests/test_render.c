/* nge_render.c: the render system (sprite component, culling, layers, animation, uploads). */
#include "test.h"

#include <arch/zxn.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "map/nge_map.h"
#include "sprite/nge_sprite.h"

#define ATTR_BYTES  5
#define AREA0       NGE_OWNER_AREA(0)
#define AREA1       NGE_OWNER_AREA(1)

/* Window: the whole width, rows 22-29: a view of 20 x 4 map tiles of 16 pixels; its top-left pixel is (0, 176) */
static const nge_rect_t window = { .x1 = 0, .x2 = 39, .y1 = 22, .y2 = 29 };
/* A narrower window, with room on the right for the column a fine offset shows: for following */
static const nge_rect_t narrow = { .x1 = 0, .x2 = 31, .y1 = 22, .y2 = 29 };

/* Upload, returning the number of records sent (their bytes start at test_port_log_val[1]) */
static int upload(void)
{
    test_port_reset();
    nge_render_upload();
    if (test_port_log_count == 0) {
        return 0;
    }
    CHECK(test_port_log_port[0] == __IO_SPRITE_SLOT && test_port_log_val[0] == 0);
    CHECK((test_port_log_count - 1) % ATTR_BYTES == 0);
    return (test_port_log_count - 1) / ATTR_BYTES;
}

static int frame(void)
{
    nge_render_update();
    return upload();
}

static uint8_t attr(int sprite, int n)
{
    return test_port_log_val[1 + sprite * ATTR_BYTES + n];
}

static int visible(int sprite)
{
    return (attr(sprite, 3) & 0x80) != 0;
}

/* An alive entity in an area at tile (x, y), drawn with pattern p in layer */
static nge_entity_id_t make(nge_owner_t area, uint8_t x, uint8_t y, uint8_t p, uint8_t layer)
{
    nge_entity_id_t id = nge_entity_create();

    nge_owner_set(id, area);
    nge_location_set(id, x, y);
    nge_render_add(id, p, 0, 0, layer);
    return id;
}

static uint8_t pattern(int sprite)
{
    return attr(sprite, 3) & 0x3F;
}

/* Sprite 0's x and y, 9 bits each */
static int sprite_x(void)
{
    return attr(0, 0) | ((attr(0, 2) & 0x01) << 8);
}

static int sprite_y(void)
{
    return attr(0, 1) | ((attr(0, 4) & 0x01) << 8);
}

static void reset(void)
{
    nge_entity_init();
    nge_render_init();
    nge_map_init(80, 48, &window);
    nge_map_set_camera(0, 20);                          /* view: columns 0-19, rows 20-23 */
    nge_render_set_view(AREA0);
    nge_render_upload();                                /* init's upload: every sprite hidden */
}

int main(void)
{
    nge_entity_id_t a;
    nge_entity_id_t b;
    nge_entity_id_t c;
    nge_entity_id_t id;
    int n;
    int ok;

    test_init();

    /* ---- nothing to draw: the first upload hides all the system's sprites ---- */
    nge_entity_init();
    nge_render_init();
    CHECK(frame() == NGE_RENDER_SPRITES);
    ok = 1;
    for (n = 0; n < NGE_RENDER_SPRITES; n++) {
        ok = ok && !visible(n);
    }
    CHECK(ok);
    CHECK(frame() == 0 && test_port_log_count == 0);         /* no view, nothing sent */

    /* ---- init again (a new game): what was drawn is hidden at the next upload ---- */
    reset();
    make(AREA0, 1, 21, 1, 0);
    make(AREA0, 2, 21, 2, 0);
    CHECK(frame() == 2 && visible(0) && visible(1));
    nge_entity_init();
    nge_render_init();
    CHECK(frame() == NGE_RENDER_SPRITES && !visible(0) && !visible(1));
    CHECK(frame() == 0);

    /* ---- position: map pixels through the camera into the window ---- */
    reset();
    a = make(AREA0, 5, 22, 7, 0);
    nge_location_set_fine(a, 3, 0);
    CHECK(frame() == 1);
    CHECK(attr(0, 0) == 83 && attr(0, 1) == (uint8_t)(176 + 32));    /* (5*16+3, 176 + (22-20)*16) */
    CHECK(attr(0, 2) == 0 && attr(0, 3) == (0xC0 | 7) && attr(0, 4) == 0);

    /* Draw offset, flags, scale, 4-bit pattern */
    nge_render_set_offset(a, -3, -8);
    nge_render_set_flags(a, NGE_SPRITE_MIRROR_X | NGE_SPRITE_PALETTE(1));
    nge_render_set_scale(a, NGE_SPRITE_SCALE_Y2);
    frame();
    CHECK(attr(0, 0) == 80 && attr(0, 1) == 200);
    CHECK(attr(0, 2) == (0x10 | 0x08) && attr(0, 4) == 0x02);
    b = nge_entity_create();
    nge_owner_set(b, AREA0);
    nge_location_set(b, 0, 20);
    nge_render_add(b, 13, 0, NGE_SPRITE_PATTERN4, 0);
    CHECK(frame() == 2);
    ok = 0;
    for (n = 0; n < 2; n++) {
        ok |= attr(n, 3) == (0xC0 | 6) && attr(n, 4) == (0x80 | 0x40);
    }
    CHECK(ok);

    /* ---- the pixel camera: sprites move with the map's fine offset (room below the window) ---- */
    reset();
    a = make(AREA0, 5, 22, 7, 0);
    nge_map_set_camera_px(0, 20 * 16 + 3);
    CHECK(frame() == 1 && attr(0, 0) == 80 && attr(0, 1) == (uint8_t)(176 + 32 - 3));
    /* Partly off the top of the window: still drawn, 3 pixels above it */
    reset();
    a = make(AREA0, 5, 20, 7, 0);
    nge_map_set_camera_px(0, 20 * 16 + 3);
    CHECK(frame() == 1 && attr(0, 1) == (uint8_t)(176 - 3));
    /* Partly, up to the last row: 15 pixels above it */
    nge_map_set_camera_px(0, 20 * 16 + 15);
    CHECK(frame() == 1 && visible(0) && attr(0, 1) == (uint8_t)(176 - 15));
    /* Wholly above it: not drawn (its record is sent hidden) */
    nge_map_set_camera_px(0, 21 * 16 + 1);
    CHECK(frame() == 1 && !visible(0));
    nge_map_set_camera(0, 20);

    /* ---- what is drawn: alive, both components, owned by the view's area ---- */
    reset();
    a = make(AREA0, 1, 21, 1, 0);
    make(AREA1, 2, 21, 2, 0);                                 /* another area */
    b = make(AREA0, 3, 21, 3, 0);
    nge_location_remove(b);                                   /* no location */
    c = make(AREA0, 4, 21, 4, 0);
    nge_render_remove(c);                                     /* no sprite */
    id = make(a, 5, 21, 5, 0);                                /* owned by an entity in the area */
    id = make(AREA0, 6, 21, 6, 0);
    nge_entity_destroy(id);                                   /* dying */
    CHECK(frame() == 1 && attr(0, 3) == (0xC0 | 1));
    nge_render_set_view(AREA1);
    CHECK(frame() == 1 && attr(0, 3) == (0xC0 | 2));
    nge_render_set_view(NGE_RENDER_NO_VIEW);
    CHECK(frame() == 1 && !visible(0));                       /* no view: hidden */
    CHECK(frame() == 0);

    /* ---- culling, allowing for the sprite's size ---- */
    reset();
    nge_map_set_camera(10, 20);                         /* view: pixels 160-479, 320-383 */
    a = make(AREA0, 9, 21, 1, 0);
    nge_location_set_fine(a, 8, 0);                           /* x 152: 8 pixels left of the view */
    b = make(AREA0, 9, 21, 2, 0);                             /* x 144: 16 left, entirely outside */
    c = make(AREA0, 30, 21, 3, 0);                            /* x 480: right of the view */
    id = make(AREA0, 15, 24, 4, 0);                           /* y 384: below the view */
    CHECK(frame() == 1 && attr(0, 3) == (0xC0 | 1));
    CHECK(attr(0, 0) == 0xF8 && (attr(0, 2) & 0x01));         /* x = -8: 0x1F8, partly in view */
    nge_render_set_scale(b, NGE_SPRITE_SCALE_X2);             /* 32 wide: now partly in view */
    CHECK(frame() == 2);
    nge_location_set(c, 29, 21);                              /* x 464: last tile */
    nge_location_set(id, 15, 23);                             /* y 368: last row */
    CHECK(frame() == 4);
    nge_render_set_offset(id, 0, 16);                         /* y 384 */
    CHECK(frame() == 4 && !visible(3));                       /* hidden: one fewer drawn */
    CHECK(frame() == 3);

    /* ---- layers: layer 0 in the lowest sprites, the top layer highest ---- */
    reset();
    a = make(AREA0, 1, 21, 1, 3);
    b = make(AREA0, 2, 21, 2, 0);
    c = make(AREA0, 3, 21, 3, 1);
    CHECK(frame() == 3);
    CHECK((attr(0, 3) & 0x3F) == 2 && (attr(1, 3) & 0x3F) == 3 && (attr(2, 3) & 0x3F) == 1);
    nge_render_set_layer(a, 0);
    nge_render_set_layer(b, 2);
    frame();
    CHECK((attr(2, 3) & 0x3F) == 2);

    /* ---- animation: count patterns from first, rate frames each ---- */
    reset();
    a = make(AREA0, 1, 21, 0, 0);
    nge_render_set_anim(a, 2, 3, 2);
    {
        static const uint8_t expect[] = { 2, 3, 3, 4, 4, 2, 2, 3 };

        ok = 1;
        for (n = 0; n < (int)sizeof(expect); n++) {
            frame();
            ok = ok && (attr(0, 3) & 0x3F) == expect[n];
        }
        CHECK(ok);
    }
    nge_render_set_pattern(a, 9);                             /* still again */
    frame();
    frame();
    CHECK((attr(0, 3) & 0x3F) == 9);

    /* ---- one-shots: once, rate frames a pattern, then the rest animation by itself ---- */
    reset();
    a = make(AREA0, 1, 21, 9, 0);                             /* rest: still 9 */
    CHECK(!nge_render_busy());
    nge_render_play(a, 4, 2, 2);
    CHECK(nge_render_busy());                                 /* from the start, before an update */
    {
        static const uint8_t expect[] = { 4, 4, 5, 5, 9, 9 };
        static const uint8_t busy[] = { 1, 1, 1, 1, 0, 0 };

        ok = 1;
        for (n = 0; n < (int)sizeof(expect); n++) {
            frame();
            ok = ok && pattern(0) == expect[n] && nge_render_busy() == busy[n];
        }
        CHECK(ok);
    }
    /* Rate 1, one pattern: shown for one frame */
    nge_render_play(a, 4, 1, 1);
    frame();
    CHECK(pattern(0) == 4 && nge_render_busy());
    frame();
    CHECK(pattern(0) == 9 && !nge_render_busy());
    /* A new rest animation during a one-shot: the one-shot carries on, then the new rest from its first */
    nge_render_play(a, 4, 2, 1);
    frame();
    nge_render_set_anim(a, 20, 2, 1);
    CHECK(pattern(0) == 4);
    frame();
    CHECK(pattern(0) == 5);
    frame();
    CHECK(pattern(0) == 20 && !nge_render_busy());
    frame();
    CHECK(pattern(0) == 21);
    /* A new one-shot replaces one playing */
    nge_render_play(a, 4, 2, 2);
    frame();
    nge_render_play(a, 30, 1, 1);
    frame();
    CHECK(pattern(0) == 30);
    /* Out of view: not played, and one playing ends */
    b = make(AREA0, 45, 21, 9, 0);                            /* column 45: right of the view (0-19) */
    nge_render_play(b, 4, 2, 2);
    frame();                                                  /* a's one-shot ends */
    CHECK(!nge_render_busy());
    nge_render_play(b, 4, 2, 2);
    CHECK(!nge_render_busy());

    /* ---- slides: from the old tile to the location, speed pixels a frame ---- */
    reset();
    a = make(AREA0, 5, 22, 1, 0);                             /* drawn at (80, 208) */
    nge_render_slide(a, -16, 0, 4);                           /* came from the left: a tile in 4 frames */
    CHECK(nge_render_busy());
    {
        static const int expect[] = { 68, 72, 76, 80, 80 };
        static const uint8_t busy[] = { 1, 1, 1, 0, 0 };

        ok = 1;
        for (n = 0; n < (int)sizeof(busy); n++) {
            frame();
            ok = ok && sprite_x() == expect[n] && sprite_y() == 208 && nge_render_busy() == busy[n];
        }
        CHECK(ok);
    }
    /* Diagonal: both axes finish together; the draw offset is kept */
    nge_render_set_offset(a, 0, -8);
    nge_render_slide(a, 16, 16, 4);                           /* came from down-right */
    frame();
    CHECK(sprite_x() == 92 && sprite_y() == 200 + 12);
    frame();
    frame();
    CHECK(nge_render_busy());
    frame();
    CHECK(sprite_x() == 80 && sprite_y() == 200 && !nge_render_busy());
    /* A slide during a slide: from where it is drawn */
    nge_render_set_offset(a, 0, 0);
    nge_render_slide(a, -16, 0, 4);
    frame();                                                  /* at -12 */
    nge_render_slide(a, -16, 0, 4);                           /* moved again: -28 from the new location */
    frame();
    CHECK(sprite_x() == 80 - 24);

    /* ---- lunges: out to the offset and back ---- */
    reset();
    a = make(AREA0, 5, 22, 1, 0);
    nge_render_lunge(a, 8, -8, 4);
    {
        static const int expect_x[] = { 84, 88, 84, 80 };
        static const int expect_y[] = { 204, 200, 204, 208 };
        static const uint8_t busy[] = { 1, 1, 1, 0 };

        ok = 1;
        for (n = 0; n < (int)sizeof(busy); n++) {
            frame();
            ok = ok && sprite_x() == expect_x[n] && sprite_y() == expect_y[n] && nge_render_busy() == busy[n];
        }
        CHECK(ok);
    }

    /* ---- busy: in view only, and never held by an entity that has gone ---- */
    reset();
    a = make(AREA0, 5, 22, 1, 0);
    nge_render_slide(a, -16, 0, 1);
    frame();
    CHECK(nge_render_busy());
    nge_entity_destroy(a);                                    /* released mid-slide */
    frame();
    CHECK(!nge_render_busy());
    /* Neither end in view: no slide */
    b = make(AREA0, 50, 22, 1, 0);
    nge_render_slide(b, -16, 0, 4);
    CHECK(!nge_render_busy());
    frame();
    CHECK(!nge_render_busy());
    /* Leaving the view: drawn while partly in it, then ends when its place is out of view */
    c = make(AREA0, 21, 22, 1, 0);                            /* x 336: past the view's 320 */
    nge_render_slide(c, -16, 0, 1);                           /* from x 320: just out too */
    CHECK(!nge_render_busy());
    nge_location_set(c, 20, 22);                              /* x 320 */
    nge_render_slide(c, -16, 0, 1);                           /* from x 304: in view */
    CHECK(nge_render_busy());
    CHECK(frame() == 1 && visible(0) && sprite_x() == 305);
    nge_map_set_camera(10, 20);                               /* the camera moves away to the right: still in view */
    frame();
    CHECK(nge_render_busy());
    nge_map_set_camera(0, 0);                                 /* the view leaves it: it ends */
    frame();
    CHECK(!nge_render_busy());
    nge_map_set_camera(0, 20);
    nge_location_set(c, 19, 22);
    CHECK(frame() == 1 && sprite_x() == 304);                 /* no motion left */
    /* Another area's entity is never in view */
    id = make(AREA1, 5, 22, 1, 0);
    nge_render_slide(id, -16, 0, 4);
    nge_render_lunge(id, 8, 0, 4);
    nge_render_play(id, 4, 2, 2);
    CHECK(!nge_render_busy());

    /* ---- following: the camera centres on the drawn position when it changes ---- */
    nge_entity_init();
    nge_render_init();
    nge_map_init(80, 48, &narrow);                            /* view 16 x 4: 8 x 2 tiles to the centre */
    nge_render_set_view(AREA0);
    a = make(AREA0, 30, 30, 1, 0);
    frame();
    CHECK(nge_map.cam_x == 0 && nge_map.cam_y == 0);           /* nothing followed */
    nge_render_follow(a);
    frame();
    CHECK(NGE_MAP_CAMERA_PX() == (30 - 8) * 16 && NGE_MAP_CAMERA_PY() == (30 - 2) * 16);
    CHECK(sprite_x() == 8 * 16 && sprite_y() == 22 * 8 + 2 * 16);
    nge_location_set(a, 31, 30);
    nge_render_slide(a, -16, 0, 4);
    frame();
    CHECK(NGE_MAP_CAMERA_PX() == (30 - 8) * 16 + 4);          /* a pixel camera, in the same update */
    CHECK(sprite_x() == 8 * 16);                              /* the player stays centred */
    frame();
    frame();
    frame();
    CHECK(NGE_MAP_CAMERA_PX() == (31 - 8) * 16 && nge_map.fine_x == 0 && sprite_x() == 8 * 16);
    /* Not moving: the camera stays where the game puts it (targeting) */
    nge_map_set_camera(2, 3);
    frame();
    frame();
    CHECK(nge_map.cam_x == 2 && nge_map.cam_y == 3);
    /* A jump (a new level) centres it */
    nge_location_set(a, 40, 20);
    frame();
    CHECK(nge_map.cam_x == 40 - 8 && nge_map.cam_y == 20 - 2);
    /* Following nothing */
    nge_render_follow(NGE_ENTITY_INVALID);
    nge_location_set(a, 50, 20);
    frame();
    CHECK(nge_map.cam_x == 40 - 8);
    nge_map_init(80, 48, &window);

    /* ---- fewer sprites: the rest are sent hidden, then no longer sent ---- */
    reset();
    a = make(AREA0, 1, 21, 1, 0);
    b = make(AREA0, 2, 21, 2, 0);
    c = make(AREA0, 3, 21, 3, 0);
    CHECK(frame() == 3);
    nge_owner_set(b, AREA1);
    nge_owner_set(c, AREA1);
    CHECK(frame() == 3 && visible(0) && !visible(1) && !visible(2));
    CHECK(frame() == 1);

    /* Several updates between uploads: the hardware's sprites are still hidden */
    nge_owner_set(b, AREA0);
    nge_owner_set(c, AREA0);
    CHECK(frame() == 3);
    nge_owner_set(b, AREA1);
    nge_owner_set(c, AREA1);
    nge_render_update();
    nge_render_update();
    CHECK(upload() == 3 && !visible(1) && !visible(2));
    CHECK(upload() == 1);

    /* ---- more visible entities than sprites: the lowest layers are left out ---- */
    reset();
    for (n = 0; n < NGE_RENDER_SPRITES + 5; n++) {
        make(AREA0, (uint8_t)(n % 20), (uint8_t)(20 + n / 20), 1, 1);   /* all in the 20 x 4 view */
    }
    a = make(AREA0, 1, 21, 9, 3);                             /* top layer: always drawn */
    b = make(AREA0, 1, 21, 8, 0);                             /* bottom layer: left out */
    CHECK(frame() == NGE_RENDER_SPRITES);
    CHECK(nge_render_dropped() == 7);
    CHECK((attr(NGE_RENDER_SPRITES - 1, 3) & 0x3F) == 9);
    ok = 1;
    for (n = 0; n < NGE_RENDER_SPRITES; n++) {
        ok = ok && (attr(n, 3) & 0x3F) != 8;
    }
    CHECK(ok);

    /* ---- asserts ---- */
    reset();
    a = make(AREA0, 1, 21, 1, 0);
    b = nge_entity_create();
    TEST_EXPECT_HALT(nge_render_add(50, 0, 0, 0, 0));        /* free */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_add(b, 0, 0, 0, NGE_RENDER_LAYERS));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_add(b, NGE_SPRITE_PATTERNS, 0, 0, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_set_pattern(b, 0));         /* no sprite component */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_set_anim(a, 62, 3, 1));     /* past the last pattern */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_set_anim(a, 0, 0, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_set_anim(a, 0, 2, 0));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_set_layer(a, NGE_RENDER_LAYERS));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_set_view(NGE_OWNER_COUNT));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_play(a, 0, 0, 1));
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_slide(a, -8, 0, 0));        /* speed 0 */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_lunge(a, 0, 0, 1));         /* nowhere to lunge */
    CHECK(test_halt_count == 1);
    TEST_EXPECT_HALT(nge_render_slide(b, -8, 0, 1));        /* no sprite component */
    CHECK(test_halt_count == 1);
    nge_render_add(b, 100, 0, NGE_SPRITE_PATTERN4, 0);       /* 4-bit patterns go to 127 */
    nge_render_set_anim(b, 120, 8, 1);
    CHECK(NGE_ENTITY_HAS_COMPONENT(b, NGE_COMPONENT_SPRITE));

    return test_summary("render");
}
