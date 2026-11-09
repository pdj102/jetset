#ifndef SCREEN_GUIDE_H
#define SCREEN_GUIDE_H

/**
 * @file screen_guide.h
 * @brief The help screen ("the guide", so it isn't confused with the help
 *        line, screen_help()): an overlay screen (game/ui/screen.h) of a
 *        few pages of fixed text, opened by ? in play. See
 *        docs/help_screen.md.
 *
 *     col 0                                  39
 *     row 0  |            "Commands"          1/4 |   title
 *         2  | the page's text: up to 29 rows      |   body
 *        30  |                                     |
 *        31  | help line: Left/right:page 1-4:go to SPACE:back |
 *
 * Left and right (or h and l) turn the page, wrapping round at the ends;
 * the digits go straight to a page. It only shows: no turn used.
 *
 * Banking: banked (NGE_BANKED, page 104: page 116 is full), with its text
 * in page 105. Don't call it while game code has its own page mapped in
 * slot 6 or 7.
 */

#include "core/nge_bank.h"

/** @brief Show the help screen at page 1 until SPACE. Uses no turn. */
void screen_guide_run(void) NGE_BANKED;

#endif
