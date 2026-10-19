/* game/target: targeting's cursor, keys, range and scrolling, and the look messages (game/messages). */
#include "test.h"

#include <string.h>

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "input/nge_input_internal.h"
#include "map/nge_map.h"

#include "game/game.h"
#include "game/gen/gen.h"
#include "game/level/level.h"
#include "game/messages/messages.h"
#include "game/screen.h"
#include "game/target/target.h"

#define EV(k)  ((nge_key_event_t)(k))

/* What game.c and actors.c provide in the game (not linked here) */
nge_entity_id_t actor_player;
static int play_helps;   /* game_play_help() calls */
static int waits;        /* game_wait_key() calls */

void game_over(uint8_t reason)
{
    (void)reason;
}

void game_play_help(void)
{
    play_helps++;
}

void game_play_redraw(void)
{
}

/*
 * The keys the player presses while a chooser waits: a key event already
 * waiting first, as the real one does, then the script's, one a wait. When
 * the script runs out, Q cancels, so a test that goes wrong ends
 * (out_of_keys says so)
 */
static const nge_key_event_t *script;
static int out_of_keys;

static void queue_key(nge_key_event_t ev)
{
    nge_input_queue[(nge_input_head + nge_input_count) % NGE_KEY_QUEUE] = ev;
    nge_input_count++;
}

nge_key_event_t game_wait_key(uint16_t frames)
{
    nge_key_event_t ev = nge_key_get();

    (void)frames;
    waits++;
    if (ev != NGE_KEY_NONE) {
        return ev;
    }
    if (script != NULL && *script != NGE_KEY_NONE) {
        return *script++;
    }
    out_of_keys = 1;
    return EV(NGE_KEY_Q);
}

static char text[NGE_TILEMAP_WIDTH + 1];

/* The message n lines up from the newest (0): the window (rows 27-30) fills from the top, then scrolls */
static const char *message(int n)
{
    int y;

    for (y = 30; y >= 27; y--) {
        test_tilemap_row((uint8_t)y, 0, 39, text);
        if (text[0] != ' ') {
            break;
        }
    }
    test_tilemap_row((uint8_t)(y - n), 0, 39, text);
    return text;
}

static int said(int n, const char *s)
{
    return strncmp(message(n), s, strlen(s)) == 0;
}

static void context(target_t *t, uint8_t x, uint8_t y, uint8_t range)
{
    t->purpose = TARGET_LOOK;
    t->origin_x = x;
    t->origin_y = y;
    t->range = range;
    t->x = x;
    t->y = y;
}

int main(void)
{
    static const nge_key_event_t right_right_enter[] = {
        EV(NGE_KEY_RIGHT), EV(NGE_KEY_L), EV(NGE_KEY_ENTER), NGE_KEY_NONE,
    };
    static const nge_key_event_t cancel[] = { EV(NGE_KEY_Q), NGE_KEY_NONE };
    static const nge_key_event_t out_then_back[] = {
        EV(NGE_KEY_DOWN), EV(NGE_KEY_DOWN), EV(NGE_KEY_SPACE), EV(NGE_KEY_UP), EV(NGE_KEY_SPACE), NGE_KEY_NONE,
    };
    target_t t;
    nge_entity_id_t ring;
    nge_entity_id_t sword;
    uint8_t x;
    uint8_t y;

    test_init();

    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    nge_input_init();
    screen_init();
    nge_map_init(LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);   /* 60 x 40, the view 28 x 27 */
    actor_player = gen_creature(CR_PLAYER, LEVEL_AREA);
    nge_location_set(actor_player, 30, 20);
    nge_map_centre_camera(30, 20);
    nge_render_set_view(LEVEL_AREA);
    nge_event_flush();

    /* ---- range: from the origin, not the start ---- */
    context(&t, 30, 20, 3);
    CHECK(target_in_range(&t));
    t.x = 33;
    CHECK(target_in_range(&t));
    t.x = 34;
    CHECK(!target_in_range(&t));
    t.x = 32;
    t.y = 22;   /* 4 + 4 <= 12 */
    CHECK(target_in_range(&t));
    context(&t, 0, 0, TARGET_RANGE_ANY);
    t.x = LEVEL_WIDTH - 1;
    t.y = LEVEL_HEIGHT - 1;
    CHECK(target_in_range(&t));

    /* ---- moving: every movement key, never off the map ---- */
    nge_map_set_camera(16, 7);   /* the cursor well inside the view: no scrolling */
    context(&t, 30, 20, TARGET_RANGE_ANY);
    CHECK(target_key(&t, EV(NGE_KEY_RIGHT)) == TARGET_STEP_MOVING && t.x == 31 && t.y == 20);
    CHECK(target_key(&t, EV(NGE_KEY_H)) == TARGET_STEP_MOVING && t.x == 30);
    CHECK(target_key(&t, EV(NGE_KEY_DOWN)) == TARGET_STEP_MOVING && t.y == 21);
    CHECK(target_key(&t, EV(NGE_KEY_K)) == TARGET_STEP_MOVING && t.y == 20);
    CHECK(target_key(&t, EV(NGE_KEY_L)) == TARGET_STEP_MOVING && t.x == 31);
    CHECK(target_key(&t, EV(NGE_KEY_J)) == TARGET_STEP_MOVING && t.y == 21);
    CHECK(nge_map.cam_x == 16 && nge_map.cam_y == 7);
    nge_map_set_camera(0, 0);
    context(&t, 0, 0, TARGET_RANGE_ANY);   /* the top-left corner */
    CHECK(target_key(&t, EV(NGE_KEY_LEFT)) == TARGET_STEP_MOVING && t.x == 0 && t.y == 0);
    CHECK(target_key(&t, EV(NGE_KEY_UP)) == TARGET_STEP_MOVING && t.x == 0 && t.y == 0);
    nge_map_set_camera(LEVEL_WIDTH, LEVEL_HEIGHT);   /* clamped: the bottom-right */
    context(&t, LEVEL_WIDTH - 1, LEVEL_HEIGHT - 1, TARGET_RANGE_ANY);
    CHECK(target_key(&t, EV(NGE_KEY_RIGHT)) == TARGET_STEP_MOVING && t.x == LEVEL_WIDTH - 1);
    CHECK(target_key(&t, EV(NGE_KEY_DOWN)) == TARGET_STEP_MOVING && t.y == LEVEL_HEIGHT - 1);

    /* ---- scrolling: one tile when the cursor comes into the margin ---- */
    nge_map_set_camera(20, 10);   /* view columns 20-47, rows 10-36 */
    context(&t, 20 + TARGET_SCROLL_MARGIN, 20, TARGET_RANGE_ANY);
    target_key(&t, EV(NGE_KEY_LEFT));   /* 23: into the left margin */
    CHECK(t.x == 23 && nge_map.cam_x == 19 && t.x - nge_map.cam_x == TARGET_SCROLL_MARGIN);
    context(&t, 47 - TARGET_SCROLL_MARGIN - 1, 20, TARGET_RANGE_ANY);   /* 42: just outside the right margin */
    nge_map_set_camera(20, 10);
    target_key(&t, EV(NGE_KEY_RIGHT));
    CHECK(t.x == 43 && nge_map.cam_x == 20);   /* 43 + 4 < 48: no scroll */
    target_key(&t, EV(NGE_KEY_RIGHT));
    CHECK(t.x == 44 && nge_map.cam_x == 21);
    context(&t, 30, 10 + TARGET_SCROLL_MARGIN, TARGET_RANGE_ANY);
    target_key(&t, EV(NGE_KEY_UP));
    CHECK(t.y == 13 && nge_map.cam_y == 9);
    context(&t, 30, 9 + 27 - TARGET_SCROLL_MARGIN, TARGET_RANGE_ANY);   /* 32: the bottom margin's edge */
    target_key(&t, EV(NGE_KEY_DOWN));
    CHECK(t.y == 33 && nge_map.cam_y == 10);
    /* At the map's edge the view can't scroll: the cursor walks on to the view's edge */
    nge_map_set_camera(0, 0);
    context(&t, 2, 2, TARGET_RANGE_ANY);
    target_key(&t, EV(NGE_KEY_LEFT));
    target_key(&t, EV(NGE_KEY_UP));
    target_key(&t, EV(NGE_KEY_LEFT));
    target_key(&t, EV(NGE_KEY_UP));
    CHECK(t.x == 0 && t.y == 0 && nge_map.cam_x == 0 && nge_map.cam_y == 0);

    /* ---- choosing and cancelling ---- */
    nge_map_centre_camera(30, 20);
    context(&t, 30, 20, 3);
    t.x = 33;
    CHECK(target_key(&t, EV(NGE_KEY_ENTER)) == TARGET_STEP_CHOSEN && t.x == 33 && t.y == 20);
    CHECK(target_key(&t, EV(NGE_KEY_SPACE)) == TARGET_STEP_CHOSEN);
    t.x = 34;
    CHECK(target_key(&t, EV(NGE_KEY_ENTER)) == TARGET_STEP_MOVING && t.x == 34 && t.y == 20);
    CHECK(said(0, "That is out of range."));
    CHECK(target_key(&t, EV(NGE_KEY_Q)) == TARGET_STEP_CANCELLED);
    CHECK(t.x == NGE_MAP_X_INVALID && t.y == NGE_MAP_Y_INVALID);
    CHECK(said(0, "Never mind."));

    /* ---- target_choose(): frames until the player answers ---- */
    nge_map_set_camera(0, 0);   /* moved away: put back on the player at the end */
    context(&t, 30, 20, TARGET_RANGE_ANY);
    queue_key(EV(NGE_KEY_LEFT));   /* typed before targeting: thrown away */
    script = right_right_enter;
    play_helps = 0;
    waits = 0;
    CHECK(target_choose(&t) == 1);
    CHECK(t.x == 32 && t.y == 20 && waits == 3 && !out_of_keys);
    CHECK(said(0, "Look at what?"));
    CHECK(play_helps == 1 && nge_key_get() == NGE_KEY_NONE);
    x = nge_map.cam_x;
    y = nge_map.cam_y;
    nge_map_centre_camera(30, 20);
    CHECK(nge_map.cam_x == x && nge_map.cam_y == y);   /* back on the player */

    context(&t, 30, 20, TARGET_RANGE_ANY);
    script = cancel;
    CHECK(target_choose(&t) == 0 && !out_of_keys);
    CHECK(t.x == NGE_MAP_X_INVALID && t.y == NGE_MAP_Y_INVALID);

    context(&t, 30, 20, 1);   /* out of range is refused, and targeting goes on */
    script = out_then_back;
    CHECK(target_choose(&t) == 1 && !out_of_keys);
    CHECK(t.x == 30 && t.y == 21);
    CHECK(said(0, "That is out of range.") && said(1, "Look at what?"));
    script = NULL;

    /* ---- asserts: a start off the map, an unknown purpose ---- */
    context(&t, 30, 20, TARGET_RANGE_ANY);
    t.x = LEVEL_WIDTH;
    TEST_EXPECT_HALT(target_choose(&t));
    CHECK(test_halt_count == 1);
    context(&t, 30, 20, TARGET_RANGE_ANY);
    t.purpose = TARGET_PURPOSE_COUNT;
    TEST_EXPECT_HALT(target_choose(&t));
    CHECK(test_halt_count == 1);

    /* ---- look: one message per entity with a kind, in ID order ---- */
    ring = gen_item(IT_RING, LEVEL_AREA);
    sword = gen_item(IT_SWORD, LEVEL_AREA);
    nge_location_set(ring, 10, 5);
    nge_location_set(sword, 10, 5);
    nge_event_flush();
    messages_look(10, 5);
    CHECK(said(1, "You see a ring of health.") && said(0, "You see a sword."));
    messages_look(11, 5);
    CHECK(said(0, "You see nothing there."));
    messages_look(30, 20);
    CHECK(said(0, "You see yourself."));
    nge_location_set(ring, 30, 20);
    messages_look(30, 20);
    CHECK(said(1, "You see yourself.") && said(0, "You see a ring of health."));

    return test_summary("target");
}
