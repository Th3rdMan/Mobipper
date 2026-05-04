/*
 * MOBIB — Calypso FCI decoder.
 *
 * Wraps `calypso_tlv` with knowledge of the specific tag layout returned
 * by a Calypso card after a successful SELECT BY NAME:
 *
 *   6F  FCI Template
 *     84  DF Name           — AID + 6-byte issuer/version extension
 *     A5  Proprietary FCI
 *       BF 0C  Issuer discretionary
 *         C7  Application Serial Number (8 bytes)
 *         53  Discretionary data
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CALYPSO_DF_NAME_MAX        16
#define CALYPSO_APP_SERIAL_MAX     8
#define CALYPSO_AID_EXTENSION_LEN  6

typedef struct {
    bool    valid;

    /* Full DF Name as returned in tag 84 (AID + extension). */
    uint8_t df_name[CALYPSO_DF_NAME_MAX];
    size_t  df_name_len;

    /* True if the DF Name starts with the Calypso AID `1TIC.ICA`. */
    bool    is_calypso_aid;

    /* The 6-byte tail that follows the AID. For the MOBIB family this
     * begins with `D0 56` (Calypso Networks Association marker). */
    uint8_t aid_extension[CALYPSO_AID_EXTENSION_LEN];
    bool    has_aid_extension;
    bool    is_mobib_extension; /**< extension begins with D0 56 */

    /* Application Serial Number from tag C7. The last four bytes equal
     * the card's PUPI on every MOBIB observed so far. */
    uint8_t app_serial[CALYPSO_APP_SERIAL_MAX];
    size_t  app_serial_len;
} CalypsoFci;

/**
 * @brief Parse a SELECT(AID) FCI response.
 *
 * @param[in]  fci      raw FCI bytes (starting with tag 6F).
 * @param[in]  fci_len  number of bytes in `fci`.
 * @param[out] out      parsed structure; `out->valid` reflects success.
 * @return true iff the FCI Template was located and decoded.
 */
bool calypso_fci_parse(const uint8_t* fci, size_t fci_len, CalypsoFci* out);
