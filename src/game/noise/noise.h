#ifndef NOISE_H
#define NOISE_H

/**
 * @file noise.h
 * @brief Noises (docs/noise.md): events make noises at a tile, by a table
 *        indexed by event, and each noise reaches the AI creatures in
 *        range (ai_hear(), game/ai/ai.h), which keep the loudest until
 *        their turn.
 *
 * A row of the table says how far the event's noise carries (its
 * loudness, in tiles; 0: silent), at which tile (the target's, the
 * source's, or (p1, p2) in the source's area), and whether it counts only
 * when the player is the event's source or target. A new noise is a new
 * row.
 *
 * Banking: noise_on_event() and the mask it tests are in common memory,
 * so a silent event costs one bit test and no banked call. The rest is
 * banked (NGE_BANKED, page 142, with the table): don't call it while game
 * code has its own page mapped in slot 6 or 7.
 */

#include <stdint.h>

#include "core/nge_bank.h"
#include "event/nge_event.h"
#include "game/events.h"

/* A row's flags */
#define NOISE_AT_SOURCE  0x01   /**< At the event's source's tile, not its target's. */
#define NOISE_AT_P1P2    0x02   /**< At (p1, p2) in the source's area (EV_AREA). */
#define NOISE_ANYONE     0x04   /**< Whoever made it; without it, only if the player is source or target. */
#define NOISE_STEP       0x08   /**< A step: only with a move in (p1, p2); loudness from noise_step_loudness(). */

/** The events the table covers, from NGE_EVENT_USER. */
#define NOISE_EVENTS  (EV_LOOKED - NGE_EVENT_USER + 1)

/** A row of the noise table, by event. */
typedef struct noise_row {
    uint8_t loud;    /**< How far it carries, in tiles (nge_map_within()); 0: silent. */
    uint8_t flags;   /**< NOISE_*. */
} noise_row_t;

/** One bit for each event from NGE_EVENT_USER: set if its row has a loudness (noise_init()). */
extern uint8_t noise_mask[(NOISE_EVENTS + 7) / 8];

/** @brief Build noise_mask from the table. Once, before the first event. */
void noise_init(void) NGE_BANKED;

/**
 * @brief Make the noise event e makes, if any: a cheap filter on the
 *        mask, then noise_event(). In common memory, for the dispatcher.
 */
void noise_on_event(const nge_event_t *e);

/**
 * @brief Make the noise event e makes by its row, if any, whatever the
 *        mask: the tile, the loudness, and ai_hear(). For noise_on_event()
 *        and the tests.
 * @param e In common memory.
 */
void noise_event(const nge_event_t *e) NGE_BANKED;

/** @brief The loudness of the player's step: the table's (stealth may change it later, docs/noise.md R12). */
uint8_t noise_step_loudness(void) NGE_BANKED;

#endif
