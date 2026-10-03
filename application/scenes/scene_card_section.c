/*
 * MOBIB — Card section detail scene.
 *
 * Reads which section was picked from `scene_card`'s scene state, asks
 * `mobib_format_section` to fill the shared text buffer, and hands it
 * to a TextBox for scrollable display.
 */

#include "../mobib_app.h"
#include "../ui/mobib_format.h"

void mobib_scene_card_section_on_enter(void* context) {
    MobibApp* app = context;

    const uint32_t state = scene_manager_get_scene_state(app->scene_manager, MobibSceneCard);
    const MobibSection section = (MobibSection)state;

    text_box_reset(app->text_box);
    mobib_format_section(section, app->dump_valid ? &app->dump : NULL, app->text_buffer);
    text_box_set_text(app->text_box, furi_string_get_cstr(app->text_buffer));
    text_box_set_font(app->text_box, TextBoxFontText);

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewTextBox);
}

bool mobib_scene_card_section_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void mobib_scene_card_section_on_exit(void* context) {
    MobibApp* app = context;
    text_box_reset(app->text_box);
}
