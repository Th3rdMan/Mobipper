/*
 * MOBIB — Flipper Zero application
 * Copyright (C) 2026  i12bp8
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "mobib_app.h"
#include "storage/mobib_storage.h"

/* ------------------------------------------------------------ scene tables */

static void (*const mobib_scene_on_enter[])(void*) = {
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
#include "scenes/scenes_config.h"
#undef ADD_SCENE
};

static bool (*const mobib_scene_on_event[])(void*, SceneManagerEvent) = {
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
#include "scenes/scenes_config.h"
#undef ADD_SCENE
};

static void (*const mobib_scene_on_exit[])(void*) = {
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
#include "scenes/scenes_config.h"
#undef ADD_SCENE
};

const SceneManagerHandlers mobib_scene_handlers = {
    .on_enter_handlers = mobib_scene_on_enter,
    .on_event_handlers = mobib_scene_on_event,
    .on_exit_handlers = mobib_scene_on_exit,
    .scene_num = MobibSceneCount,
};

/* ------------------------------------------------------- dispatcher glue */

static bool mobib_back_event(void* context) {
    MobibApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static bool mobib_custom_event(void* context, uint32_t event) {
    MobibApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

/* ------------------------------------------------------- alloc / free */

static MobibApp* mobib_app_alloc(void) {
    MobibApp* app = malloc(sizeof(MobibApp));
    if(!app) return NULL;
    memset(app, 0, sizeof(*app));

    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&mobib_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, mobib_back_event);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, mobib_custom_event);

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, MobibViewSubmenu, submenu_get_view(app->submenu));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, MobibViewWidget, widget_get_view(app->widget));

    app->text_box = text_box_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, MobibViewTextBox, text_box_get_view(app->text_box));

    app->browser_path = furi_string_alloc();
    app->file_browser = file_browser_alloc(app->browser_path);
    view_dispatcher_add_view(
        app->view_dispatcher, MobibViewFileBrowser, file_browser_get_view(app->file_browser));

    app->dialog_ex = dialog_ex_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, MobibViewDialogEx, dialog_ex_get_view(app->dialog_ex));

    app->dump_path = furi_string_alloc();
    app->text_buffer = furi_string_alloc();

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    return app;
}

static void mobib_app_free(MobibApp* app) {
    furi_assert(app);

    view_dispatcher_remove_view(app->view_dispatcher, MobibViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, MobibViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, MobibViewTextBox);
    view_dispatcher_remove_view(app->view_dispatcher, MobibViewFileBrowser);
    view_dispatcher_remove_view(app->view_dispatcher, MobibViewDialogEx);

    submenu_free(app->submenu);
    widget_free(app->widget);
    text_box_free(app->text_box);
    file_browser_free(app->file_browser);
    furi_string_free(app->browser_path);
    dialog_ex_free(app->dialog_ex);

    furi_string_free(app->dump_path);
    furi_string_free(app->text_buffer);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    free(app);
}

/* --------------------------------------------------------------- entry */

int32_t mobib_app_main(void* p) {
    UNUSED(p);

    MobibApp* app = mobib_app_alloc();
    if(!app) return -1;

    mobib_storage_migrate_legacy();
    scene_manager_next_scene(app->scene_manager, MobibSceneStart);
    view_dispatcher_run(app->view_dispatcher);

    mobib_app_free(app);
    return 0;
}
