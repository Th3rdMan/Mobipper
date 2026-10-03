/*
 * MOBIB — Extended holder decoder. See `calypso_holder.h`.
 */

#include "calypso_holder.h"
#include "calypso_bits.h"

#include <string.h>

const char* calypso_holder_gender_name(uint8_t gender) {
    switch(gender) {
    case 1:
        return "Homme";
    case 2:
        return "Femme";
    default:
        return NULL;
    }
}

const char* calypso_holder_gender_title(uint8_t gender) {
    /* zoobab uses these directly from the 2 gender bits. */
    switch(gender) {
    case 1:
        return "M.";
    case 2:
        return "Mme";
    default:
        return NULL;
    }
}

/* Decode an EN1545 5-bit packed string. Per metrodroid's
 * En1545FixedString.parseString:
 *   - 5 bits per character.
 *   - 0 / 31 = space (skipped at the start, kept inside).
 *   - 1..26 = 'A'..'Z'.
 * The string is right-trimmed to its last non-space character. */
static void parse_5bit_string(
    CalypsoBits* b,
    size_t total_bits,
    char* out,
    size_t out_cap,
    size_t* out_len) {
    if(out_cap == 0) {
        *out_len = 0;
        return;
    }
    size_t written = 0;
    size_t last_non_space = 0;
    bool started = false;

    /* We read at most (total_bits / 5) characters; -1 to mirror metrodroid's
     * `i + 4 < start + length` guard which stops one character early. */
    /* Per zoobab/mobib-extractor `bin_to_alphabet`: values outside 1..26
     * are rendered as space. metrodroid is more lenient (only 0 and 31)
     * but in practice MOBIB cards use 27..30 as separators / padding. */
    const size_t max_chars = total_bits / 5;
    for(size_t c = 0; c < max_chars; ++c) {
        const uint32_t v = calypso_bits_read(b, 5);
        if(!b->ok) break;
        char ch;
        if(v < 1 || v > 26) {
            if(!started) continue; /* skip leading padding */
            ch = ' ';
        } else {
            ch = (char)('A' + (int)v - 1);
            started = true;
            last_non_space = written;
        }
        if(written + 1 < out_cap) out[written++] = ch;
    }
    /* Right-trim. */
    if(started) {
        out[last_non_space + 1 < out_cap ? last_non_space + 1 : out_cap - 1] = '\0';
        *out_len = last_non_space + 1;
    } else {
        out[0] = '\0';
        *out_len = 0;
    }
}

bool calypso_holder_parse(const uint8_t* buf, size_t len, CalypsoHolder* out) {
    if(!buf || !out) return false;
    memset(out, 0, sizeof(*out));
    if(len < 32) return false; /* need both records */

    CalypsoBits b;
    calypso_bits_init(&b, buf, len);

    calypso_bits_skip(&b, 18); /* UNKNOWN_A */
    calypso_bits_skip(&b, 76); /* CARD_SERIAL */
    calypso_bits_skip(&b, 16); /* UNKNOWN_B */
    calypso_bits_skip(&b, 58); /* UNKNOWN_C */

    out->birth_year_top2 = (uint8_t)calypso_bits_read(&b, 8);
    out->birth_year_bot2 = (uint8_t)calypso_bits_read(&b, 8);
    out->birth_month = (uint8_t)calypso_bits_read(&b, 8);
    out->birth_day = (uint8_t)calypso_bits_read(&b, 8);

    out->gender = (uint8_t)calypso_bits_read(&b, 2);
    calypso_bits_skip(&b, 3); /* UNKNOWN_D */

    if(!b.ok) return false;

    parse_5bit_string(&b, 259, out->name, sizeof(out->name), &out->name_len);

    out->valid = true;
    return true;
}
