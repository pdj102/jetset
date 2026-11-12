/* game/anim: the animations and visual effects events start (slides, lunges, sequences; bolts, glows, splashes, flashes, stand-ins), and holding the world. */
#include "test.h"

#include <string.h>

#include "data/sprite_data.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_entity.h"
#include "ecs/nge_owner.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_render_internal.h"
#include "ecs/systems/nge_timer.h"
#include "event/nge_event.h"
#include "map/nge_map.h"
#include "vfx/nge_vfx.h"
#include "vfx/nge_vfx_internal.h"

#include "game/anim/anim.h"
#include "game/anim/anim_internal.h"
#include "game/actors.h"
#include "game/components/health.h"
#include "game/effects/effects.h"
#include "game/events.h"
#include "game/gen/gen.h"
#include "game/gen/gen_kinds.h"
#include "game/level/level.h"
#include "game/spells/spells.h"

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

/* VFX updates until none is left: how many it took (at most 100) */
static int settle_vfx(void)
{
    int n;

    for (n = 0; n < 100 && nge_vfx_busy(); n++) {
        nge_render_update();
        nge_vfx_update();
    }
    return n;
}

/* VFX slots in use of a kind (NGE_VFX_BOLT, NGE_VFX_BURST) */
static int vfx_count(uint8_t kind)
{
    int i;
    int n = 0;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        n += nge_vfx_kind[i] == kind;
    }
    return n;
}

/* The slot in use of a kind showing pattern first, or -1 */
static int vfx_find(uint8_t kind, uint8_t first)
{
    int i;

    for (i = 0; i < NGE_VFX_SLOTS; i++) {
        if (nge_vfx_kind[i] == kind && nge_vfx_first[i] == first) {
            return i;
        }
    }
    return -1;
}

static int vfx_bank(int slot)
{
    return nge_vfx_flags[slot] >> 4;
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
    nge_vfx_init(68);
    nge_timer_init();
    nge_event_init();
    level_depth = 0;                                     /* LEVEL_AREA is AREA */
    nge_render_set_view(AREA);
    anim_init();
    player = creature(CR_PLAYER, 5, 5);
    rat = creature(CR_RAT, 6, 5);
    nge_event_flush();                                   /* the player's innate effect */

    /* ---- the kinds' sequences: copied from the kind records; the player's from the art ---- */
    CHECK(anim_seqs[CR_PLAYER][GEN_SEQ_MOVE].count == 0);   /* walking is the slide alone */
    CHECK(anim_seqs[CR_PLAYER][GEN_SEQ_ATTACK].first == SPRITE_PLAYER_ATTACK &&
          anim_seqs[CR_PLAYER][GEN_SEQ_ATTACK].count == SPRITE_PLAYER_ATTACK_COUNT &&
          anim_seqs[CR_PLAYER][GEN_SEQ_ATTACK].rate == SPRITE_PLAYER_ATTACK_RATE);
    ok = 1;
    for (dx = CR_PLAYER + 1; dx < CR_COUNT; dx++) {
        for (dy = 0; dy < GEN_SEQ_COUNT; dy++) {
            /* the classes' kinds (game/chargen) have the player's */
            ok = ok && anim_seqs[dx][dy].count == ((dx == CR_FIGHTER || dx == CR_MAGE) ? anim_seqs[CR_PLAYER][dy].count : 0);
        }
    }
    CHECK(ok);
    /* The attack sequence ends with the lunge, so it doesn't slow fighting (the world waits for it) */
    CHECK(SPRITE_PLAYER_ATTACK_COUNT * SPRITE_PLAYER_ATTACK_RATE <= 2 * ANIM_LUNGE_PIXELS / ANIM_LUNGE_SPEED);

    /* The motions alone first: the player without its sequences */
    memset(anim_seqs[CR_PLAYER], 0, sizeof(anim_seqs[CR_PLAYER]));

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
    anim_init();                                         /* back to the kind records' */

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

    /* ======== visual effects (docs/vfx.md) ======== */
    {
        nge_entity_id_t bow;
        nge_entity_id_t dying;
        nge_render_look_t look;
        int s;
        int t;

        nge_location_set(player, 2, 5);
        rat = creature(CR_RAT, 6, 5);                       /* 4 tiles to the right: 64 pixels */
        nge_event_flush();
        nge_render_get(rat, &look);

        /* ---- a shot flies from the shooter to the target: an arrow from a bow, pointing its way ---- */
        bow = gen_item(IT_SHORT_BOW, NGE_OWNER_WORLD);
        post(rat, EV_FIRED, player, (int8_t)bow, 0);
        s = vfx_find(NGE_VFX_BOLT, SPRITE_FX_ARROW);
        CHECK(s >= 0 && vfx_count(NGE_VFX_BOLT) == 1);
        CHECK(nge_vfx_x[s] == 2 * 16 && nge_vfx_ex[s] == 6 * 16 && nge_vfx_ey[s] == 5 * 16);
        CHECK(nge_vfx_speed[s] == ANIM_SHOT_SPEED && (nge_vfx_mode[s] & NGE_VFX_DIRECTED));
        CHECK(anim_holding() && !nge_render_busy());       /* the VFX alone holds the world */
        CHECK(settle_vfx() == 64 / ANIM_SHOT_SPEED && !anim_holding());
        /* No launcher (a thrown item, a creature's own): the thrown look */
        post(rat, EV_FIRED, player, (int8_t)NGE_EVENT_NONE, 0);
        CHECK(vfx_find(NGE_VFX_BOLT, SPRITE_FX_THROWN) >= 0);
        nge_vfx_clear();

        /* ---- a spell at another creature: an orb in its effect's colour; at the caster, none ---- */
        post(rat, EV_CAST, player, SP_FIRE_BOLT, 0);
        s = vfx_find(NGE_VFX_BOLT, SPRITE_FX_ORB);
        CHECK(s >= 0 && vfx_bank(s) == BANK_ORANGE && nge_vfx_speed[s] == ANIM_MAGIC_SPEED);
        nge_vfx_clear();
        post(player, EV_CAST, player, SP_CURE_WOUNDS, 0);
        CHECK(vfx_count(NGE_VFX_BOLT) == 0);

        /* ---- an effect taking hold glows in its colour; innate ones and damaging bolts don't ---- */
        post(rat, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_HOLD, 0);
        s = vfx_find(NGE_VFX_BURST, SPRITE_FX_GLOW);
        CHECK(s >= 0 && vfx_bank(s) == BANK_DARK && nge_vfx_delay[s] == 0);
        nge_vfx_clear();
        post(player, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_REGEN, 0);
        post(rat, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_BOLT_FIRE, 0);
        CHECK(!nge_vfx_busy());

        /* ---- damage: the white flash and the splash in the type's colour, at the lunge's peak ---- */
        nge_render_get(rat, &look);
        post(rat, EV_DAMAGED, player, 3, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_COLD));
        s = vfx_find(NGE_VFX_BURST, look.pattern);
        t = vfx_find(NGE_VFX_BURST, SPRITE_FX_SPLASH);
        CHECK(s >= 0 && vfx_bank(s) == BANK_FLASH && nge_vfx_delay[s] == ANIM_IMPACT_DELAY &&
              nge_vfx_rate[s] == ANIM_FLASH_FRAMES);
        CHECK(t >= 0 && vfx_bank(t) == BANK_ICE && nge_vfx_delay[t] == ANIM_IMPACT_DELAY);
        CHECK(s < t);                                         /* the splash draws over the flash */
        nge_vfx_clear();
        post(rat, EV_DAMAGED, player, 0, (int8_t)EV_DMG_PACK(DMG_IMMUNE, DMG_FIRE));
        CHECK(!nge_vfx_busy());                               /* 0 taken: nothing */

        /* ---- a shot and its damage together: the splash waits for the shot to land ---- */
        post(rat, EV_FIRED, player, (int8_t)bow, 0);
        nge_event_post(rat, EV_HIT, player, 15, bow);
        post(rat, EV_DAMAGED, player, 2, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_PIERCE));
        t = vfx_find(NGE_VFX_BURST, SPRITE_FX_SPLASH);
        CHECK(t >= 0 && nge_vfx_delay[t] == 64 / ANIM_SHOT_SPEED - 1);
        CHECK(vfx_bank(t) == BANK_RED);
        settle_vfx();
        settle();

        /* ---- a kill: a stand-in until the bolt lands, then its flash, then the puff ---- */
        dying = rat;
        post(dying, EV_CAST, player, SP_MAGIC_BOLT, 0);       /* 64 pixels at 4: lands after 15 updates */
        nge_render_get(dying, &look);                         /* what it shows now: its rest animation moves on */
        nge_entity_destroy(dying);                            /* health_damage() destroys before the events */
        nge_event_post(dying, EV_DAMAGED, player, 9, EV_DMG_PACK(DMG_NORMAL, DMG_MAGIC));
        post(dying, EV_DIED, player, 0, 0);
        CHECK(vfx_count(NGE_VFX_BURST) == 4);                 /* splash, stand-in, flash, puff: no hit flash */
        s = -1;
        t = -1;
        {
            int i;
            int hold = -1;

            for (i = 0; i < NGE_VFX_SLOTS; i++) {
                if (nge_vfx_kind[i] == NGE_VFX_BURST && nge_vfx_first[i] == look.pattern) {
                    if (nge_vfx_mode[i] & NGE_VFX_HOLD) {
                        hold = i;
                    } else {
                        s = i;
                    }
                }
            }
            t = vfx_find(NGE_VFX_BURST, SPRITE_FX_PUFF);
            CHECK(hold >= 0 && nge_vfx_delay[hold] == 64 / ANIM_MAGIC_SPEED - 1 && vfx_bank(hold) == (look.flags >> 4));
            CHECK(s >= 0 && vfx_bank(s) == BANK_FLASH && nge_vfx_delay[s] == 64 / ANIM_MAGIC_SPEED - 1);
            CHECK(t >= 0 && nge_vfx_delay[t] == 64 / ANIM_MAGIC_SPEED - 1 + ANIM_FLASH_FRAMES && vfx_bank(t) == BANK_BONE);
        }
        CHECK(anim_holding());
        settle_vfx();
        CHECK(!anim_holding());
        /* Killed in melee: the stand-in for the lunge's peak */
        rat = creature(CR_RAT, 3, 5);
        nge_event_flush();
        nge_render_get(rat, &look);
        nge_entity_destroy(rat);
        post(rat, EV_DIED, player, 0, 0);
        s = vfx_find(NGE_VFX_BURST, look.pattern);
        CHECK(s >= 0 && nge_vfx_delay[s] == ANIM_IMPACT_DELAY);
        settle_vfx();
        /* The player isn't destroyed (persistent): no stand-in */
        post(player, EV_DIED, rat, 0, 0);
        CHECK(!nge_vfx_busy());

        /* ---- off the current level, or out of view: nothing ---- */
        post(other, EV_DAMAGED, player, 3, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_SLASH));
        post(other, EV_FIRED, player, (int8_t)bow, 0);
        CHECK(!nge_vfx_busy());
        rat = creature(CR_RAT, 25, 25);
        nge_event_flush();
        post(rat, EV_DAMAGED, player, 3, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_SLASH));
        CHECK(!nge_vfx_busy() && !anim_holding());
    }

    /* ======== an area's VFX: a bolt to the centre, a burst there, impacts after it (docs/items_aimed.md 4.8) ======== */
    {
        nge_entity_id_t wand = gen_item(IT_WAND_FIREBALL, NGE_OWNER_WORLD);
        nge_entity_id_t panic = gen_item(IT_SP_PANIC, NGE_OWNER_WORLD);
        nge_entity_id_t near;
        int lands = 64 / ANIM_MAGIC_SPEED - 1;   /* the centre is 4 tiles from the player */
        int s;
        int t;

        anim_vfx_clear();
        nge_location_set(player, 2, 5);
        near = creature(CR_RAT, 6, 6);                      /* next to the centre (6, 5) */
        nge_event_flush();

        /* ---- no target: nothing flies; EV_AREA flies the effect's bolt and bursts when it lands ---- */
        post(NGE_EVENT_NONE, EV_AIMED, player, (int8_t)wand, 0);
        CHECK(!nge_vfx_busy());
        post(NGE_EVENT_NONE, EV_AREA, player, 6, 5);
        s = vfx_find(NGE_VFX_BOLT, SPRITE_FX_ORB);
        t = vfx_find(NGE_VFX_BURST, SPRITE_FX_SPLASH);
        CHECK(s >= 0 && vfx_bank(s) == BANK_ORANGE && nge_vfx_speed[s] == ANIM_MAGIC_SPEED);
        CHECK(nge_vfx_x[s] == 2 * 16 && nge_vfx_ex[s] == 6 * 16 && nge_vfx_ey[s] == 5 * 16);
        CHECK(t >= 0 && vfx_bank(t) == BANK_ORANGE && nge_vfx_x[t] == 6 * 16 && nge_vfx_y[t] == 5 * 16 &&
              nge_vfx_delay[t] == lands);
        /* An impact and a glow on another tile wait for the bolt to the centre */
        post(near, EV_DAMAGED, player, 4, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_FIRE));
        post(near, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_HOLD, 0);
        {
            int i;
            int splash = 0;
            int flashed = 0;
            int glowed = 0;

            for (i = 0; i < NGE_VFX_SLOTS; i++) {
                if (nge_vfx_kind[i] != NGE_VFX_BURST || nge_vfx_y[i] != 6 * 16) {
                    continue;
                }
                splash += nge_vfx_first[i] == SPRITE_FX_SPLASH && nge_vfx_delay[i] == lands;
                flashed += vfx_bank(i) == BANK_FLASH && nge_vfx_delay[i] == lands;
                glowed += nge_vfx_first[i] == SPRITE_FX_GLOW && nge_vfx_delay[i] == lands;
            }
            CHECK(splash == 1 && flashed == 1 && glowed == 1);
        }
        /* Cleared, as on a change of level: nothing shows, and the centre is forgotten */
        anim_vfx_clear();
        CHECK(!nge_vfx_busy());
        post(near, EV_DAMAGED, player, 4, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_FIRE));
        t = vfx_find(NGE_VFX_BURST, SPRITE_FX_SPLASH);
        CHECK(t >= 0 && nge_vfx_delay[t] == ANIM_IMPACT_DELAY);
        anim_vfx_clear();

        /* ---- a spell's area too; a later EV_FIRED forgets the centre ---- */
        post(NGE_EVENT_NONE, EV_CAST, player, SP_FROST_BOLT, 0);
        post(NGE_EVENT_NONE, EV_AREA, player, 6, 5);
        s = vfx_find(NGE_VFX_BOLT, SPRITE_FX_ORB);
        CHECK(s >= 0 && vfx_bank(s) == BANK_ICE);
        post(player, EV_FIRED, player, (int8_t)NGE_EVENT_NONE, 0);   /* at itself: nothing flies */
        post(near, EV_DAMAGED, player, 4, (int8_t)EV_DMG_PACK(DMG_NORMAL, DMG_COLD));
        {
            int i;
            int early = 0;

            for (i = 0; i < NGE_VFX_SLOTS; i++) {
                early += nge_vfx_kind[i] == NGE_VFX_BURST && nge_vfx_y[i] == 6 * 16 &&
                         nge_vfx_first[i] == SPRITE_FX_SPLASH && nge_vfx_delay[i] == ANIM_IMPACT_DELAY;
            }
            CHECK(early == 1);
        }
        anim_vfx_clear();

        /* ---- a burst: the centre is the user's tile, nothing flies, and a glow doesn't wait ---- */
        post(NGE_EVENT_NONE, EV_AIMED, player, (int8_t)panic, 0);
        post(NGE_EVENT_NONE, EV_AREA, player, 2, 5);
        CHECK(!nge_vfx_busy());
        post(near, EV_EFFECT_STARTED, NGE_EVENT_NONE, EF_FEAR, 0);
        s = vfx_find(NGE_VFX_BURST, SPRITE_FX_GLOW);
        CHECK(s >= 0 && nge_vfx_delay[s] == 0 && vfx_count(NGE_VFX_BOLT) == 0);
        anim_vfx_clear();
    }

    return test_summary("anim");
}
