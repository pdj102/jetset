/*
 * Entry point: the basis of a classic roguelike. Sets up the screen
 * (src/game/screen.h), one level with a test layout (src/game/level.h) and
 * its entities (src/game/actors.h), then runs the game loop
 * (src/game/game.h).
 */
#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_ecs.h"
#include "ecs/systems/nge_render.h"
#include "ecs/systems/nge_timer.h"
#include "hw/nge_hw.h"
#include "input/nge_input.h"
#include "map/nge_map.h"
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

    /* Entities: the render system draws those in the level's area */
    nge_ecs_init();     /* ECS data page into slot 0; makes every entity free */
    nge_render_init();
    nge_render_set_view(LEVEL_AREA);
    nge_timer_init();   /* the clock: turn 0 */
    actors_init();

    nge_map_centre_camera(NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));

    nge_util_info("init done");

    /* The main loop: the title, then play (see src/game/game.h) */
    game_run();
    return 0;   /* not reached: game_run() never returns */
}
