/*
 * MOBIB — NFC reader wrapper.
 *
 * Owns an `Nfc` instance and an `NfcPoller` configured for ISO 14443-3B
 * (Type B), the transport layer underneath every Calypso/MOBIB card.
 * The wrapper hides the asynchronous poller callback behind a simple
 * "detected / removed" callback that is safe to consume from a scene.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct MobibNfc MobibNfc;

/** Snapshot of the activation data harvested from a Type-B card. */
typedef struct {
    /* Pseudo-Unique PICC Identifier — 4 bytes for ISO 14443-3B. */
    uint8_t  pupi[4];
    size_t   pupi_len;

    /* 4-byte Application Data field returned in ATQB. */
    uint8_t  app_data[4];

    /* Capabilities derived from the ATQB protocol-info byte. */
    bool     supports_iso14443_4;
    uint16_t frame_size_max;
    uint32_t fwt_fc_max;
} MobibCardInfo;

typedef enum {
    MobibNfcEventDetected, /**< A Type-B card was activated; `info` is valid. */
    MobibNfcEventError,    /**< Activation failed; `info` is NULL.            */
} MobibNfcEvent;

/**
 * @brief Callback invoked from the NFC worker thread.
 *
 * The callback runs on the NFC thread, **never** on the GUI thread.
 * It must not block and must not call into view code directly. Typical
 * pattern: copy `info` somewhere safe and post a custom view-dispatcher
 * event, then handle the UI update on the GUI side.
 */
typedef void (*MobibNfcCallback)(
    MobibNfcEvent       event,
    const MobibCardInfo* info,
    void*                context);

MobibNfc* mobib_nfc_alloc(void);
void      mobib_nfc_free(MobibNfc* instance);

/** Start polling. The callback may fire multiple times; the wrapper stops
 *  the poller automatically after the first successful detection. */
void mobib_nfc_start(MobibNfc* instance, MobibNfcCallback callback, void* context);

/** Stop polling. Idempotent; safe to call from any thread. */
void mobib_nfc_stop(MobibNfc* instance);
