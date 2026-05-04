/*
 * MOBIB — Scan scene: poll for an ISO 14443-B card and display the
 * activation parameters. Stepping stone for milestone M2 (Calypso AID
 * select + record dump).
 */

#include "../mobib_app.h"
#include "../nfc/mobib_nfc.h"

#include <notification/notification_messages.h>

typedef enum {
    ScanCustomEventDetected = 0x100,
    ScanCustomEventError,
} ScanCustomEvent;

/* Worker thread → GUI thread bridge. The poller callback runs on the NFC
 * worker; we copy the activation snapshot into this static buffer and
 * post a custom event. The scene reads it from the GUI thread. */
static MobibCardInfo s_last_card;

static void mobib_scan_nfc_cb(MobibNfcEvent event, const MobibCardInfo* info, void* ctx) {
    MobibApp* app = ctx;

    if(event == MobibNfcEventDetected && info) {
        s_last_card = *info;
        view_dispatcher_send_custom_event(app->view_dispatcher, ScanCustomEventDetected);
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

static void mobib_scan_render_card(MobibApp* app, const MobibCardInfo* c) {
    Widget* w = app->widget;
    widget_reset(w);

    widget_add_text_box_element(
        w, 0, 0, 128, 14, AlignCenter, AlignTop, "\e#Card detected\e#", false);

    FuriString* body = furi_string_alloc();

    furi_string_cat_str(body, "PUPI: ");
    for(size_t i = 0; i < c->pupi_len; ++i) {
        furi_string_cat_printf(body, "%02X", c->pupi[i]);
    }
    furi_string_cat_str(body, "\n");

    furi_string_cat_printf(
        body, "AppData: %02X %02X %02X %02X\n",
        c->app_data[0], c->app_data[1], c->app_data[2], c->app_data[3]);

    furi_string_cat_printf(
        body, "ISO14443-4: %s\n", c->supports_iso14443_4 ? "yes" : "no");
    furi_string_cat_printf(body, "FSCImax: %u B", (unsigned)c->frame_size_max);

    widget_add_text_scroll_element(w, 0, 16, 128, 48, furi_string_get_cstr(body));
    furi_string_free(body);
}

void mobib_scene_scan_on_enter(void* context) {
    MobibApp* app = context;

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
    case ScanCustomEventDetected:
        mobib_nfc_stop(app->nfc);
        notification_message(app->notifications, &sequence_success);
        mobib_scan_render_card(app, &s_last_card);
        return true;
    case ScanCustomEventError:
        /* Activation hiccup — keep waiting silently. */
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
