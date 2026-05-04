/*
 * MOBIB — NFC reader wrapper. See `mobib_nfc.h` for the contract.
 */

#include "mobib_nfc.h"
#include "../calypso/calypso.h"

#include <furi.h>
#include <nfc/nfc.h>
#include <nfc/nfc_poller.h>
#include <nfc/protocols/nfc_protocol.h>
#include <nfc/protocols/iso14443_3b/iso14443_3b.h>
#include <nfc/protocols/iso14443_4b/iso14443_4b.h>
#include <nfc/protocols/iso14443_4b/iso14443_4b_poller.h>

#define TAG "MobibNfc"

/* SFIs we attempt to read on every card. The list is intentionally broad
 * — Calypso cards ignore unknown SFIs with SW=6A82/6A83, so a brute walk
 * is cheap and gives us the union of every variant out there. The
 * Belgian MOBIB family is documented to use a subset of these. */
static const uint8_t MOBIB_SFI_PROBE[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
};

/* Records to try per SFI. Calypso EFs are typically 1..N records; we cap
 * here to keep the dump bounded and fast. The walk stops on the first
 * non-9000 SW so EFs shorter than this only cost one extra APDU. */
#define MOBIB_RECORDS_PER_SFI 16

struct MobibNfc {
    Nfc*             nfc;
    NfcPoller*       poller;
    MobibNfcCallback callback;
    void*            context;
    bool             running;
};

/* ------------------------------------------------------------ helpers */

static void mobib_nfc_fill_card_info(const Iso14443_3bData* data, MobibCardInfo* out) {
    memset(out, 0, sizeof(*out));

    size_t uid_len = 0;
    const uint8_t* uid = iso14443_3b_get_uid(data, &uid_len);
    if(uid && uid_len > 0 && uid_len <= sizeof(out->pupi)) {
        memcpy(out->pupi, uid, uid_len);
        out->pupi_len = uid_len;
    }

    size_t app_len = 0;
    const uint8_t* app = iso14443_3b_get_application_data(data, &app_len);
    if(app && app_len > 0) {
        const size_t n = app_len < sizeof(out->app_data) ? app_len : sizeof(out->app_data);
        memcpy(out->app_data, app, n);
    }

    out->supports_iso14443_4 = iso14443_3b_supports_iso14443_4(data);
    out->frame_size_max      = iso14443_3b_get_frame_size_max(data);
    out->fwt_fc_max          = iso14443_3b_get_fwt_fc_max(data);
}

static void mobib_nfc_run_calypso_dump(Iso14443_4bPoller* poller, MobibDump* dump) {
    CalypsoCtx* ctx = calypso_ctx_alloc();
    if(!ctx) return;
    calypso_ctx_bind(ctx, poller);

    dump->fci_len = 0;
    dump->calypso_selected = calypso_select_aid(
        ctx, CALYPSO_AID, CALYPSO_AID_LEN, dump->fci, sizeof(dump->fci), &dump->fci_len);

    if(dump->calypso_selected) {
        for(size_t i = 0; i < sizeof(MOBIB_SFI_PROBE); ++i) {
            const uint8_t sfi = MOBIB_SFI_PROBE[i];
            for(uint8_t rec = 1; rec <= MOBIB_RECORDS_PER_SFI; ++rec) {
                if(dump->record_count >= MOBIB_DUMP_RECORD_MAX) break;

                MobibRecord* slot = &dump->records[dump->record_count];
                size_t       len  = 0;
                if(!calypso_read_record(
                       ctx, sfi, rec, slot->data, sizeof(slot->data), &len)) {
                    /* SW != 9000: file/record absent. Move to next SFI on
                     * the very first record; otherwise the EF exists but
                     * we've walked past its last record. */
                    if(rec == 1) break;
                    break;
                }
                slot->sfi    = sfi;
                slot->record = rec;
                slot->len    = (uint8_t)len;
                dump->record_count++;
            }
            if(dump->record_count >= MOBIB_DUMP_RECORD_MAX) break;
        }

        /* HOLDER_EXTENDED is selected by file ID 0x3F1C, not by SFI. We
         * try after the SFI walk so failures here don't poison the rest
         * of the dump. The file is two records of ~29 bytes each. */
        if(calypso_select_file_id(ctx, 0x3F1C)) {
            for(uint8_t rec = 1; rec <= MOBIB_HOLDER_EXT_RECS; ++rec) {
                size_t len = 0;
                uint8_t* buf = dump->holder_ext[rec - 1];
                if(calypso_read_record_current(
                       ctx, rec, buf, MOBIB_HOLDER_EXT_REC_SZ, &len)) {
                    dump->holder_ext_len[rec - 1] = (uint8_t)len;
                    dump->holder_ext_present = true;
                }
            }
        }
    }

    calypso_ctx_free(ctx);
}

/* Static dump buffer — too large for the poller-thread stack. The wrapper
 * is single-shot per `mobib_nfc_start` so contention is impossible. */
static MobibDump s_dump;

/* ----------------------------------------------------------- callback */

static NfcCommand mobib_nfc_poller_cb(NfcGenericEvent event, void* context) {
    MobibNfc* self = context;
    furi_assert(self);
    furi_assert(event.protocol == NfcProtocolIso14443_4b);

    const Iso14443_4bPollerEvent* evt = event.event_data;

    if(evt->type == Iso14443_4bPollerEventTypeReady) {
        Iso14443_4bPoller*     poller_4b = event.instance;
        const Iso14443_4bData* data_4b   = nfc_poller_get_data(self->poller);
        const Iso14443_3bData* data_3b   = iso14443_4b_get_base_data(data_4b);

        memset(&s_dump, 0, sizeof(s_dump));
        mobib_nfc_fill_card_info(data_3b, &s_dump.card);
        mobib_nfc_run_calypso_dump(poller_4b, &s_dump);

        if(self->callback) self->callback(MobibNfcEventDumped, &s_dump, self->context);
        return NfcCommandStop;
    }

    /* Activation error — keep polling so the user can retry by tapping again. */
    return NfcCommandContinue;
}

/* ------------------------------------------------------------ public */

MobibNfc* mobib_nfc_alloc(void) {
    MobibNfc* self = malloc(sizeof(MobibNfc));
    if(!self) return NULL;
    memset(self, 0, sizeof(*self));
    self->nfc = nfc_alloc();
    return self;
}

void mobib_nfc_free(MobibNfc* self) {
    if(!self) return;
    mobib_nfc_stop(self);
    if(self->nfc) nfc_free(self->nfc);
    free(self);
}

void mobib_nfc_start(MobibNfc* self, MobibNfcCallback callback, void* context) {
    furi_assert(self);
    if(self->running) return;

    self->callback = callback;
    self->context  = context;

    self->poller = nfc_poller_alloc(self->nfc, NfcProtocolIso14443_4b);
    nfc_poller_start(self->poller, mobib_nfc_poller_cb, self);
    self->running = true;
}

void mobib_nfc_stop(MobibNfc* self) {
    if(!self || !self->running) return;
    nfc_poller_stop(self->poller);
    nfc_poller_free(self->poller);
    self->poller  = NULL;
    self->running = false;
}
