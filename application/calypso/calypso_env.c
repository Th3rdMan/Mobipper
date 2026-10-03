/*
 * MOBIB — Intercode v2 Environment decoder.
 */

#include "calypso_env.h"
#include "calypso_bits.h"
#include "calypso_date.h"

#include <string.h>

/* ------------------------------ tables ----------------------------- */

/* The 12-bit country field is BCD-encoded: the four nibbles spell out
 * the ISO 3166-1 numeric code as a string. Belgium "056" lives in 12
 * bits as 0x056 — i.e. when read MSB-first as an integer it equals
 * decimal 86, NOT decimal 56. We therefore compare against the raw
 * value (0x0NN), not the ISO numeric. */
static const struct {
    uint16_t raw; /**< 12-bit raw value, identical to BCD digits. */
    const char* name;
} kCountries[] = {
    {0x056, "Belgique"},
    {0x250, "France"},
    {0x528, "Pays-Bas"},
    {0x380, "Italie"},
    {0, NULL},
};

static const struct {
    uint16_t country; /**< raw 12-bit value from kCountries. */
    uint16_t network; /**< raw 12-bit value. */
    const char* name;
} kNetworks[] = {
    /* Full 24-bit NetworkId 0x056001 — see metrodroid MobibTransitData
     * MOBIB_NETWORK_ID. Used by STIB, De Lijn, TEC and SNCB alike. */
    {0x056, 0x001, "MOBIB"},
    {0, 0, NULL},
};

static const char* lookup_country(uint16_t code) {
    for(size_t i = 0; kCountries[i].name; ++i) {
        if(kCountries[i].raw == code) return kCountries[i].name;
    }
    return NULL;
}

static const char* lookup_network(uint16_t country, uint16_t network) {
    for(size_t i = 0; kNetworks[i].name; ++i) {
        if(kNetworks[i].country == country && kNetworks[i].network == network) {
            return kNetworks[i].name;
        }
    }
    return NULL;
}

/* ------------------------------ public ----------------------------- */

bool calypso_env_parse(const uint8_t* rec, size_t rec_len, CalypsoEnvironment* out) {
    if(!rec || !out) return false;
    memset(out, 0, sizeof(*out));

    /* MOBIB Environment is at least 56 bits up to the validity-end field. */
    if(rec_len < 7) return false;

    CalypsoBits b;
    calypso_bits_init(&b, rec, rec_len);

    out->version = (uint8_t)calypso_bits_read(&b, 6);
    calypso_bits_skip(&b, 7); /* ENV_UNKNOWN_A — purpose unclear, see metrodroid. */

    /* NetworkId is 24 bits packed as (country 12 | network 12). */
    const uint32_t net = calypso_bits_read(&b, 24);
    out->country_code = (uint16_t)((net >> 12) & 0xFFF);
    out->network_id = (uint16_t)(net & 0xFFF);

    /* ENV_UNKNOWN_B is 5 bits for v>=3 and 9 bits for v<=2. */
    calypso_bits_skip(&b, out->version >= 3 ? 5 : 9);

    /* IssuerId is not present in the MOBIB layout at this offset; leave 0. */
    out->issuer_id = 0;

    out->validity_end_days = (uint16_t)calypso_bits_read(&b, 14);

    /* ENV_UNKNOWN_C: 6 bits for v<=2, 10 bits for v>=3. */
    calypso_bits_skip(&b, out->version >= 3 ? 10 : 6);

    /* HOLDER_BIRTH_DATE: 32 bits, BCD packed as YYYYMMDD. */
    out->birth_year_top2 = (uint8_t)calypso_bits_read(&b, 8);
    out->birth_year_bot2 = (uint8_t)calypso_bits_read(&b, 8);
    out->birth_month_bcd = (uint8_t)calypso_bits_read(&b, 8);
    out->birth_day_bcd = (uint8_t)calypso_bits_read(&b, 8);

    /* ENV_CARD_SERIAL: 76 bits BCD — skip for now, the FCI carries the
     * canonical PUPI-derived application serial number anyway. */
    calypso_bits_skip(&b, 76);

    /* ENV_UNKNOWN_D: 5 bits. */
    calypso_bits_skip(&b, 5);

    /* HOLDER_INT_POSTAL_CODE: 14 bits. */
    out->holder_postal_code = (uint16_t)calypso_bits_read(&b, 14);

    if(!b.ok) {
        /* Validity-end fields decoded earlier are still useful, so don't
         * abort. We just leave the holder fields as zero when the record
         * is short. */
    }

    /* An unwritten Calypso field is all-zeros, which would correspond to
     * 1997-01-01. Treat that as "not set" rather than a real expiry. */
    if(out->validity_end_days != 0) {
        calypso_date_from_days(
            out->validity_end_days,
            &out->validity_end_year,
            &out->validity_end_month,
            &out->validity_end_day);
    }

    out->country_name = lookup_country(out->country_code);
    out->network_name = lookup_network(out->country_code, out->network_id);

    out->valid = true;
    return true;
}
