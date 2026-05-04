/*
 * MOBIB — STIB bus stop table.
 *
 * Compiled-in lookup table of (line, stop_code) -> stop name. Sourced
 * from zoobab/mobib-extractor `Database/Bus.csv` (~2200 rows).
 *
 * Records are sorted by (line, code) so we can use binary search
 * (O(log n)) for each lookup; renders of the journeys section stay
 * snappy even with multiple events.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t    line;   /**< STIB bus line number (decimal). */
    uint16_t    code;   /**< Stop code = LOCATION_ID_BUS field. */
    const char* name;   /**< Stop name (UTF-8, ≤24 chars + NUL). */
} CalypsoBusStop;

/** Returns NULL if no match. */
const CalypsoBusStop* calypso_bus_stop_lookup(uint16_t line, uint16_t code);
