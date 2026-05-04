/*
 * MOBIB — Start scene: top-level menu shown when the app launches.
 */

#include "../mobib_app.h"

typedef enum {
    StartSubmenuRead,
    StartSubmenuAbout,
} StartSubmenuIndex;

static void mobib_scene_start_submenu_cb(void* context, uint32_t index) {
    MobibApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void mobib_scene_start_on_enter(void* context) {
    MobibApp* app = context;
    Submenu*  m   = app->submenu;

    submenu_reset(m);
    submenu_set_header(m, "MOBIB");
    submenu_add_item(m, "Read card",  StartSubmenuRead,  mobib_scene_start_submenu_cb, app);
    submenu_add_item(m, "About",      StartSubmenuAbout, mobib_scene_start_submenu_cb, app);

    submenu_set_selected_item(
        m, scene_manager_get_scene_state(app->scene_manager, MobibSceneStart));

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewSubmenu);
}

bool mobib_scene_start_on_event(void* context, SceneManagerEvent event) {
    MobibApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(app->scene_manager, MobibSceneStart, event.event);
        switch(event.event) {
        case StartSubmenuRead:
            scene_manager_next_scene(app->scene_manager, MobibSceneScan);
            return true;
        case StartSubmenuAbout:
            scene_manager_next_scene(app->scene_manager, MobibSceneAbout);
            return true;
        default:
            return false;
        }
    }
    return false;
}

void mobib_scene_start_on_exit(void* context) {
    MobibApp* app = context;
    submenu_reset(app->submenu);
}
