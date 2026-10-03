/*
 * MOBIB — Delete confirmation scene.
 *
 * Asks before removing the dump file behind the card currently shown.
 * Uses the app's own DialogEx view (not the shared Dialogs service).
 * On confirm, returns to the saved cards list, or to the main menu when
 * the card came from a fresh scan.
 */

#include "../mobib_app.h"
#include "../storage/mobib_storage.h"

#include <storage/storage.h>
#include <toolbox/path.h>

static void mobib_scene_delete_dialog_cb(DialogExResult result, void* context) {
    MobibApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, result);
}

void mobib_scene_delete_on_enter(void* context) {
    MobibApp* app = context;
    DialogEx* dialog = app->dialog_ex;

    /* text_buffer is free here (no TextBox shown) and outlives the view. */
    path_extract_filename(app->dump_path, app->text_buffer, true);

    dialog_ex_set_header(dialog, "Supprimer ?", 64, 4, AlignCenter, AlignTop);
    dialog_ex_set_text(
        dialog, furi_string_get_cstr(app->text_buffer), 64, 30, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(dialog, "Annuler");
    dialog_ex_set_right_button_text(dialog, "Supprimer");
    dialog_ex_set_result_callback(dialog, mobib_scene_delete_dialog_cb);
    dialog_ex_set_context(dialog, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewDialogEx);
}

bool mobib_scene_delete_on_event(void* context, SceneManagerEvent event) {
    MobibApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == DialogExResultRight) {
        Storage* storage = furi_record_open(RECORD_STORAGE);
        const bool removed = storage_simply_remove(storage, furi_string_get_cstr(app->dump_path));
        furi_record_close(RECORD_STORAGE);

        if(removed) {
            furi_string_reset(app->dump_path);
            app->dump_valid = false;
            scene_manager_set_scene_state(app->scene_manager, MobibSceneCard, 0);
            if(!scene_manager_search_and_switch_to_previous_scene(
                   app->scene_manager, MobibSceneDumps)) {
                scene_manager_search_and_switch_to_previous_scene(
                    app->scene_manager, MobibSceneStart);
            }
            return true;
        }
    }

    /* Cancel (or failed removal): back to the card menu. */
    scene_manager_previous_scene(app->scene_manager);
    return true;
}

void mobib_scene_delete_on_exit(void* context) {
    MobibApp* app = context;
    dialog_ex_reset(app->dialog_ex);
    furi_string_reset(app->text_buffer);
}
