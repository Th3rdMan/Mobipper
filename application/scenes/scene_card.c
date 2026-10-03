/*
 * MOBIB — Card overview scene.
 *
 * Shown after a successful scan or when opening a saved dump. Presents
 * a submenu of categorised sections; selecting one descends into
 * `scene_card_section` which renders the formatted text in a TextBox.
 * When the card is backed by a file on SD, a last entry opens
 * `scene_delete` to remove it.
 */

#include "../mobib_app.h"
#include "../ui/mobib_format.h"
#include "../storage/mobib_storage.h"

#define CARD_ITEM_DELETE ((uint32_t)MobibSectionCount)

static bool mobib_scene_card_has_file(MobibApp* app) {
    return furi_string_start_with_str(app->dump_path, MOBIB_DUMP_DIR "/");
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
        app->submenu, scene_manager_get_scene_state(app->scene_manager, MobibSceneCard));

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewSubmenu);
}

bool mobib_scene_card_on_event(void* context, SceneManagerEvent event) {
    MobibApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == CARD_ITEM_DELETE) {
        scene_manager_set_scene_state(app->scene_manager, MobibSceneCard, event.event);
        scene_manager_next_scene(app->scene_manager, MobibSceneDelete);
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
