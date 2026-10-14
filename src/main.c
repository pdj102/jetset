/*
 * Entry point: the basis of a classic roguelike. Sets up the screen
 * (src/game/screen.h), one level with a test layout (src/game/level.h) and
 * its entities (src/game/actors.h), then runs the frame loop.
 */
#include "core/nge_util.h"
#include "ecs/components/nge_location.h"
#include "ecs/nge_ecs.h"
#include "ecs/systems/nge_render.h"
#include "hw/nge_frame.h"
#include "hw/nge_hw.h"
#include "map/nge_map.h"
#include "io/nge_cspect_stream.h"
#include "sprite/nge_sprite.h"

#include "game/actors.h"
#include "game/level.h"
#include "game/screen.h"

int main(void)
{
    /* Disables interrupts and every interrupt source first (see nge_hw.h) */
    nge_hw_init();

    /* Send asserts and debug output to the CSpect console (NULL on hardware) */
    nge_util_set_debug_stream(nge_open_cspect_debug());

    screen_init();
    nge_sprite_init();
    level_init();

    /* Entities: the render system draws those in the level's area */
    nge_ecs_init();     /* ECS data page into slot 0; makes every entity free */
    nge_render_init();
    nge_render_set_view(LEVEL_AREA, &level_map);
    actors_init();

    nge_map_centre_camera(&level_map, NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));

    screen_draw_stats();
    screen_message("Welcome to the dungeon, Paul.");
    screen_message("You see a potion in the room to the south.");

    nge_util_info("init done");

    while (1) {
        nge_frame_wait_vblank();
        /* First thing in the blanking period, so nothing changes mid-frame */
        nge_render_upload();
        nge_map_update(&level_map);
        nge_ecs_check_guard();

        /* Game logic goes here: input, the player's turn, monsters */

        /* Build the sprites for the next frame */
        nge_render_update();
    }
}
