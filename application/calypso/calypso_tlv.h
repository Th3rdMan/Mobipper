/*
 * MOBIB — minimal BER-TLV walker.
 *
 * Just enough of ISO 7816-4 / X.690 to traverse the FCI returned by
 * Calypso cards. Supports 1- and 2-byte tags and short-form lengths
 * (single byte, < 128) plus long-form lengths up to two byte payload.
 * That covers every TLV a real Calypso card emits in practice.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t tag; /**< Tag, packed as (b0<<8)|b1 for two-byte tags. */
    size_t length; /**< Number of bytes pointed to by `value`.        */
    const uint8_t* value; /**< Borrowed pointer into the original buffer.    */
} CalypsoTlv;

/**
 * @brief Parse one TLV starting at `*cursor`, advancing the cursor past it.
 *
 * @return true on success, false if the buffer is malformed or truncated.
 */
bool calypso_tlv_next(const uint8_t** cursor, const uint8_t* end, CalypsoTlv* out);

/**
 * @brief Depth-first search for a tag.
 *
 * Recursively descends into constructed TLVs (tag class bit 0x20 set).
 * Returns the first match.
 */
bool calypso_tlv_find(const uint8_t* buf, size_t len, uint32_t tag, CalypsoTlv* out);
