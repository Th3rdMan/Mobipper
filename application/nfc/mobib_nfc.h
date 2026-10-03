/*
 * MOBIB — NFC reader wrapper.
 *
 * Owns an `Nfc` instance and an `NfcPoller` configured for ISO 14443-4B,
 * which transparently performs ISO 14443-3B activation (PUPI/ATQB) and
 * the ATTRIB/RATS handshake required to send APDUs.
 *
 * On a successful activation the wrapper attempts to SELECT the Calypso
 * application (`1TIC.ICA`) and walk a curated list of SFIs, building a
 * `MobibDump` snapshot. The dump is delivered to the caller through a
 * single callback that runs on the NFC worker thread.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct MobibNfc MobibNfc;

/** Snapshot of the activation data harvested from a Type-B card. */
typedef struct {
    /* Pseudo-Unique PICC Identifier — 4 bytes for ISO 14443-3B. */
    uint8_t pupi[4];
    size_t pupi_len;

    /* 4-byte Application Data field returned in ATQB. */
    uint8_t app_data[4];

    /* Capabilities derived from the ATQB protocol-info byte. */
    bool supports_iso14443_4;
    uint16_t frame_size_max;
    uint32_t fwt_fc_max;
} MobibCardInfo;

/* Maximum number of records we attempt to collect per dump. Bounded by
 * stack budget more than by Calypso (real cards expose a few dozen). */
#define MOBIB_DUMP_RECORD_MAX 128
#define MOBIB_RECORD_DATA_CAP 32
#define MOBIB_FCI_CAP         64

typedef struct {
    uint8_t sfi; /**< Short File Identifier (1..31). */
    uint8_t record; /**< 1-based record index inside the EF. */
    uint8_t len; /**< Number of valid bytes in `data`. */
    uint8_t data[MOBIB_RECORD_DATA_CAP];
} MobibRecord;

/* HOLDER_EXTENDED at file ID 0x3F1C carries the human identity (name,
 * gender, birthdate). Two records of ~29 bytes each, concatenated. */
#define MOBIB_HOLDER_EXT_RECS   2
#define MOBIB_HOLDER_EXT_REC_SZ 32

/* Path-selected "extra" files probed after the SFI walk. Each slot
 * stores up to 64 bytes of the first record. See mobib_nfc.c for the
 * list of files probed. */
#define MOBIB_EXTRA_FILES    8
#define MOBIB_EXTRA_DATA_CAP 64

typedef struct {
    char label[16]; /**< short human label, e.g. "ICC" or "ID". */
    uint16_t file_id; /**< 16-bit path, 0 = empty slot.           */
    uint8_t len;
    uint8_t data[MOBIB_EXTRA_DATA_CAP];
} MobibExtraFile;

typedef struct MobibDump {
    MobibCardInfo card;
    bool calypso_selected; /**< AID 1TIC.ICA accepted. */
    uint8_t fci[MOBIB_FCI_CAP]; /**< File Control Information. */
    size_t fci_len;
    MobibRecord records[MOBIB_DUMP_RECORD_MAX];
    size_t record_count;

    /* Path-selected files. We only persist what fits a known schema; the
     * raw bytes go into the FlipperFormat dump under matching keys. */
    bool holder_ext_present;
    uint8_t holder_ext[MOBIB_HOLDER_EXT_RECS][MOBIB_HOLDER_EXT_REC_SZ];
    uint8_t holder_ext_len[MOBIB_HOLDER_EXT_RECS];

    /* Opportunistic dump of every file we could SELECT by path, so the
     * Deep scan section can surface them. Unused slots have file_id=0. */
    size_t extra_count;
    MobibExtraFile extras[MOBIB_EXTRA_FILES];

    /* Optional secondary applications we tried to SELECT by AID. */
    bool has_mpp; /**< Parking app (AID 3MTR.ICA / 1MPP). */
    bool has_rt2; /**< Transport v2 (AID 3TCW.ICA).       */
    bool has_eticket; /**< ETicket application.               */
} MobibDump;

typedef enum {
    MobibNfcEventDumped, /**< Card activated and dump attempted; `dump` is valid. */
    MobibNfcEventError, /**< Activation or transport failure; `dump` is NULL.    */
} MobibNfcEvent;

/**
 * @brief Callback invoked from the NFC worker thread.
 *
 * The callback runs on the NFC thread, **never** on the GUI thread.
 * It must not block and must not call into view code directly. Typical
 * pattern: copy `dump` somewhere safe and post a custom view-dispatcher
 * event, then handle the UI update on the GUI side.
 */
typedef void (*MobibNfcCallback)(MobibNfcEvent event, const MobibDump* dump, void* context);

MobibNfc* mobib_nfc_alloc(void);
void mobib_nfc_free(MobibNfc* instance);

/** Start polling. The callback may fire multiple times; the wrapper stops
 *  the poller automatically after the first successful detection. */
void mobib_nfc_start(MobibNfc* instance, MobibNfcCallback callback, void* context);

/** Stop polling. Idempotent; safe to call from any thread. */
void mobib_nfc_stop(MobibNfc* instance);
