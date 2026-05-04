/*
 * MOBIB — scene registry. Generated from `scenes_config.h` via x-macros.
 */
#pragma once

#include <gui/scene_manager.h>

typedef enum {
#define ADD_SCENE(prefix, name, id) MobibScene##id,
#include "scenes_config.h"
#undef ADD_SCENE
    MobibSceneCount,
} MobibScene;

extern const SceneManagerHandlers mobib_scene_handlers;

/* Forward declarations of every scene's three handlers. */
#define ADD_SCENE(prefix, name, id)                                            \
    void prefix##_scene_##name##_on_enter(void* context);                      \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent e); \
    void prefix##_scene_##name##_on_exit(void* context);
#include "scenes_config.h"
#undef ADD_SCENE
