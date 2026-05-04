/*
 * MOBIB — About scene: short description and version string.
 */

#include "../mobib_app.h"

void mobib_scene_about_on_enter(void* context) {
    MobibApp* app = context;
    Widget*   w   = app->widget;

    widget_reset(w);
    widget_add_text_box_element(
        w, 0, 0, 128, 14, AlignCenter, AlignTop, "\e#MOBIB\e#", false);
    widget_add_text_scroll_element(
        w, 0, 16, 128, 48,
        "Reader for Belgian MOBIB transit cards (Calypso, ISO 14443-B).\n"
        "\n"
        "Version 0.1 — UI skeleton.\n"
        "GPL-3.0-or-later.\n"
        "\n"
        "Not affiliated with STIB/MIVB, SNCB/NMBS, De Lijn or TEC.");

    view_dispatcher_switch_to_view(app->view_dispatcher, MobibViewWidget);
}

bool mobib_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void mobib_scene_about_on_exit(void* context) {
    MobibApp* app = context;
    widget_reset(app->widget);
}
