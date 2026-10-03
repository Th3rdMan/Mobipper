/*
 * MOBIB — Card overview scene.
 *
 * Shown after a successful scan or when opening a saved dump. Presents
 * a submenu of categorised sections; selecting one descends into
 * `scene_card_section` which renders the formatted text in a TextBox.
 * When the card is backed by a file on SD, a last entry deletes it.
 */

#include "../mobib_app.h"
#include "../ui/mobib_format.h"
#include "../storage/mobib_storage.h"

#include <dialogs/dialogs.h>
#include <storage/storage.h>
#include <toolbox/path.h>

#define CARD_ITEM_DELETE ((uint32_t)MobibSectionCount)

static bool mobib_scene_card_has_file(MobibApp* app) {
    return furi_string_start_with_str(app->dump_path, MOBIB_DUMP_DIR "/");
}

/* Ask for confirmation, then remove the dump file. */
static bool mobib_scene_card_delete(MobibApp* app) {
    FuriString* name = furi_string_alloc();
    path_extract_filename(app->dump_path, name, true);

    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessage* msg = dialog_message_alloc();
    dialog_message_set_header(msg, "Supprimer ?", 64, 0, AlignCenter, AlignTop);
    dialog_message_set_text(
        msg, furi_string_get_cstr(name), 64, 32, AlignCenter, AlignCenter);
    dialog_message_set_buttons(msg, "Annuler", NULL, "Supprimer");
    const DialogMessageButton answer = dialog_message_show(dialogs, msg);
    dialog_message_free(msg);
    furi_record_close(RECORD_DIALOGS);

    furi_string_free(name);

    if(answer != DialogMessageButtonRight) return false;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    const bool removed = storage_simply_remove(storage, furi_string_get_cstr(app->dump_path));
    furi_record_close(RECORD_STORAGE);

    if(removed) {
        furi_string_reset(app->dump_path);
        app->dump_valid = false;
    }
    return removed;
}

void mobib_scene_card_on_enter(void* context) {
    MobibApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Carte MOBIB");

    for(uint32_t i = 0; i < (uint32_t)MobibSectionCount; ++i) {
        submenu_add_item(
            app->submenu,
            mobib_section_title((MobibSection)i),
            i,
            (void (*)(void*, uint32_t))view_dispatcher_send_custom_event,
            app->view_dispatcher);
    }
    if(mobib_scene_card_has_file(app)) {
        submenu_add_item(
            app->submenu,
            "Supprimer la sauvegarde",
            CARD_ITEM_DELETE,
            (void (*)(void*, uint32_t))view_dispatcher_send_custom_event,
            app->view_dispatcher);
    }

    submenu_set_selected_item(
        app->submenu,
        scene_manager_get_scene_state(app->scene_manager, MobibSceneCard));

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewSubmenu);
}

bool mobib_scene_card_on_event(void* context, SceneManagerEvent event) {
    MobibApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == CARD_ITEM_DELETE) {
        if(mobib_scene_card_delete(app)) {
            scene_manager_set_scene_state(app->scene_manager, MobibSceneCard, 0);
            /* Back to the file list when the card was opened from it,
             * otherwise (fresh scan) back to the main menu. */
            if(!scene_manager_search_and_switch_to_previous_scene(
                   app->scene_manager, MobibSceneDumps)) {
                scene_manager_search_and_switch_to_previous_scene(
                    app->scene_manager, MobibSceneStart);
            }
        }
        return true;
    }
    if(event.event >= (uint32_t)MobibSectionCount) return false;

    scene_manager_set_scene_state(app->scene_manager, MobibSceneCard, event.event);
    scene_manager_next_scene(app->scene_manager, MobibSceneCardSection);
    return true;
}

void mobib_scene_card_on_exit(void* context) {
    MobibApp* app = context;
    submenu_reset(app->submenu);
}
