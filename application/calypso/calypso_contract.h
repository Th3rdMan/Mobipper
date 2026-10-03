/*
 * MOBIB — Contract / subscription decoder.
 *
 * Layout ported from metrodroid `MobibSubscription.kt` (GPL-3.0, Google
 * 2018). MOBIB stores contracts in the file with SFI 9; this decoder
 * supports both the legacy v<=3 layout and the modern bitmap-based
 * v>=4 layout that all currently-issued cards use.
 *
 * v<=3 layout:
 *   ContractVersion        6
 *   CONTRACT_UNKNOWN_B    21
 *   CONTRACT_TARIFF       14
 *   CONTRACT_SALE         14   (date, days since 1997-01-01)
 *   CONTRACT_UNKNOWN_C    48
 *   CONTRACT_PRICE_AMOUNT 16
 *   CONTRACT_UNKNOWN_D   113
 *
 * v>=4 layout (CURRENT):
 *   ContractVersion        6
 *   CONTRACT_UNKNOWN_A    19
 *   CONTRACT_TARIFF       14
 *   CONTRACT_UNKNOWN_B    50
 *   CONTRACT_PRICE_AMOUNT 16
 *   CONTRACT_UNKNOWN_C     6
 *   bitmap (5 bits, LSB-first):
 *     0: NeverSeen0          5
 *     1: NeverSeen1          5
 *     2: CONTRACT_SALE      14   (date)
 *     3: container { DurationUnits 2 + CONTRACT_DURATION 8 }
 *     4: NeverSeen4          8
 *   CONTRACT_UNKNOWN_D    24
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CALYPSO_CONTRACT_HAS_SALE     (1u << 0)
#define CALYPSO_CONTRACT_HAS_DURATION (1u << 1)
#define CALYPSO_CONTRACT_HAS_PRICE    (1u << 2)

/* Tariff codes recognised by metrodroid's MobibLookup. The value is the
 * raw 14-bit field; we expose it so the UI can pretty-print known ones
 * and fall back to hex for everything else. */
#define CALYPSO_TARIFF_JUMP_1_TRIP          0x2801u
#define CALYPSO_TARIFF_JUMP_10_TRIPS        0x2803u
#define CALYPSO_TARIFF_AIRPORT_BUS          0x0805u
#define CALYPSO_TARIFF_JUMP_24H_BUS_AIRPORT 0x303Du

typedef struct {
    bool valid;
    uint32_t flags;

    uint8_t version;
    uint16_t tariff; /**< 14-bit tariff code; 0 if absent. */
    uint16_t price_amount; /**< Price field; 0 if absent. */

    /* Sale date (days since 1997-01-01) and Y/M/D breakout. */
    uint16_t sale_days;
    uint16_t sale_year;
    uint8_t sale_month;
    uint8_t sale_day;

    uint8_t duration_units; /**< 0=days, 1=weeks, 2=months (per metrodroid). */
    uint8_t duration; /**< Duration in `duration_units`. */
} CalypsoContract;

/** Returns a static human label or NULL for unknown tariffs. */
const char* calypso_contract_tariff_name(uint16_t tariff);

/** Decode a single Contract record. Returns false for empty records and
 *  unsupported versions. */
bool calypso_contract_parse(const uint8_t* rec, size_t rec_len, CalypsoContract* out);
