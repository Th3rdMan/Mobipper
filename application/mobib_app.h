/*
 * MOBIB — Flipper Zero application
 * Copyright (C) 2026  i12bp8
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <notification/notification.h>

#include "scenes/scenes.h"

/** Identifiers of every persistent view registered on the dispatcher. */
typedef enum {
    MobibViewSubmenu,
    MobibViewWidget,
} MobibView;

/** Global application state. Allocated once in `mobib_app_main`. */
typedef struct {
    Gui*               gui;
    NotificationApp*   notifications;
    ViewDispatcher*    view_dispatcher;
    SceneManager*      scene_manager;

    /* Reusable view modules — one of each is enough for the whole app. */
    Submenu*           submenu;
    Widget*            widget;
} MobibApp;
