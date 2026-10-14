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

static nge_tile_t tiles[NGE_MAP_STRIDE * NGE_MAP_MAX_HEIGHT];
static nge_map_t map;
/* Window: the whole width, rows 22-30 (as the demo); its top-left pixel is (0, 176) */
static const nge_rect_t window = { .x1 = 0, .x2 = 39, .y1 = 22, .y2 = 30 };

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

static void reset(void)
{
    nge_entity_init();
    nge_render_init();
    nge_map_init(&map, tiles, 80, 48, &window);
    nge_map_set_camera(&map, 0, 20);                          /* view: columns 0-39, rows 20-28 */
    nge_render_set_view(AREA0, &map);
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

    /* ---- nothing to draw ---- */
    nge_entity_init();
    nge_render_init();
    CHECK(frame() == 0 && test_port_log_count == 0);         /* no view, nothing sent */

    /* ---- position: map pixels through the camera into the window ---- */
    reset();
    a = make(AREA0, 5, 25, 7, 0);
    nge_location_set_fine(a, 3, 0);
    CHECK(frame() == 1);
    CHECK(attr(0, 0) == 43 && attr(0, 1) == (uint8_t)(176 + 40));    /* (5*8+3, 176 + (25-20)*8) */
    CHECK(attr(0, 2) == 0 && attr(0, 3) == (0xC0 | 7) && attr(0, 4) == 0);

    /* Draw offset, flags, scale, 4-bit pattern */
    nge_render_set_offset(a, -3, -8);
    nge_render_set_flags(a, NGE_SPRITE_MIRROR_X | NGE_SPRITE_PALETTE(1));
    nge_render_set_scale(a, NGE_SPRITE_SCALE_Y2);
    frame();
    CHECK(attr(0, 0) == 40 && attr(0, 1) == 208);
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
    nge_render_set_view(AREA1, &map);
    CHECK(frame() == 1 && attr(0, 3) == (0xC0 | 2));
    nge_render_set_view(AREA1, NULL);
    CHECK(frame() == 1 && !visible(0));                       /* no map: hidden */
    CHECK(frame() == 0);

    /* ---- culling, allowing for the sprite's size ---- */
    reset();
    nge_map_set_camera(&map, 10, 20);                         /* view: pixels 80-399, 160-231 */
    a = make(AREA0, 9, 21, 1, 0);                             /* x 72: 8 pixels left of the view */
    b = make(AREA0, 8, 21, 2, 0);                             /* x 64: 16 left, entirely outside */
    c = make(AREA0, 50, 21, 3, 0);                            /* x 400: right of the view */
    id = make(AREA0, 30, 29, 4, 0);                           /* y 232: below the view */
    CHECK(frame() == 1 && attr(0, 3) == (0xC0 | 1));
    CHECK(attr(0, 0) == 0xF8 && (attr(0, 2) & 0x01));         /* x = -8: 0x1F8, partly in view */
    nge_render_set_scale(b, NGE_SPRITE_SCALE_X2);             /* 32 wide: now partly in view */
    CHECK(frame() == 2);
    nge_location_set(c, 49, 21);                              /* x 392: last 8 pixels */
    nge_location_set(id, 30, 28);                             /* y 224: last row */
    CHECK(frame() == 4);
    nge_render_set_offset(id, 0, 8);                          /* y 232 */
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
        make(AREA0, (uint8_t)(n % 40), (uint8_t)(20 + n / 40), 1, 1);
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
    TEST_EXPECT_HALT(nge_render_set_view(NGE_OWNER_COUNT, &map));
    CHECK(test_halt_count == 1);
    nge_render_add(b, 100, 0, NGE_SPRITE_PATTERN4, 0);       /* 4-bit patterns go to 127 */
    nge_render_set_anim(b, 120, 8, 1);
    CHECK(NGE_ENTITY_HAS_COMPONENTS(b, NGE_COMPONENT_SPRITE));

    return test_summary("render");
}
