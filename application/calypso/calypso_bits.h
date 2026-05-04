/*
 * MOBIB — MSB-first bit-stream reader.
 *
 * Calypso / Intercode records pack fields at arbitrary bit offsets in
 * the MSB-first orientation: the first byte's most-significant bit is
 * bit 0 of the stream. This helper centralises that arithmetic so the
 * decoders read like the spec.
 *
 * Reads up to 32 bits at a time. Out-of-range reads return 0 and clear
 * the `ok` flag, leaving the cursor untouched so callers can decide
 * whether to bail out or continue.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const uint8_t* data;       /**< Borrowed buffer.                       */
    size_t         total_bits; /**< Total length of `data` in bits.        */
    size_t         pos;        /**< Current bit offset (next bit to read). */
    bool           ok;         /**< Cleared on any out-of-range read.      */
} CalypsoBits;

static inline void calypso_bits_init(CalypsoBits* b, const uint8_t* data, size_t bytes) {
    b->data       = data;
    b->total_bits = bytes * 8;
    b->pos        = 0;
    b->ok         = true;
}

static inline uint32_t calypso_bits_read(CalypsoBits* b, size_t n) {
    if(n == 0 || n > 32) return 0;
    if(b->pos + n > b->total_bits) {
        b->ok = false;
        return 0;
    }
    uint32_t v = 0;
    for(size_t i = 0; i < n; ++i) {
        const size_t bit_index = b->pos + i;
        const uint8_t byte = b->data[bit_index >> 3];
        const uint8_t bit  = (byte >> (7 - (bit_index & 7))) & 0x1;
        v = (v << 1) | bit;
    }
    b->pos += n;
    return v;
}

static inline void calypso_bits_skip(CalypsoBits* b, size_t n) {
    if(b->pos + n > b->total_bits) {
        b->ok = false;
        return;
    }
    b->pos += n;
}
