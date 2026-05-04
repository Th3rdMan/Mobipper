/*
 * MOBIB — Saved cards scene.
 *
 * Pops the system file browser scoped to `/ext/apps_data/mobib/dumps`
 * filtered to `.mobibdump` extensions. On selection, loads the dump
 * via `mobib_storage_load_dump` into the shared app state and pushes
 * `scene_card`. On cancel, pops back to start.
 */

#include "../mobib_app.h"
#include "../storage/mobib_storage.h"

#include <dialogs/dialogs.h>

void mobib_scene_dumps_on_enter(void* context) {
    MobibApp* app = context;

    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);

    DialogsFileBrowserOptions opts;
    dialog_file_browser_set_basic_options(&opts, MOBIB_DUMP_EXT, NULL);
    opts.base_path = MOBIB_DUMP_DIR;

    FuriString* selected = furi_string_alloc();
    FuriString* preselect = furi_string_alloc_set(MOBIB_DUMP_DIR);

    const bool picked = dialog_file_browser_show(dialogs, selected, preselect, &opts);
    furi_string_free(preselect);
    furi_record_close(RECORD_DIALOGS);

    if(picked) {
        if(mobib_storage_load_dump(furi_string_get_cstr(selected), &app->dump)) {
            app->dump_valid = true;
            furi_string_set(app->dump_path, selected);
            furi_string_free(selected);
            scene_manager_next_scene(app->scene_manager, MobibSceneCard);
            return;
        }
    }
    furi_string_free(selected);

    /* Either cancelled or load failed — drop back to start. */
    scene_manager_previous_scene(app->scene_manager);
}

bool mobib_scene_dumps_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void mobib_scene_dumps_on_exit(void* context) {
    UNUSED(context);
}
