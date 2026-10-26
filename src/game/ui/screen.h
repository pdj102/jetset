#ifndef SCREEN_H
#define SCREEN_H

/**
 * @file screen.h
 * @brief In-game screens: the main screen of play, and the overlay screens
 *        shown over it (the inventory, the title), with the help line
 *        under them all. See docs/ui_screens.md.
 *
 *     rows 0-30   the screen shown: its panels (ui/nge_panel.h)
 *     row  31     the help line: what the keys do now (screen_help())
 *
 * The main screen (game/ui/screen_main.h) is on Layer 2 buffer
 * SCREEN_BUFFER_MAIN, and is always under the others. An overlay screen
 * is drawn on buffer SCREEN_BUFFER_OVERLAY, covering the whole display,
 * and pauses the game while it is open: no time passes, no animation
 * plays, no sprite moves. Buffer 0 is left as it was, so closing the
 * overlay shows the main screen again at once, with whatever was written
 * to it meanwhile (messages, stats). Each overlay screen is a run function
 * in its own file (screen_inv.h, screen_title.h), a chooser
 * (game/game.h) or a game phase, built from these steps:
 *
 *     screen_open();                     the game paused; buffer 1's help line
 *     open the panels on SCREEN_BUFFER_OVERLAY, draw them, screen_help(...)
 *     ev = screen_wait_key();            the first frame shows the screen, whole
 *     ... handle the keys, redraw what changes ...
 *     close the panels' streams
 *     screen_close();                    the screen beneath shown again
 *
 * An overlay screen may open another (SCREEN_DEPTH_MAX deep): when it
 * closes, the one beneath draws itself again, as it shares buffer 1.
 *
 * Banking: screen_help() is in common memory, so banked callers can pass
 * their literals. The rest is banked (NGE_BANKED, page 116): don't call it
 * while game code has its own page mapped in slot 6 or 7.
 */

#include "core/nge_bank.h"
#include "input/nge_input.h"

/* Layer 2 buffers (layer2/nge_layer2.h) */
#define SCREEN_BUFFER_MAIN     0   /**< The main screen's. */
#define SCREEN_BUFFER_OVERLAY  1   /**< The overlay screens'. */
#define SCREEN_BUFFER_COUNT    2   /**< The number of buffers used by the screens. */

#define SCREEN_DEPTH_MAX  2       /**< Overlay screens open at once, at most: one, and one over it. */
#define SCREEN_PAPER      0x00    /**< The panels' background (RRRGGGBB): black. */
#define SCREEN_HIGHLIGHT  0x02    /**< A selected row's background (RRRGGGBB): blue. */
#define SCREEN_HELP_ROW   31      /**< The help line's row. */

/** @brief Show text on the help line (row 31) of the screen shown, replacing what was there: what the keys do now. */
void screen_help(const char *text);

/**
 * @brief Open an overlay screen: the game paused, waiting key events
 *        thrown away, and the help line of SCREEN_BUFFER_OVERLAY cleared.
 *        The caller then draws the screen on SCREEN_BUFFER_OVERLAY; it is
 *        shown at the next frame (screen_frame()).
 */
void screen_open(void) NGE_BANKED;

/**
 * @brief Run one paused frame: wait for the blanking period, show the
 *        buffer of the screen on top, and read the keyboard. Nothing in
 *        the game moves.
 */
void screen_frame(void) NGE_BANKED;

/** @brief Run paused frames (screen_frame()) until a key event arrives, and return it. */
nge_key_event_t screen_wait_key(void) NGE_BANKED;

/**
 * @brief Close the overlay screen on top, after it has closed its panels'
 *        streams. If it was the last, the main screen is shown again from
 *        the next blanking period, as it was left; waiting key events are
 *        thrown away.
 */
void screen_close(void) NGE_BANKED;

#endif
