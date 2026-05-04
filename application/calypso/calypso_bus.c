/*
 * MOBIB — STIB bus stop table. See `calypso_bus.h`.
 *
 * Auto-generated from zoobab/mobib-extractor Database/Bus.csv (2203 rows).
 * To regenerate run the python snippet in docs/MOBIB_NOTES.md.
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
