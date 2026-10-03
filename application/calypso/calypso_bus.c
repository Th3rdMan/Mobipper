/*
 * MOBIB — STIB bus stop table. See `calypso_bus.h`.
 *
 * Generated from the STIB-MIVB GTFS feed (data.belgianmobility.io, 2026-10-03,
 * CC BY 4.0): every (route_short_name, numeric stop_id) pair served by a
 * trip, with the French stop name. 97 % of the (line, code) pairs shared
 * with the original zoobab/mobib-extractor table carry the same name.
 * To regenerate: python tools/gtfs_stops.py <gtfs_dir> <old.inc> <out.inc>
 */

#include "calypso_bus.h"

#include <stddef.h>

static const CalypsoBusStop kStops[] = {
#include "calypso_bus_table.inc"
};

static const size_t kStopsCount = sizeof(kStops) / sizeof(kStops[0]);

const CalypsoBusStop* calypso_bus_stop_lookup(uint16_t line, uint16_t code) {
    /* Binary search over the (line, code) ascending ordering. */
    size_t lo = 0;
    size_t hi = kStopsCount;
    while(lo < hi) {
        const size_t mid = (lo + hi) / 2;
        const CalypsoBusStop* s = &kStops[mid];
        if(s->line < line || (s->line == line && s->code < code)) {
            lo = mid + 1;
        } else if(s->line > line || (s->line == line && s->code > code)) {
            hi = mid;
        } else {
            return s;
        }
    }
    return NULL;
}
