/*
 * MOBIB — NFC reader wrapper. See `mobib_nfc.h` for the contract.
 */

#include "mobib_nfc.h"

#include <furi.h>
#include <nfc/nfc.h>
#include <nfc/nfc_poller.h>
#include <nfc/protocols/nfc_protocol.h>
#include <nfc/protocols/iso14443_3b/iso14443_3b.h>
#include <nfc/protocols/iso14443_3b/iso14443_3b_poller.h>

struct MobibNfc {
    Nfc*             nfc;
    NfcPoller*       poller;
    MobibNfcCallback callback;
    void*            context;
    bool             running;
};

static void mobib_nfc_fill_info(const Iso14443_3bData* data, MobibCardInfo* out) {
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

static NfcCommand mobib_nfc_poller_cb(NfcGenericEvent event, void* context) {
    MobibNfc* self = context;
    furi_assert(self);
    furi_assert(event.protocol == NfcProtocolIso14443_3b);

    const Iso14443_3bPollerEvent* evt = event.event_data;

    if(evt->type == Iso14443_3bPollerEventTypeReady) {
        const Iso14443_3bData* data = nfc_poller_get_data(self->poller);
        MobibCardInfo info;
        mobib_nfc_fill_info(data, &info);

        if(self->callback) self->callback(MobibNfcEventDetected, &info, self->context);
        return NfcCommandStop;
    }

    /* Activation error — keep polling so the user can retry by tapping again. */
    return NfcCommandContinue;
}

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

    self->poller = nfc_poller_alloc(self->nfc, NfcProtocolIso14443_3b);
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
