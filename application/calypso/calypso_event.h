/*
 * MOBIB — Event log record decoder (transaction / journey).
 *
 * Layout ported from metrodroid's `MobibTransaction.kt` and
 * `En1545Bitmap.kt` (GPL-3.0, Copyright Google 2018, micolous &
 * contributors). MOBIB Event records live in the file with SFI 23 and
 * record one journey or validation each.
 *
 * Bit layout for protocol version 3 (the only version observed in the
 * wild on currently-issued MOBIB cards):
 *
 *   bits  0..  5   EVENT_VERSION                   6 bits
 *   bits  6.. 19   EVENT_DATE  (days since 1997)  14 bits
 *   bits 20.. 30   EVENT_TIME  (minutes/midnight) 11 bits
 *   bits 31.. 61   EVENT_UNKNOWN_B1               31 bits
 *
 *   bitmap1: 5 bits, LSB-first selects:
 *     0: container { unknown 4 + LOCATION_ID_BUS 12 }   (16 bits)
 *     1: ROUTE_NUMBER                                   (16 bits)
 *     2: NeverSeen2                                     (16 bits)
 *     3: NeverSeen3                                     (16 bits)
 *     4: container { SERVICE_PROVIDER 5 + LOCATION_ID 17 + UNKNOWN_E1 10 }
 *
 *   bitmap2: 6 bits, LSB-first selects:
 *     0: SERIAL_NUMBER                                  (24 bits)
 *     1: UNKNOWN_F                                      (16 bits)
 *     2: TransferNumber                                 ( 8 bits)
 *     3: NeverSeenA3                                    (16 bits)
 *     4: container { FIRST_STAMP_DATE 14 + FIRST_STAMP_TIME 11 } (25 bits)
 *     5: NeverSeenA5                                    (16 bits)
 *
 *   trailing 21 bits EVENT_UNKNOWN_G — ignored.
 *
 * The `flags` field on the parsed structure mirrors which optional
 * sub-fields were actually present in the bitmaps so callers do not
 * have to peek at the raw record themselves.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CALYPSO_EVENT_HAS_LOCATION_BUS  (1u << 0)
#define CALYPSO_EVENT_HAS_ROUTE         (1u << 1)
#define CALYPSO_EVENT_HAS_PROVIDER      (1u << 2)
#define CALYPSO_EVENT_HAS_SERIAL        (1u << 3)
#define CALYPSO_EVENT_HAS_FIRST_STAMP   (1u << 4)

/* Service provider codes used by MOBIB. The values match the constants
 * declared in metrodroid's MobibLookup. Higher values exist (metro,
 * train, ...) but no public reference enumerates them all, so callers
 * fall back to displaying the raw code for unknown providers. */
#define CALYPSO_PROVIDER_BUS  0x0Fu
#define CALYPSO_PROVIDER_TRAM 0x16u

typedef struct {
    bool     valid;
    uint32_t flags;

    uint8_t  version;

    /* Event timestamp — always present on v3 records. */
    uint16_t event_date_days;
    uint16_t event_year;
    uint8_t  event_month;
    uint8_t  event_day;
    uint16_t event_time_minutes;
    uint8_t  event_hour;
    uint8_t  event_minute;

    /* Optional fields, validity depends on `flags`. */
    uint16_t location_id_bus;
    uint16_t route_number;
    uint8_t  service_provider;
    uint32_t location_id;
    uint32_t serial_number;

    uint16_t first_stamp_year;
    uint8_t  first_stamp_month;
    uint8_t  first_stamp_day;
    uint8_t  first_stamp_hour;
    uint8_t  first_stamp_minute;
} CalypsoEvent;

/** Returns "Bus", "Tram" or NULL for unknown providers. */
const char* calypso_event_provider_name(uint8_t provider);

/** Returns true iff `rec` carries a non-empty event record (zero buffers
 *  are skipped, matching metrodroid's `isAllZero` shortcut). */
bool calypso_event_parse(const uint8_t* rec, size_t rec_len, CalypsoEvent* out);
