/*
 * Entry point: the basis of a classic roguelike. Sets up the hardware, the
 * screen (src/game/screen.h), the map (src/game/level/level.h) and the
 * graphics once, then runs the game loop (src/game/game.h): the title,
 * then a new game (GAME_NEW_GAME: the player and the first level).
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
#include "game/game.h"
#include "game/level/level.h"
#include "game/screen.h"

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

    /* Every module the frame loop uses in a valid state: GAME_NEW_GAME sets them up again for each game */
    nge_ecs_init();     /* ECS data page into slot 0; every entity free */
    nge_render_init();
    nge_timer_init();
    actors_load_graphics();

    nge_util_info("init done");

    /* The main loop: the title, then play (see src/game/game.h) */
    game_run();
    return 0;   /* not reached: game_run() never returns */
}
