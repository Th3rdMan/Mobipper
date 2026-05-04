/*
 * MOBIB — Calypso command layer. Contract documented in `calypso.h`.
 */

#include "calypso.h"

#include <furi.h>
#include <toolbox/bit_buffer.h>
#include <nfc/protocols/iso14443_4b/iso14443_4b_poller.h>

/* "1TIC.ICA" in ASCII — the master Calypso AID. */
const uint8_t CALYPSO_AID[8] = {0x31, 0x54, 0x49, 0x43, 0x2E, 0x49, 0x43, 0x41};
const size_t  CALYPSO_AID_LEN = sizeof(CALYPSO_AID);

/* APDU buffer sizing: a Calypso command never exceeds CLA INS P1 P2 Lc + 256B + Le. */
#define CALYPSO_BUF_CAP 261

struct CalypsoCtx {
    Iso14443_4bPoller* poller;
    BitBuffer*         tx;
    BitBuffer*         rx;
};

CalypsoCtx* calypso_ctx_alloc(void) {
    CalypsoCtx* ctx = malloc(sizeof(CalypsoCtx));
    if(!ctx) return NULL;
    ctx->poller = NULL;
    ctx->tx     = bit_buffer_alloc(CALYPSO_BUF_CAP);
    ctx->rx     = bit_buffer_alloc(CALYPSO_BUF_CAP);
    return ctx;
}

void calypso_ctx_free(CalypsoCtx* ctx) {
    if(!ctx) return;
    bit_buffer_free(ctx->tx);
    bit_buffer_free(ctx->rx);
    free(ctx);
}

void calypso_ctx_bind(CalypsoCtx* ctx, Iso14443_4bPoller* poller) {
    furi_assert(ctx);
    ctx->poller = poller;
}

bool calypso_apdu(
    CalypsoCtx*    ctx,
    const uint8_t* apdu,
    size_t         apdu_len,
    uint8_t*       response,
    size_t         response_cap,
    size_t*        response_len,
    uint16_t*      sw) {
    furi_assert(ctx);
    furi_assert(ctx->poller);
    furi_assert(apdu);

    bit_buffer_reset(ctx->tx);
    bit_buffer_append_bytes(ctx->tx, apdu, apdu_len);
    bit_buffer_reset(ctx->rx);

    Iso14443_4bError err = iso14443_4b_poller_send_block(ctx->poller, ctx->tx, ctx->rx);
    if(err != Iso14443_4bErrorNone) return false;

    const size_t n = bit_buffer_get_size_bytes(ctx->rx);
    if(n < 2) return false; /* malformed: need at least SW1 SW2 */

    const uint8_t* data = bit_buffer_get_data(ctx->rx);
    if(sw) *sw = ((uint16_t)data[n - 2] << 8) | data[n - 1];

    const size_t payload = n - 2;
    const size_t copy    = payload < response_cap ? payload : response_cap;
    if(response && copy) memcpy(response, data, copy);
    if(response_len) *response_len = copy;

    return true;
}

bool calypso_select_aid(
    CalypsoCtx*    ctx,
    const uint8_t* aid,
    size_t         aid_len,
    uint8_t*       fci,
    size_t         fci_cap,
    size_t*        fci_len) {
    if(aid_len == 0 || aid_len > 16) return false;

    /* SELECT BY NAME, request FCI: 00 A4 04 00 Lc <AID> Le=00 */
    uint8_t apdu[6 + 16];
    apdu[0] = 0x00;
    apdu[1] = 0xA4;
    apdu[2] = 0x04;
    apdu[3] = 0x00;
    apdu[4] = (uint8_t)aid_len;
    memcpy(&apdu[5], aid, aid_len);
    apdu[5 + aid_len] = 0x00;

    uint16_t sw      = 0;
    size_t   rxlen   = 0;
    const bool ok    = calypso_apdu(ctx, apdu, 6 + aid_len, fci, fci_cap, &rxlen, &sw);
    if(fci_len) *fci_len = ok ? rxlen : 0;
    return ok && sw == CALYPSO_SW_OK;
}

bool calypso_read_record(
    CalypsoCtx* ctx,
    uint8_t     sfi,
    uint8_t     record,
    uint8_t*    data,
    size_t      data_cap,
    size_t*     data_len) {
    /* READ RECORD: 00 B2 <record> <(SFI<<3)|0x04> Le=00 */
    const uint8_t apdu[5] = {
        0x00,
        0xB2,
        record,
        (uint8_t)((sfi << 3) | 0x04),
        0x00,
    };

    uint16_t sw    = 0;
    size_t   rxlen = 0;
    const bool ok  = calypso_apdu(ctx, apdu, sizeof(apdu), data, data_cap, &rxlen, &sw);
    if(data_len) *data_len = ok ? rxlen : 0;
    return ok && sw == CALYPSO_SW_OK;
}

bool calypso_select_file_id(CalypsoCtx* ctx, uint16_t file_id) {
    /* SELECT FILE: 00 A4 08 00 02 <hi> <lo> Le=00
     * P1=08 selects by path from MF; P2=00 returns FCI as response. */
    const uint8_t apdu[7] = {
        0x00, 0xA4, 0x08, 0x00,
        0x02,
        (uint8_t)(file_id >> 8),
        (uint8_t)(file_id & 0xFF),
    };

    uint16_t sw = 0;
    const bool ok = calypso_apdu(ctx, apdu, sizeof(apdu), NULL, 0, NULL, &sw);
    return ok && sw == CALYPSO_SW_OK;
}

bool calypso_read_record_current(
    CalypsoCtx* ctx,
    uint8_t     record,
    uint8_t*    data,
    size_t      data_cap,
    size_t*     data_len) {
    /* READ RECORD: 00 B2 <record> 04 00 — current EF. */
    const uint8_t apdu[5] = {0x00, 0xB2, record, 0x04, 0x00};
    uint16_t sw    = 0;
    size_t   rxlen = 0;
    const bool ok  = calypso_apdu(ctx, apdu, sizeof(apdu), data, data_cap, &rxlen, &sw);
    if(data_len) *data_len = ok ? rxlen : 0;
    return ok && sw == CALYPSO_SW_OK;
}
