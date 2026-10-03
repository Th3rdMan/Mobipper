/*
 * MOBIB — STIB station / line lookup tables.
 *
 * Sourced from zoobab/mobib-extractor (`Database/Metro.csv`,
 * `Database/Bus.csv`), MIT-style "use freely" licence per the
 * upstream README. Compiled in only for the small Metro table; the
 * Bus table is shipped as an SD asset because it is too large for FAP
 * footprint.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t zone; /**< 6 bits, top of LOCATION_ID. */
    uint8_t subzone; /**< 4 bits.                      */
    uint8_t station; /**< 7 bits, bottom of LOCATION_ID. */
    const char* line; /**< Metro line label (e.g. "1A/1B"). */
    const char* name; /**< Station name. */
} CalypsoMetroStation;

/** Look up a metro station by the 17-bit LOCATION_ID, split into its
 *  zone / sub-zone / station fields. Returns NULL when no match. */
const CalypsoMetroStation*
    calypso_metro_station_lookup(uint8_t zone, uint8_t subzone, uint8_t station);

/** Convenience: split a 17-bit LOCATION_ID and look up. */
const CalypsoMetroStation* calypso_metro_station_lookup_id(uint32_t location_id);
