/*
 * MOBIB — Calypso command layer.
 *
 * Thin wrapper around `iso14443_4b_poller_send_block` that builds and
 * dispatches the small subset of ISO 7816-4 / Calypso APDUs the app
 * needs at this stage:
 *
 *   - SELECT BY NAME    (CLA=00 INS=A4 P1=04 P2=00)
 *   - READ RECORD       (CLA=00 INS=B2 P1=record P2=(SFI<<3)|0x04)
 *
 * All functions in this module **must** be called from inside an
 * `Iso14443_4bPoller` event callback — that is the only context in
 * which `iso14443_4b_poller_send_block` is legal.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declaration to keep the header free of nfc/ includes. */
typedef struct Iso14443_4bPoller Iso14443_4bPoller;

/** "1TIC.ICA" — the master Calypso application identifier. */
extern const uint8_t  CALYPSO_AID[8];
extern const size_t   CALYPSO_AID_LEN;

/** Common ISO 7816-4 status words encountered on Calypso cards. */
#define CALYPSO_SW_OK            0x9000u
#define CALYPSO_SW_RECORD_NOFOUND 0x6A83u
#define CALYPSO_SW_FILE_NOFOUND   0x6A82u

typedef struct CalypsoCtx CalypsoCtx;

CalypsoCtx* calypso_ctx_alloc(void);
void        calypso_ctx_free(CalypsoCtx* ctx);

/** Bind the context to an active 4B poller. The pointer is borrowed; the
 *  caller retains ownership and the binding is only valid for the
 *  duration of the poller callback that produced `poller`. */
void calypso_ctx_bind(CalypsoCtx* ctx, Iso14443_4bPoller* poller);

/**
 * @brief Generic APDU exchange.
 *
 * @param[in]  apdu          serialised case-2 / case-4 APDU
 * @param[in]  apdu_len      number of bytes in `apdu`
 * @param[out] response      buffer for the response payload (may be NULL if cap=0)
 * @param[in]  response_cap  size of `response` in bytes
 * @param[out] response_len  number of payload bytes written (may be NULL)
 * @param[out] sw            ISO 7816 status word from the response (may be NULL)
 * @return true on transport success (the SW is valid even if it is not 9000),
 *         false if the underlying block exchange failed.
 */
bool calypso_apdu(
    CalypsoCtx*    ctx,
    const uint8_t* apdu,
    size_t         apdu_len,
    uint8_t*       response,
    size_t         response_cap,
    size_t*        response_len,
    uint16_t*      sw);

/** SELECT BY NAME. Returns true and copies the FCI iff SW == 9000. */
bool calypso_select_aid(
    CalypsoCtx*    ctx,
    const uint8_t* aid,
    size_t         aid_len,
    uint8_t*       fci,
    size_t         fci_cap,
    size_t*        fci_len);

/** READ RECORD by SFI. Returns true and copies the record iff SW == 9000. */
bool calypso_read_record(
    CalypsoCtx* ctx,
    uint8_t     sfi,
    uint8_t     record,
    uint8_t*    data,
    size_t      data_cap,
    size_t*     data_len);
