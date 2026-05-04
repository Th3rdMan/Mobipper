/*
 * MOBIB — Event log decoder. Contract documented in `calypso_event.h`.
 */

#include "calypso_event.h"
#include "calypso_bits.h"
#include "calypso_date.h"

#include <string.h>

/* EN 1545 bitmap convention (per metrodroid's En1545Bitmap.kt):
 *   - read N bits MSB-first (where N is the bitmap entry count).
 *   - the resulting integer is interpreted with bit 0 (LSB) selecting
 *     the first entry, bit 1 the second, and so on.
 * `calypso_bits_read` already returns the value with the most recently
 * read bit in the LSB position, so the integer can be used directly.   */
static uint32_t bitmap_read(CalypsoBits* b, size_t n) {
    return calypso_bits_read(b, n);
}

static bool buffer_is_all_zero(const uint8_t* buf, size_t len) {
    for(size_t i = 0; i < len; ++i) if(buf[i] != 0) return false;
    return true;
}

const char* calypso_event_provider_name(uint8_t provider) {
    switch(provider) {
    case CALYPSO_PROVIDER_METRO:    return "Metro";
    case CALYPSO_PROVIDER_PREMETRO: return "Premetro";
    case CALYPSO_PROVIDER_BUS:      return "Bus";
    case CALYPSO_PROVIDER_TRAM:     return "Tram";
    default:                        return NULL;
    }
}

bool calypso_event_parse(const uint8_t* rec, size_t rec_len, CalypsoEvent* out) {
    if(!rec || !out) return false;
    memset(out, 0, sizeof(*out));

    if(buffer_is_all_zero(rec, rec_len)) return false;

    CalypsoBits b;
    calypso_bits_init(&b, rec, rec_len);

    out->version = (uint8_t)calypso_bits_read(&b, 6);
    /* Only v3 layout implemented; bail out gracefully on older variants
     * so the caller can still display the raw bytes if it wants. */
    if(out->version < 3) return false;

    out->event_date_days     = (uint16_t)calypso_bits_read(&b, 14);
    out->event_time_minutes  = (uint16_t)calypso_bits_read(&b, 11);
    calypso_bits_skip(&b, 31); /* EVENT_UNKNOWN_B1 */

    /* ---------------------------- bitmap 1 ---------------------------- */
    const uint32_t bm1 = bitmap_read(&b, 5);

    if(bm1 & 0x01) {
        calypso_bits_skip(&b, 4);                               /* unknown */
        out->location_id_bus = (uint16_t)calypso_bits_read(&b, 12);
        out->flags |= CALYPSO_EVENT_HAS_LOCATION_BUS;
    }
    if(bm1 & 0x02) {
        out->route_number = (uint16_t)calypso_bits_read(&b, 16);
        out->flags |= CALYPSO_EVENT_HAS_ROUTE;
    }
    if(bm1 & 0x04) calypso_bits_skip(&b, 16); /* NeverSeen2 */
    if(bm1 & 0x08) calypso_bits_skip(&b, 16); /* NeverSeen3 */
    if(bm1 & 0x10) {
        out->service_provider = (uint8_t)calypso_bits_read(&b, 5);
        out->location_id      = calypso_bits_read(&b, 17);
        calypso_bits_skip(&b, 10); /* unknown E1 */
        out->flags |= CALYPSO_EVENT_HAS_PROVIDER;
    }

    /* ---------------------------- bitmap 2 ---------------------------- */
    const uint32_t bm2 = bitmap_read(&b, 6);

    if(bm2 & 0x01) {
        out->serial_number = calypso_bits_read(&b, 24);
        out->flags |= CALYPSO_EVENT_HAS_SERIAL;
    }
    if(bm2 & 0x02) calypso_bits_skip(&b, 16);
    if(bm2 & 0x04) calypso_bits_skip(&b, 8);
    if(bm2 & 0x08) calypso_bits_skip(&b, 16);
    if(bm2 & 0x10) {
        const uint16_t fs_days    = (uint16_t)calypso_bits_read(&b, 14);
        const uint16_t fs_minutes = (uint16_t)calypso_bits_read(&b, 11);
        if(fs_days != 0) {
            calypso_date_from_days(
                fs_days,
                &out->first_stamp_year,
                &out->first_stamp_month,
                &out->first_stamp_day);
            calypso_time_from_minutes(
                fs_minutes, &out->first_stamp_hour, &out->first_stamp_minute);
            out->flags |= CALYPSO_EVENT_HAS_FIRST_STAMP;
        }
    }
    if(bm2 & 0x20) calypso_bits_skip(&b, 16);

    if(!b.ok) return false;

    /* Convert the always-present timestamp. */
    if(out->event_date_days != 0) {
        calypso_date_from_days(
            out->event_date_days,
            &out->event_year,
            &out->event_month,
            &out->event_day);
    }
    calypso_time_from_minutes(
        out->event_time_minutes, &out->event_hour, &out->event_minute);

    out->valid = true;
    return true;
}
