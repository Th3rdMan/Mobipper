/*
 * MOBIB — Intercode v2 Environment decoder.
 */

#include "calypso_env.h"
#include "calypso_bits.h"
#include "calypso_date.h"

#include <string.h>

/* ------------------------------ tables ----------------------------- */

static const struct {
    uint16_t    code;
    const char* name;
} kCountries[] = {
    {56,  "Belgium"},
    {250, "France"},
    {528, "Netherlands"},
    {380, "Italy"},
    {0,   NULL},
};

static const struct {
    uint16_t    country;
    uint16_t    network;
    const char* name;
} kNetworks[] = {
    /* Belgium 056 / 0x001 — MOBIB common interop scheme used by STIB,
     * De Lijn, TEC and SNCB. Every Belgian MOBIB card shares this ID
     * (full 24-bit NetworkId 0x056001 — see metrodroid). */
    {56, 0x001, "MOBIB"},
    {0,  0,    NULL},
};

static const char* lookup_country(uint16_t code) {
    for(size_t i = 0; kCountries[i].name; ++i) {
        if(kCountries[i].code == code) return kCountries[i].name;
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
    out->network_id   = (uint16_t)(net & 0xFFF);

    /* ENV_UNKNOWN_B is 5 bits for v>=3 and 9 bits for v<=2. */
    calypso_bits_skip(&b, out->version >= 3 ? 5 : 9);

    /* IssuerId is not present in the MOBIB layout at this offset; leave 0. */
    out->issuer_id = 0;

    out->validity_end_days = (uint16_t)calypso_bits_read(&b, 14);
    if(!b.ok) return false;

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
