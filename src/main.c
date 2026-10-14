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
#include "input/nge_input.h"
#include "map/nge_map.h"
#include "io/nge_cspect_stream.h"
#include "sprite/nge_sprite.h"

#include "game/actors.h"
#include "game/level.h"
#include "game/screen.h"

int main(void)
{
    uint8_t space_held = 0;

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
    nge_render_set_view(LEVEL_AREA, &level_map);
    actors_init();

    nge_map_centre_camera(&level_map, NGE_LOCATION_X(actor_player), NGE_LOCATION_Y(actor_player));

    /* Show the scene, then ask for the player's name (blocking: the scene stands still) */
    screen_draw_stats();
    nge_render_update();
    nge_frame_wait_vblank();
    nge_render_upload();
    nge_map_update(&level_map);
    screen_ask_name();
    screen_draw_stats();
    screen_message("Welcome to the dungeon.");
    screen_message("Move with the cursor keys or h j k l.");

    nge_util_info("init done");

    while (1) {
        nge_key_event_t ev;
        uint8_t held;

        nge_frame_wait_vblank();
        /* First thing in the blanking period, so nothing changes mid-frame */
        nge_render_upload();
        nge_map_update(&level_map);
        nge_input_update();
        nge_ecs_check_guard();

        /* The player's turn: one command per frame, from the key events */
        ev = nge_key_get();
        if (ev != NGE_KEY_NONE) {
            actors_player_command(ev);
        }

        /* A key held, read from the snapshot */
        held = nge_key_down(NGE_KEY_SPACE);
        if (held != space_held) {
            space_held = held;
            screen_show_held(held);
        }

        /* Build the sprites for the next frame */
        nge_render_update();
    }
}
