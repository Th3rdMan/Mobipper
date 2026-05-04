/*
 * MOBIB — EN 1545 date / time helpers.
 *
 * Calypso uses two compact temporal encodings throughout its records:
 *
 *   - DATE: 14 bits, days since 1997-01-01 in proleptic civil days.
 *   - TIME: 11 bits, minutes since local midnight (0..1439).
 *
 * These helpers are intentionally tiny and allocation-free so they can
 * be called from inside a poller callback or a parser hot loop.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/** Convert "days since 1997-01-01" into Gregorian Y/M/D. Returns false
 *  on absurd inputs (year > 2100). */
bool calypso_date_from_days(
    uint16_t days,
    uint16_t* year,
    uint8_t*  month,
    uint8_t*  day);

/** Convert "minutes since midnight" into hour/minute. Clamps to 23:59. */
void calypso_time_from_minutes(
    uint16_t minutes,
    uint8_t* hour,
    uint8_t* minute);
