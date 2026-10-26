/*
 * Entry point: the basis of a classic roguelike. Sets up the hardware, the
 * screen (src/game/ui/screen_main.h), the map (src/game/level/level.h) and the
 * graphics once, then runs the game loop (src/game/game.h): the title,
 * then a game (play_run(): the player and the first level).
 */
#include "core/nge_util.h"
#include "ecs/nge_ecs.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "hw/nge_hw.h"
#include "input/nge_input.h"
#include "io/nge_cspect_stream.h"
#include "sprite/nge_sprite.h"

#include "game/actors.h"
#include "game/anim/anim.h"
#include "game/game.h"
#include "game/level/level.h"
#include "game/ui/screen_main.h"

int main(void)
{
    /* Disables interrupts and every interrupt source first (see nge_hw.h) */
    nge_hw_init();

    /* Send asserts and debug output to the CSpect console (NULL on hardware) */
    nge_util_set_debug_stream(nge_open_cspect_debug());

    screen_init();
    nge_input_init();
    nge_sprite_init();
    level_init();

    /* Every module the frames use in a valid state: each new game sets them up again (play_start()) */
    nge_ecs_init();     /* ECS data page into slot 0; every entity free */
    nge_render_init();
    nge_timer_init();
    actors_load_graphics();
    anim_init();        /* the creature kinds' sequences */

    nge_util_info("init done");

    /* The main loop: the title, then play (see src/game/game.h) */
    game_run();
    return 0;   /* not reached: game_run() never returns */
}
