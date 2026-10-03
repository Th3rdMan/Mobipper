/*
 * MOBIB — Saved cards scene.
 *
 * Shows the app's own FileBrowser view scoped to `/ext/apps_data/mobipper/dumps`
 * and filtered to `.mobibdump`. The system Dialogs service is avoided on
 * purpose: it serves one browser at a time, and when the Apps menu or Archive
 * holds it the app blocked forever on a blank screen.
 *
 * Selecting a file loads it into the shared app state and pushes
 * `scene_card`; Back in the dumps folder pops back to start.
 */

#include "../mobib_app.h"
#include "../storage/mobib_storage.h"

enum {
    DumpsEventSelected = 0x200,
};

static void mobib_scene_dumps_browser_cb(void* context) {
    MobibApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, DumpsEventSelected);
}

void mobib_scene_dumps_on_enter(void* context) {
    MobibApp* app = context;

    /* Preselect a file, not the folder: given a folder, the Momentum
     * browser parks the cursor on ".." and one OK leaves the dumps dir. */
    if(furi_string_start_with_str(app->dump_path, MOBIB_DUMP_DIR "/")) {
        furi_string_set(app->browser_path, app->dump_path);
    } else {
        mobib_storage_first_dump(app->browser_path);
    }

    file_browser_configure(
        app->file_browser, MOBIB_DUMP_EXT, MOBIB_DUMP_DIR, true, true, NULL, true);
    file_browser_set_callback(app->file_browser, mobib_scene_dumps_browser_cb, app);
    file_browser_start(app->file_browser, app->browser_path);

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewFileBrowser);
}

bool mobib_scene_dumps_on_event(void* context, SceneManagerEvent event) {
    MobibApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != DumpsEventSelected) {
        return false;
    }

    if(mobib_storage_load_dump(furi_string_get_cstr(app->browser_path), &app->dump)) {
        app->dump_valid = true;
        furi_string_set(app->dump_path, app->browser_path);
        scene_manager_set_scene_state(app->scene_manager, MobibSceneCard, 0);
        scene_manager_next_scene(app->scene_manager, MobibSceneCard);
    }
    return true;
}

void mobib_scene_dumps_on_exit(void* context) {
    MobibApp* app = context;
    file_browser_stop(app->file_browser);
}
