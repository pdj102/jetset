/*
 * The main screen's common part: the map view's cells (nge_map reads them),
 * its streams, the messages (banked callers pass their literals) and the
 * overlay sprites (uploaded every frame). Setting up and the stats are in
 * screen_main_bank.c.
 */
#include "game/ui/screen_main.h"

#include <stddef.h>

#include "core/nge_util.h"
#include "io/nge_print.h"
#include "io/nge_text.h"
#include "sprite/nge_sprite.h"

#include "game/ui/screen_internal.h"

/* The map view is whole map tiles: 2 x 2 hardware tiles each (map/nge_map.h) */
const nge_rect_t screen_map_view = { .x1 = 0, .x2 = 27, .y1 = 0, .y2 = 25 };

NGE_FILE *screen_stats = NULL;
NGE_FILE *screen_messages = NULL;
char screen_name[SCREEN_NAME_MAX + 1] = "Rogue";

uint8_t screen_message_line_used;
NGE_FILE *screen_message_stream = NULL;

/* The overlay sprites' records: common memory, so the upload can read them from any caller */
nge_sprite_attr_t screen_overlay[SCREEN_OVERLAY_COUNT];

/*
 * The new line comes before each message, not after it: the newest message
 * stays on the bottom line, and all the window's lines show messages
 */
void screen_message_line(void)
{
    if (screen_message_line_used) {
        nge_putc(NGE_CTRL_LF, screen_messages);   /* scrolls once at the bottom */
    }
    screen_message_line_used = 1;
}

void screen_overlay_set(uint8_t n, uint16_t x, uint16_t y, uint8_t pattern, uint8_t flags)
{
    nge_util_assert(n < SCREEN_OVERLAY_COUNT);

    nge_sprite_set(&screen_overlay[n], x, y, pattern, flags, NGE_SPRITE_PATTERN4 | NGE_SPRITE_SCALE_1);
}

void screen_overlay_hide(uint8_t n)
{
    nge_util_assert(n < SCREEN_OVERLAY_COUNT);

    nge_sprite_clear(&screen_overlay[n], 1);
}

void screen_overlay_upload(void)
{
    nge_sprite_upload(SCREEN_SPRITE_OVERLAY, screen_overlay, SCREEN_OVERLAY_COUNT);
}

void screen_message_to(NGE_FILE *stream)
{
    screen_message_stream = stream;
}

void screen_message(const char *text)
{
    if (screen_message_stream != NULL) {
        nge_text_cls(screen_message_stream);
        nge_puts(text, screen_message_stream);
        return;
    }
    screen_message_line();
    nge_text_ink(NGE_INK_WHITE, screen_messages);
    nge_puts(text, screen_messages);
}
