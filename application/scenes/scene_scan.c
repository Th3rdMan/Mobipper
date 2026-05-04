/*
 * MOBIB — Scan scene: poll for a card, run a Calypso dump and render
 * a scrollable summary of every record we managed to read.
 */

#include "../mobib_app.h"
#include "../nfc/mobib_nfc.h"
#include "../storage/mobib_storage.h"
#include "../calypso/calypso_fci.h"

#include <notification/notification_messages.h>

typedef enum {
    ScanCustomEventDumped = 0x100,
    ScanCustomEventError,
} ScanCustomEvent;

/* The poller callback runs on the NFC worker thread and hands us a
 * pointer that becomes invalid as soon as the wrapper is stopped, so we
 * stash a private copy here for the GUI thread to consume. The dump is
 * a few KB which is too large for the GUI stack. */
static MobibDump  s_dump;
static FuriString* s_saved_path; /**< Owned by the scene; lives between events. */

static void mobib_scan_nfc_cb(MobibNfcEvent event, const MobibDump* dump, void* ctx) {
    MobibApp* app = ctx;

    if(event == MobibNfcEventDumped && dump) {
        s_dump = *dump;
        view_dispatcher_send_custom_event(app->view_dispatcher, ScanCustomEventDumped);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, ScanCustomEventError);
    }
}

static void mobib_scan_render_waiting(MobibApp* app) {
    Widget* w = app->widget;
    widget_reset(w);
    widget_add_text_box_element(
        w, 0, 0, 128, 14, AlignCenter, AlignTop, "\e#Scanning…\e#", false);
    widget_add_string_multiline_element(
        w, 64, 36, AlignCenter, AlignCenter, FontSecondary,
        "Hold MOBIB card\nflat against the\nback of the Flipper");
}

static void mobib_scan_append_pupi(FuriString* body, const MobibCardInfo* c) {
    furi_string_cat_str(body, "PUPI ");
    for(size_t i = 0; i < c->pupi_len; ++i) {
        furi_string_cat_printf(body, "%02X", c->pupi[i]);
    }
    furi_string_cat_str(body, "\n");
}

static void mobib_scan_append_records(FuriString* body, const MobibDump* d) {
    for(size_t i = 0; i < d->record_count; ++i) {
        const MobibRecord* r = &d->records[i];
        furi_string_cat_printf(body, "SFI%02X r%u (%uB)\n", r->sfi, r->record, r->len);
        const size_t preview = r->len < 12 ? r->len : 12;
        for(size_t b = 0; b < preview; ++b) {
            furi_string_cat_printf(body, "%02X ", r->data[b]);
        }
        if(r->len > preview) furi_string_cat_str(body, "…");
        furi_string_cat_str(body, "\n");
    }
}

static void mobib_scan_append_fci(FuriString* body, const MobibDump* d) {
    if(!d->calypso_selected || d->fci_len == 0) {
        furi_string_cat_str(body, "AID: not selected\n");
        return;
    }

    CalypsoFci fci;
    if(!calypso_fci_parse(d->fci, d->fci_len, &fci) || !fci.valid) {
        furi_string_cat_str(body, "FCI: unparsed\n");
        return;
    }

    if(fci.is_calypso_aid) {
        furi_string_cat_str(body, "AID: 1TIC.ICA");
        if(fci.is_mobib_extension) furi_string_cat_str(body, " (MOBIB)");
        furi_string_cat_str(body, "\n");
    } else {
        furi_string_cat_str(body, "AID: ");
        for(size_t i = 0; i < fci.df_name_len; ++i) {
            furi_string_cat_printf(body, "%02X", fci.df_name[i]);
        }
        furi_string_cat_str(body, "\n");
    }

    if(fci.has_aid_extension) {
        furi_string_cat_str(body, "Ext: ");
        for(size_t i = 0; i < CALYPSO_AID_EXTENSION_LEN; ++i) {
            furi_string_cat_printf(body, "%02X ", fci.aid_extension[i]);
        }
        furi_string_cat_str(body, "\n");
    }

    if(fci.app_serial_len > 0) {
        furi_string_cat_str(body, "Serial ");
        for(size_t i = 0; i < fci.app_serial_len; ++i) {
            furi_string_cat_printf(body, "%02X", fci.app_serial[i]);
        }
        furi_string_cat_str(body, "\n");
    }
}

static void mobib_scan_render_dump(MobibApp* app, const MobibDump* d) {
    Widget* w = app->widget;
    widget_reset(w);

    bool is_mobib = false;
    if(d->calypso_selected && d->fci_len > 0) {
        CalypsoFci fci;
        if(calypso_fci_parse(d->fci, d->fci_len, &fci)) is_mobib = fci.is_mobib_extension;
    }
    const char* title = is_mobib ? "\e#MOBIB card\e#"
                       : (d->calypso_selected ? "\e#Calypso card\e#" : "\e#Type-B card\e#");
    widget_add_text_box_element(w, 0, 0, 128, 14, AlignCenter, AlignTop, title, false);

    FuriString* body = furi_string_alloc();

    mobib_scan_append_pupi(body, &d->card);
    mobib_scan_append_fci(body, d);
    furi_string_cat_printf(body, "Records: %u\n", (unsigned)d->record_count);

    if(s_saved_path && furi_string_size(s_saved_path) > 0) {
        const char* p = furi_string_get_cstr(s_saved_path);
        const char* leaf = strrchr(p, '/');
        furi_string_cat_printf(body, "Saved: %s\n", leaf ? leaf + 1 : p);
    }

    if(d->record_count > 0) {
        furi_string_cat_str(body, "\n");
        mobib_scan_append_records(body, d);
    }

    widget_add_text_scroll_element(w, 0, 16, 128, 48, furi_string_get_cstr(body));
    furi_string_free(body);
}

void mobib_scene_scan_on_enter(void* context) {
    MobibApp* app = context;

    if(!s_saved_path) s_saved_path = furi_string_alloc();
    furi_string_reset(s_saved_path);

    mobib_scan_render_waiting(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewWidget);

    notification_message(app->notifications, &sequence_blink_start_cyan);

    app->nfc = mobib_nfc_alloc();
    mobib_nfc_start(app->nfc, mobib_scan_nfc_cb, app);
}

bool mobib_scene_scan_on_event(void* context, SceneManagerEvent event) {
    MobibApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case ScanCustomEventDumped:
        mobib_nfc_stop(app->nfc);
        notification_message(app->notifications, &sequence_success);
        if(!mobib_storage_save_dump(&s_dump, s_saved_path)) {
            furi_string_set(s_saved_path, "<save failed>");
        }
        mobib_scan_render_dump(app, &s_dump);
        return true;
    case ScanCustomEventError:
        return true;
    default:
        return false;
    }
}

void mobib_scene_scan_on_exit(void* context) {
    MobibApp* app = context;

    if(app->nfc) {
        mobib_nfc_stop(app->nfc);
        mobib_nfc_free(app->nfc);
        app->nfc = NULL;
    }

    notification_message(app->notifications, &sequence_blink_stop);
    widget_reset(app->widget);
}
