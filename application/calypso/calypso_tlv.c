/*
 * MOBIB — minimal BER-TLV walker. Contract documented in `calypso_tlv.h`.
 */

#include "calypso_tlv.h"

static bool tlv_read_tag(const uint8_t** p, const uint8_t* end, uint32_t* tag) {
    if(*p >= end) return false;
    uint8_t first = *(*p)++;
    *tag = first;
    if((first & 0x1F) == 0x1F) {
        /* Multi-byte tag. We support up to one continuation byte, which is
         * sufficient for every Calypso/Intercode tag observed in the wild. */
        if(*p >= end) return false;
        const uint8_t second = *(*p)++;
        *tag = ((uint32_t)first << 8) | second;
        if(second & 0x80) return false; /* refuse 3+ byte tags */
    }
    return true;
}

static bool tlv_read_length(const uint8_t** p, const uint8_t* end, size_t* len) {
    if(*p >= end) return false;
    const uint8_t first = *(*p)++;
    if((first & 0x80) == 0) {
        *len = first;
        return true;
    }
    const uint8_t n = first & 0x7F;
    if(n == 0 || n > 2) return false; /* indefinite or > 65535: refuse */
    if(*p + n > end) return false;
    size_t v = 0;
    for(uint8_t i = 0; i < n; ++i) v = (v << 8) | *(*p)++;
    *len = v;
    return true;
}

bool calypso_tlv_next(const uint8_t** cursor, const uint8_t* end, CalypsoTlv* out) {
    if(!cursor || !*cursor || !end || *cursor > end) return false;

    /* Skip any 00 / FF padding bytes between TLVs (allowed by ISO 7816-4). */
    while(*cursor < end && (**cursor == 0x00 || **cursor == 0xFF)) (*cursor)++;
    if(*cursor >= end) return false;

    const uint8_t* p = *cursor;
    uint32_t tag = 0;
    size_t   len = 0;
    if(!tlv_read_tag(&p, end, &tag)) return false;
    if(!tlv_read_length(&p, end, &len)) return false;
    if(p + len > end) return false;

    out->tag    = tag;
    out->length = len;
    out->value  = p;

    *cursor = p + len;
    return true;
}

static bool tlv_is_constructed(uint32_t tag) {
    /* The "constructed" bit lives in the first tag byte, bit 0x20. */
    const uint8_t first = (tag > 0xFF) ? (uint8_t)(tag >> 8) : (uint8_t)tag;
    return (first & 0x20) != 0;
}

bool calypso_tlv_find(
    const uint8_t* buf,
    size_t         len,
    uint32_t       tag,
    CalypsoTlv*    out) {
    if(!buf || !out) return false;
    const uint8_t* cur = buf;
    const uint8_t* end = buf + len;

    while(cur < end) {
        CalypsoTlv tlv;
        if(!calypso_tlv_next(&cur, end, &tlv)) return false;
        if(tlv.tag == tag) {
            *out = tlv;
            return true;
        }
        if(tlv_is_constructed(tlv.tag)) {
            if(calypso_tlv_find(tlv.value, tlv.length, tag, out)) return true;
        }
    }
    return false;
}
