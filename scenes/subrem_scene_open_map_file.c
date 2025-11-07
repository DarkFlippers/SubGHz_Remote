#include "../subghz_remote_app_i.h"

enum {
    SubRemSceneOpenMapFileCustomEventLoaded,
    SubRemSceneOpenMapFileCustomEventBack,
};

void subrem_scene_open_map_file_on_enter(void* context) {
    furi_assert(context);
    SubGhzRemoteApp* app = context;

    SubRemLoadMapState load_state = subrem_load_from_file(app);

    // Safety check: don't send events during destruction
    if(app->is_destroying) {
        return;
    }

    // Don't do scene transitions in on_enter! Use custom events instead
    if(load_state == SubRemLoadMapStateBack) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, SubRemSceneOpenMapFileCustomEventBack);
    } else if(load_state == SubRemLoadMapStateOK || load_state == SubRemLoadMapStateNotAllOK) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, SubRemSceneOpenMapFileCustomEventLoaded);
    } else {
        // Error loading - go back
        view_dispatcher_send_custom_event(
            app->view_dispatcher, SubRemSceneOpenMapFileCustomEventBack);
    }
}

bool subrem_scene_open_map_file_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    SubGhzRemoteApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint32_t start_scene_state =
            scene_manager_get_scene_state(app->scene_manager, SubRemSceneStart);

        if(event.event == SubRemSceneOpenMapFileCustomEventBack) {
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        } else if(event.event == SubRemSceneOpenMapFileCustomEventLoaded) {
            if(start_scene_state == SubmenuIndexSubRemEditMapFile) {
                scene_manager_set_scene_state(
                    app->scene_manager, SubRemSceneEditMenu, SubRemSubKeyNameUp);
                scene_manager_next_scene(app->scene_manager, SubRemSceneEditMenu);
            } else if(start_scene_state == SubmenuIndexSubRemOpenMapFile) {
                scene_manager_next_scene(app->scene_manager, SubRemSceneRemote);
            }
            consumed = true;
        }
    }

    return consumed;
}

void subrem_scene_open_map_file_on_exit(void* context) {
    UNUSED(context);
}
