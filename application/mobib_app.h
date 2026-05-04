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
#include <gui/modules/text_box.h>
#include <notification/notification.h>

#include "scenes/scenes.h"
#include "nfc/mobib_nfc.h"

/** Identifiers of every persistent view registered on the dispatcher. */
typedef enum {
    MobibViewSubmenu,
    MobibViewWidget,
    MobibViewTextBox,
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
    TextBox*           text_box;

    /* Scenes that need NFC borrow this; allocated lazily. */
    MobibNfc*          nfc;

    /* Latest captured (or loaded) dump shared by scene_card and friends.
     * Owned by the app for the duration of the navigation chain that
     * follows a scan or an open-from-disk action. */
    MobibDump          dump;
    bool               dump_valid;
    FuriString*        dump_path;   /**< Saved file path for the result screen. */

    /* Buffer used by scene_card_section to feed the TextBox. The TextBox
     * does not copy its input string so the buffer must outlive the view. */
    FuriString*        text_buffer;
} MobibApp;
