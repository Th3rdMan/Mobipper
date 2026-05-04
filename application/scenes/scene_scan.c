/*
 * MOBIB — Scan scene.
 *
 * Polls for a Type-B card, runs the Calypso dump, persists it to SD,
 * and on success advances to `scene_card` which presents a paginated
 * overview. The scene itself only owns the "scanning…" prompt and the
 * NFC instance.
 */

#include "../mobib_app.h"
#include "../nfc/mobib_nfc.h"
#include "../storage/mobib_storage.h"

#include <notification/notification_messages.h>

typedef enum {
    ScanCustomEventDumped = 0x100,
    ScanCustomEventError,
} ScanCustomEvent;

static void mobib_scan_nfc_cb(MobibNfcEvent event, const MobibDump* dump, void* ctx) {
    MobibApp* app = ctx;

    if(event == MobibNfcEventDumped && dump) {
        app->dump       = *dump;
        app->dump_valid = true;
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

void mobib_scene_scan_on_enter(void* context) {
    MobibApp* app = context;

    app->dump_valid = false;
    furi_string_reset(app->dump_path);

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

        /* Persist to SD; failure is non-fatal — the user can still browse. */
        if(!mobib_storage_save_dump(&app->dump, app->dump_path)) {
            furi_string_set(app->dump_path, "<save failed>");
        }

        /* Replace ourselves with the overview scene so back jumps to start. */
        scene_manager_next_scene(app->scene_manager, MobibSceneCard);
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
