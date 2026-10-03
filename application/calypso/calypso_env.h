/*
 * MOBIB — Environment record decoder (MOBIB profile).
 *
 * Layout ported from metrodroid's `MobibTransitData.kt` (GPL-3.0,
 * Copyright Google 2018, micolous & contributors). The MOBIB variant
 * differs from the generic EN 1545 Intercode layout by an additional
 * 7-bit `ENV_UNKNOWN_A` field between the version and the network id
 * and by a 5- or 9-bit `ENV_UNKNOWN_B` before the validity end date
 * (5 bits for v >= 3, 9 bits for v <= 2).
 *
 * Bit map (MSB-first):
 *   bits  0.. 5   ENV_VERSION_NUMBER             6 bits
 *   bits  6..12   ENV_UNKNOWN_A                  7 bits
 *   bits 13..36   ENV_NETWORK_ID                24 bits  (country 12 | network 12)
 *   bits 37..41   ENV_UNKNOWN_B (v>=3)           5 bits
 *           or    ENV_UNKNOWN_B (v<=2)           9 bits  (shifts later fields)
 *   bits 42..55   ENV_APPLICATION_VALIDITY_END  14 bits  (days since 1997-01-01)
 *
 * For known MOBIB cards the network id equals 0x056001
 * (country 0x056 = Belgium, network 0x001).
 * Anything past the validity date is profile-specific (holder birthdate,
 * card serial, postal code) and not parsed in this milestone.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool valid;

    uint8_t version;
    uint16_t country_code; /**< ISO 3166 numeric — top 12 bits of NetworkId. */
    uint16_t network_id; /**< Bottom 12 bits of NetworkId.                   */
    uint8_t issuer_id; /**< Always 0 in the MOBIB profile (no such field). */

    /* Days since 1997-01-01 (0 = 1997-01-01). */
    uint16_t validity_end_days;
    /* Same value broken out as Gregorian Y/M/D. Zero if computation fails. */
    uint16_t validity_end_year;
    uint8_t validity_end_month;
    uint8_t validity_end_day;

    /* Holder fields. Always present in the bit stream but typically zero
     * on anonymous MOBIB Basic cards. */
    uint8_t birth_year_top2; /**< First two BCD digits, e.g. 0x19 or 0x20.   */
    uint8_t birth_year_bot2; /**< Last two BCD digits.                       */
    uint8_t birth_month_bcd; /**< BCD-encoded month, 0x01..0x12.             */
    uint8_t birth_day_bcd; /**< BCD-encoded day, 0x01..0x31.               */
    uint16_t holder_postal_code;

    /* Best-effort human labels. NULL when unknown — never freed by the caller. */
    const char* country_name;
    const char* network_name;
} CalypsoEnvironment;

bool calypso_env_parse(const uint8_t* rec, size_t rec_len, CalypsoEnvironment* out);
