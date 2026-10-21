/* game/anim: the animations events start (slides, lunges, sequences), and holding the world. */
#include "test.h"

#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "map/nge_map.h"

#include "game/anim/anim.h"
#include "game/anim/anim_internal.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/level/level.h"

#define AREA  NGE_OWNER_AREA(0)

static const nge_rect_t window = { .x1 = 0, .x2 = 19, .y1 = 0, .y2 = 19 };   /* a view of 10 x 10 tiles */

/* The screen's map view, for the level module (screen.c isn't linked) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

static void dispatch(const nge_event_t *e)
{
    anim_on_event(e);
}

/* Post an event and handle it */
static void post(nge_entity_id_t target, uint8_t id, nge_entity_id_t source, int8_t p1, int8_t p2)
{
    nge_event_post(target, id, source, (uint8_t)p1, (uint8_t)p2);
    nge_event_process(dispatch);
}

/* Updates until nothing is busy: how many it took (at most 20) */
static int settle(void)
{
    int n;

    for (n = 0; n < 20 && nge_render_busy(); n++) {
        nge_render_update();
    }
    return n;
}

static nge_entity_id_t creature(uint8_t kind, uint8_t x, uint8_t y)
{
    nge_entity_id_t id = gen_creature(kind, AREA);

    nge_location_set(id, x, y);
    return id;
}

int main(void)
{
    nge_entity_id_t player;
    nge_entity_id_t rat;
    nge_entity_id_t other;
    int dx;
    int dy;
    int ok;

    test_init();

    nge_map_init(30, 30, &window);                       /* view: tiles 0-9 */
    nge_entity_init();
    nge_render_init();
    nge_timer_init();
    nge_event_init();
    level_depth = 0;                                     /* LEVEL_AREA is AREA */
    nge_render_set_view(AREA);
    anim_init();
    player = creature(CR_PLAYER, 5, 5);
    rat = creature(CR_RAT, 6, 5);
    nge_event_flush();                                   /* the player's innate effect */

    /* ---- the kinds' sequences: copied from the base records, none yet ---- */
    ok = 1;
    for (dx = 0; dx < CR_COUNT; dx++) {
        for (dy = 0; dy < GEN_SEQ_COUNT; dy++) {
            ok = ok && anim_seqs[dx][dy].count == 0;
        }
    }
    CHECK(ok);

    /* ---- a move slides from the old tile: (dx, dy) tiles back ---- */
    CHECK(!anim_holding());
    post(player, EV_MOVED, NGE_EVENT_NONE, 1, 0);        /* moved right, onto (5, 5) */
    CHECK(anim_holding());
    CHECK(nge_render_sprite_mox[player] == -16 && nge_render_sprite_moy[player] == 0);   /* a tile: 16 pixels */
    CHECK(nge_render_sprite_oneshot[player] == 0);       /* no move sequence */
    CHECK(settle() == 4 && !anim_holding());             /* 4 frames a tile, the key repeat rate */
    post(player, EV_MOVED, NGE_EVENT_NONE, -1, 1);       /* moved down-left */
    CHECK(nge_render_sprite_mox[player] == 16 && nge_render_sprite_moy[player] == -16);
    settle();

    /* ---- an attack lunges the attacker towards the target, hit or missed, in each direction ---- */
    ok = 1;
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            nge_location_set(rat, (uint8_t)(5 + dx), (uint8_t)(5 + dy));
            post(rat, (dx + dy) & 1 ? EV_HIT : EV_MISSED, player, 10, NGE_EVENT_NONE);
            ok = ok && nge_render_sprite_mtx[player] == dx * ANIM_LUNGE_PIXELS &&
                 nge_render_sprite_mty[player] == dy * ANIM_LUNGE_PIXELS && anim_holding();
            ok = ok && nge_render_sprite_mspeed[rat] == 0;   /* the target doesn't move */
            ok = ok && settle() == 2 * ANIM_LUNGE_PIXELS / ANIM_LUNGE_SPEED;
        }
    }
    CHECK(ok);
    /* No attacker (a trap, later): nothing */
    post(rat, EV_HIT, NGE_EVENT_NONE, 10, NGE_EVENT_NONE);
    CHECK(!anim_holding());

    /* ---- a kind's sequence plays with the action, then the rest animation ---- */
    anim_seqs[CR_PLAYER][GEN_SEQ_MOVE].first = 4;
    anim_seqs[CR_PLAYER][GEN_SEQ_MOVE].count = 2;
    anim_seqs[CR_PLAYER][GEN_SEQ_MOVE].rate = 1;
    anim_seqs[CR_PLAYER][GEN_SEQ_ATTACK].first = 6;
    anim_seqs[CR_PLAYER][GEN_SEQ_ATTACK].count = 1;
    anim_seqs[CR_PLAYER][GEN_SEQ_ATTACK].rate = 1;
    post(player, EV_MOVED, NGE_EVENT_NONE, 0, 1);
    CHECK(nge_render_sprite_oneshot[player] == 1 && nge_render_sprite_first[player] == 4);
    settle();
    CHECK(nge_render_sprite_oneshot[player] == 0 && nge_render_sprite_first[player] == nge_render_sprite_rest_first[player]);
    nge_location_set(rat, 6, 5);
    post(rat, EV_MISSED, player, 1, NGE_EVENT_NONE);
    CHECK(nge_render_sprite_oneshot[player] == 1 && nge_render_sprite_first[player] == 6);
    settle();
    anim_init();                                         /* back to the base records' */

    /* ---- skipped: no sprite, or not on the current level ---- */
    nge_render_remove(rat);
    post(rat, EV_MOVED, NGE_EVENT_NONE, 1, 0);
    post(player, EV_HIT, rat, 10, NGE_EVENT_NONE);       /* the rat's lunge */
    CHECK(!anim_holding());
    other = gen_creature(CR_RAT, NGE_OWNER_AREA(1));
    nge_location_set(other, 4, 5);
    post(other, EV_MOVED, NGE_EVENT_NONE, 1, 0);
    post(player, EV_HIT, other, 10, NGE_EVENT_NONE);
    CHECK(!anim_holding());

    /* ---- out of view: no slide, nothing held (the render system's rule) ---- */
    nge_location_set(player, 25, 25);
    post(player, EV_MOVED, NGE_EVENT_NONE, 1, 0);
    CHECK(!anim_holding() && nge_render_sprite_mox[player] == 0);

    return test_summary("anim");
}
