#ifndef SCREEN_INTERNAL_H
#define SCREEN_INTERNAL_H

/*
 * Shared by the ui module's files; not for use outside it:
 * - screen.c (common): the help line;
 * - screen_bank.c (page 116): opening and closing overlay screens;
 * - screen_main.c (common), screen_main_bank.c (page 116): the main screen;
 * - screen_inv_bank.c (page 116): the inventory screen;
 * - screen_title_bank.c (page 116): the title screen;
 * - screen_pdata.c (page 117): the inventory's list, mapped into slot 7
 *   while the module's code runs.
 */

#include <stdint.h>

#include "core/nge_types.h"
#include "ecs/nge_entity.h"
#include "io/nge_stream.h"
#include "sprite/nge_sprite.h"
#include "ui/nge_list.h"

#include "game/ui/screen.h"
#include "game/ui/screen_main.h"

/* The help line of each buffer, by SCREEN_BUFFER_*: buffer 1's is open only while an overlay screen is */
extern NGE_FILE *screen_help_lines[2];

/* Overlay screens open, 0 to SCREEN_DEPTH_MAX: 0 while the main screen is shown */
extern uint8_t screen_depth;

/* The help line's cells (screen_bank.c: in page 116, for the module's banked code only) */
extern const nge_rect_t screen_help_area;

/* ---- The main screen (screen_main.c) ---- */

/* The message window's bottom line has text: the next message goes on a new line */
extern uint8_t screen_message_line_used;

/* Where messages go instead of the message window (screen_message_to()), or NULL */
extern NGE_FILE *screen_message_stream;

/* The overlay sprites' records */
extern nge_sprite_attr_t screen_overlay[SCREEN_OVERLAY_COUNT];

/* ---- The inventory's page data (screen_pdata.c) ---- */

#define SCREEN_INV_MAX  30   /* items listed at most: a pack of 20, and 10 in use */

/* The inventory's panels, by number */
#define SCREEN_INV_TITLE    0
#define SCREEN_INV_LIST     1
#define SCREEN_INV_DETAILS  2
#define SCREEN_INV_STATUS   3
#define SCREEN_INV_PANELS   4

extern nge_entity_id_t screen_inv_ids[SCREEN_INV_MAX];       /* the items listed, in order */
extern nge_list_t screen_inv_list;                           /* their cursor and rows */
extern NGE_FILE *screen_inv_streams[SCREEN_INV_PANELS];      /* the panels' streams, by SCREEN_INV_* */

#endif
