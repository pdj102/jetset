#ifndef SCREEN_SPELL_H
#define SCREEN_SPELL_H

/**
 * @file screen_spell.h
 * @brief The spell list: an overlay screen (game/ui/screen.h) to choose a
 *        spell the player knows, to cast. See docs/spells.md, section 4.8.
 *
 *     col 0                                  39
 *     row 0  |        "Cast a Spell"           |   title
 *         2  |    Spell           Level  MP    |
 *         3  | a) Magic Bolt          1   1    |   a row a spell known
 *        26  | MP 5/5                          |
 *        29  | why a spell can't be cast       |   status
 *        31  | help line                       |
 *
 * A spell that can't be cast now is red: choosing it says why on the
 * status line (messages_act_failed()), in the words the message window
 * would use, and the screen stays open. Asked in check mode on the
 * player's own tile, so a bolt asks only about the spell and its cost.
 *
 * The book screen (screen_book_run()) is the same screen in a second mode
 * (docs/spells.md, 10.5.5): a book's spells, each coloured by ACT_LEARN
 * asked in check mode (white: can learn; cyan: known; red: can't, and
 * choosing it says why), the free slots at the foot. A white row chosen is
 * learned there and then, and the screen stays open for more.
 *
 * Banking: banked (NGE_BANKED, page 104, beside the PC screen). Don't call
 * it while game code has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "ecs/nge_entity.h"

/**
 * @brief Choose a spell the player knows, to cast: 1 with it in *sp, 0
 *        for back. With none known, says so and opens nothing.
 */
uint8_t screen_spell_run(uint8_t *sp) NGE_BANKED;

/**
 * @brief Read book (a spell book the player carries, ACT_LEARN's book check
 *        passed) and learn from it until SPACE: 1 if a spell was learned,
 *        so the reading uses the turn (D22), 0 if not.
 */
uint8_t screen_book_run(nge_entity_id_t book) NGE_BANKED;

#endif
