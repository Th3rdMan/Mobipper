/*
 * MOBIB — Extended holder file decoder.
 *
 * Layout ported from metrodroid `MobibTransitData.kt extHolderFields`
 * (GPL-3.0, Google 2018). Two `HOLDER_EXTENDED` records (file 0x3F1C)
 * are concatenated, then parsed as:
 *
 *   EXT_HOLDER_UNKNOWN_A    18  bits
 *   EXT_HOLDER_CARD_SERIAL  76  bits  (BCD)
 *   EXT_HOLDER_UNKNOWN_B    16  bits
 *   EXT_HOLDER_UNKNOWN_C    58  bits
 *   EXT_HOLDER_DATE_OF_BIRTH 32 bits  (BCD YYYYMMDD)
 *   EXT_HOLDER_GENDER        2  bits  (0=anonymous, 1=male, 2=female)
 *   EXT_HOLDER_UNKNOWN_D     3  bits
 *   EXT_HOLDER_NAME        259  bits  (~52 5-bit chars, A=1..Z=26, 0/31=space)
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CALYPSO_HOLDER_NAME_MAX 64

typedef struct {
    bool     valid;

    /* 0=anonymous, 1=male, 2=female. */
    uint8_t  gender;

    /* Birth date, BCD packed YYYYMMDD. */
    uint8_t  birth_year_top2;
    uint8_t  birth_year_bot2;
    uint8_t  birth_month;
    uint8_t  birth_day;

    /* Trimmed, NUL-terminated holder name. */
    char     name[CALYPSO_HOLDER_NAME_MAX];
    size_t   name_len;
} CalypsoHolder;

/** Returns "Male", "Female", or NULL. */
const char* calypso_holder_gender_name(uint8_t gender);

/** Returns "Mr", "Mrs", or NULL. */
const char* calypso_holder_gender_title(uint8_t gender);

/**
 * @brief Decode a concatenated HOLDER_EXTENDED record pair.
 *
 * @param[in]  buf   record1 bytes followed by record2 bytes.
 * @param[in]  len   total length of `buf` in bytes.
 * @param[out] out   parsed holder; `out->valid` reflects success.
 * @return true iff the field stream had enough bits to decode at least
 *         the gender field.
 */
bool calypso_holder_parse(const uint8_t* buf, size_t len, CalypsoHolder* out);
