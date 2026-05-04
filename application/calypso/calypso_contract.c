/*
 * MOBIB — Contract / subscription decoder. See `calypso_contract.h`.
 */

#include "calypso_contract.h"
#include "calypso_bits.h"
#include "calypso_date.h"

#include <string.h>

static bool buffer_is_all_zero(const uint8_t* buf, size_t len) {
    for(size_t i = 0; i < len; ++i) if(buf[i] != 0) return false;
    return true;
}

const char* calypso_contract_tariff_name(uint16_t tariff) {
    switch(tariff) {
    case CALYPSO_TARIFF_JUMP_1_TRIP:          return "Jump 1 trip";
    case CALYPSO_TARIFF_JUMP_10_TRIPS:        return "Jump 10 trips";
    case CALYPSO_TARIFF_AIRPORT_BUS:          return "Airport bus";
    case CALYPSO_TARIFF_JUMP_24H_BUS_AIRPORT: return "Jump 24h + airport";
    default:                                  return NULL;
    }
}

static bool parse_v_le3(CalypsoBits* b, CalypsoContract* out) {
    calypso_bits_skip(b, 21);                        /* UNKNOWN_B */
    out->tariff       = (uint16_t)calypso_bits_read(b, 14);
    out->sale_days    = (uint16_t)calypso_bits_read(b, 14);
    calypso_bits_skip(b, 48);                        /* UNKNOWN_C */
    out->price_amount = (uint16_t)calypso_bits_read(b, 16);
    /* Trailing 113 bits of UNKNOWN_D ignored. */
    if(out->sale_days)    out->flags |= CALYPSO_CONTRACT_HAS_SALE;
    if(out->price_amount) out->flags |= CALYPSO_CONTRACT_HAS_PRICE;
    return b->ok;
}

static bool parse_v_ge4(CalypsoBits* b, CalypsoContract* out) {
    calypso_bits_skip(b, 19);                        /* UNKNOWN_A */
    out->tariff = (uint16_t)calypso_bits_read(b, 14);
    calypso_bits_skip(b, 50);                        /* UNKNOWN_B */
    out->price_amount = (uint16_t)calypso_bits_read(b, 16);
    calypso_bits_skip(b, 6);                         /* UNKNOWN_C */

    const uint32_t bm = calypso_bits_read(b, 5);

    if(bm & 0x01) calypso_bits_skip(b, 5);           /* NeverSeen0 */
    if(bm & 0x02) calypso_bits_skip(b, 5);           /* NeverSeen1 */
    if(bm & 0x04) {
        out->sale_days = (uint16_t)calypso_bits_read(b, 14);
        if(out->sale_days) out->flags |= CALYPSO_CONTRACT_HAS_SALE;
    }
    if(bm & 0x08) {
        out->duration_units = (uint8_t)calypso_bits_read(b, 2);
        out->duration       = (uint8_t)calypso_bits_read(b, 8);
        out->flags |= CALYPSO_CONTRACT_HAS_DURATION;
    }
    if(bm & 0x10) calypso_bits_skip(b, 8);           /* NeverSeen4 */

    if(out->price_amount) out->flags |= CALYPSO_CONTRACT_HAS_PRICE;
    /* Trailing 24 bits of UNKNOWN_D ignored. */
    return b->ok;
}

bool calypso_contract_parse(const uint8_t* rec, size_t rec_len, CalypsoContract* out) {
    if(!rec || !out) return false;
    memset(out, 0, sizeof(*out));
    if(buffer_is_all_zero(rec, rec_len)) return false;

    CalypsoBits b;
    calypso_bits_init(&b, rec, rec_len);

    out->version = (uint8_t)calypso_bits_read(&b, 6);

    bool ok;
    if(out->version <= 3) {
        ok = parse_v_le3(&b, out);
    } else {
        ok = parse_v_ge4(&b, out);
    }
    if(!ok) return false;

    if(out->sale_days) {
        calypso_date_from_days(
            out->sale_days, &out->sale_year, &out->sale_month, &out->sale_day);
    }

    out->valid = true;
    return true;
}
