cmake_minimum_required(VERSION 3.20)
project(pal4_angelscript_native LANGUAGES CXX)
include(CTest)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "AngelScript 2.2.0 integration requires MSVC Win32 (-A Win32).")
endif()

get_filename_component(_fresh "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(_sdk "${_fresh}/third_party/angelscript-2.2.0")

file(GLOB _engine_sources CONFIGURE_DEPENDS "${_sdk}/source/*.cpp")
add_library(pal4_angelscript_220 STATIC ${_engine_sources})
target_include_directories(pal4_angelscript_220 PUBLIC "${_sdk}/include")
target_compile_definitions(pal4_angelscript_220
    PUBLIC AS_DEPRECATED PRIVATE ANGELSCRIPT_EXPORT)

add_library(pal4_angelscript_game STATIC
    native_angelscript_engine.cpp
    "${_fresh}/src/script_global_variable_registry.cpp"
    native_script_frame_runner.cpp
    "${_sdk}/scriptstring/scriptstring.cpp")
target_compile_features(pal4_angelscript_game PUBLIC cxx_std_20)
target_include_directories(pal4_angelscript_game PUBLIC "${_fresh}/include"
    "${_sdk}/scriptstring")
target_link_libraries(pal4_angelscript_game PUBLIC pal4_angelscript_220)

add_library(pal4_angelscript_game_callbacks STATIC
    native_game_script_callbacks.cpp
    "${_fresh}/src/script_cache_context.cpp"
    native_script_entity_transform_runtime.cpp
    "${_fresh}/src/script_entity_transform.cpp"
    "${_fresh}/src/game_resource_manager_transform.cpp"
    native_script_combat_config.cpp
    "${_fresh}/src/script_combat_config.cpp"
    "${_fresh}/src/script_combat_start.cpp"
    "${_fresh}/src/script_inventory_change.cpp"
    "${_fresh}/src/script_money_transaction.cpp"
    "${_fresh}/src/script_inventory_query.cpp"
    "${_fresh}/src/script_hint_display.cpp"
    "${_fresh}/src/script_minimap_level.cpp"
    "${_fresh}/src/script_prescription_addition.cpp"
    "${_fresh}/src/script_party_membership.cpp"
    "${_fresh}/src/script_player_status.cpp"
    "${_fresh}/src/script_forbidden_skill.cpp"
    "${_fresh}/src/script_player_garb.cpp"
    "${_fresh}/src/script_player_favor.cpp"
    "${_fresh}/src/script_party_recovery.cpp"
    "${_fresh}/src/script_player_rest.cpp"
    "${_fresh}/src/script_party_magic_query.cpp"
    "${_fresh}/src/script_npc_hand_effect.cpp"
    "${_fresh}/src/script_player_gob_attachment.cpp"
    "${_fresh}/src/script_player_blend.cpp"
    "${_fresh}/src/player_manager_slot_inventory_state.cpp"
    "${_fresh}/src/script_pal_test_result.cpp"
    "${_fresh}/src/script_arena_skill_state.cpp"
    native_script_random_talk_pool.cpp
    "${_fresh}/src/script_random_talk_show.cpp"
    "${_fresh}/src/script_command_queue.cpp"
    "${_fresh}/src/script_pet_visibility.cpp"
    native_script_timer.cpp
    "${_fresh}/src/pal_script_executor.cpp"
    "${_fresh}/src/pal_script_timed_processor.cpp"
    "${_fresh}/src/script_timed_state.cpp"
    "${_fresh}/src/background_music_player.cpp"
    "${_fresh}/src/script_bgm_callbacks.cpp"
    "${_fresh}/src/script_camera_callbacks.cpp"
    "${_fresh}/src/script_gameplay_callbacks.cpp"
    "${_fresh}/src/script_camera_sequences.cpp"
    "${_fresh}/src/player_manager_object.cpp"
    "${_fresh}/src/script_player_actions.cpp"
    "${_fresh}/src/script_current_player_visibility.cpp"
    "${_fresh}/src/script_player_action.cpp"
    "${_fresh}/src/script_action_hold_release.cpp"
    "${_fresh}/src/script_player_equipment.cpp"
    "${_fresh}/src/inventory_item_slots.cpp"
    "${_fresh}/src/script_player_position.cpp"
    "${_fresh}/src/script_random_player_position.cpp"
    "${_fresh}/src/script_world_misc.cpp"
    "${_fresh}/src/script_player_rotation.cpp"
    "${_fresh}/src/script_player_turn.cpp"
    "${_fresh}/src/script_player_walk.cpp"
    "${_fresh}/src/script_system_ui.cpp"
    "${_fresh}/src/script_sound_2d.cpp"
    "${_fresh}/src/script_wait_handler.cpp"
    "${_fresh}/src/script_talk_handler.cpp"
    "${_fresh}/src/script_tutorial_wait.cpp"
    "${_fresh}/src/script_scenario_portrait.cpp"
    "${_fresh}/src/script_entity_callbacks.cpp"
    "${_fresh}/src/script_monster_visibility.cpp"
    "${_fresh}/src/script_monster_blend.cpp"
    native_script_flash_runtime.cpp
    native_script_timed_wait_runtime.cpp
    native_script_tutorial_runtime.cpp
    native_script_world_map_runtime.cpp
    "${_fresh}/src/script_world_map_wait.cpp"
    "${_fresh}/src/world_map_buttons.cpp"
    "${_fresh}/src/script_poetry_wait.cpp"
    native_script_quest_runtime.cpp
    native_script_inn_runtime.cpp
    native_script_common_dialog_runtime.cpp
    native_script_select_dialog_runtime.cpp
    "${_fresh}/src/script_select_dialog.cpp"
    "${_fresh}/src/script_dialogs.cpp"
    "${_fresh}/src/script_common_dialog_selection.cpp"
    "${_fresh}/src/script_inn_dialog.cpp"
    "${_fresh}/src/script_quest_dialog.cpp"
    "${_fresh}/src/script_frame_state_document.cpp"
    native_script_trade_runtime.cpp
    native_script_puzzle_runtime.cpp
    "${_fresh}/src/script_puzzle_game.cpp"
    "${_fresh}/src/script_ui_state_callbacks.cpp"
    "${_fresh}/src/script_trade_wait.cpp"
    "${_fresh}/src/script_trade_npc_behavior.cpp"
    native_script_talk_runtime.cpp
    "${_fresh}/src/effect_render_parameters.cpp"
    "${_fresh}/src/game_arena_load_handler.cpp"
    "${_fresh}/src/game_arena_ready_handler.cpp"
    "${_fresh}/src/uppercase_string_pair.cpp"
    "${_fresh}/src/pal_screen_effect.cpp"
    "${_fresh}/src/preterit_effect.cpp"
    "${_fresh}/src/script_builtin_handlers.cpp"
    "${_fresh}/src/script_builtin_forwarders.cpp"
    "${_fresh}/src/script_flash_effect.cpp"
    "${_fresh}/src/script_global_callbacks.cpp"
    "${_fresh}/src/script_movie_playback.cpp"
    "${_fresh}/src/script_music_transport.cpp")
target_sources(pal4_angelscript_game_callbacks PRIVATE
    "${_fresh}/src/script_shadow_control.cpp"
    "${_fresh}/src/script_npc_material.cpp"
    "${_fresh}/src/script_player_skill_acquisition.cpp"
    native_script_end_action_queue.cpp
    "${_fresh}/src/script_execution_dispatch.cpp"
    "${_fresh}/src/script_execution_flags.cpp"
    "${_fresh}/src/script_wait_handler_base.cpp"
    "${_fresh}/src/script_ui_timer_save_data.cpp"
    "${_fresh}/src/script_object_visibility.cpp"
    "${_fresh}/src/script_object_touch.cpp"
    "${_fresh}/src/script_object_blend.cpp"
    "${_fresh}/src/script_object_action.cpp"
    "${_fresh}/src/script_object_timed_move.cpp"
    "${_fresh}/src/script_object_timed_scale.cpp"
    "${_fresh}/src/script_current_player_timed_move.cpp"
    "${_fresh}/src/script_execution_handler_registration.cpp"
    "${_fresh}/src/script_visibility_query.cpp"
    "${_fresh}/src/script_gob_reset.cpp"
    "${_fresh}/src/script_gob_position.cpp"
    "${_fresh}/src/script_event_volume_visibility.cpp"
    "${_fresh}/src/script_object_research.cpp"
    "${_fresh}/src/script_effect_callbacks.cpp"
    "${_fresh}/src/script_night_time_query.cpp")
target_compile_features(pal4_angelscript_game_callbacks PUBLIC cxx_std_20)
target_include_directories(pal4_angelscript_game_callbacks PUBLIC "${_fresh}/include")
target_link_libraries(pal4_angelscript_game_callbacks PUBLIC pal4_angelscript_game)

if(BUILD_TESTING)
    add_executable(pal4_native_script_end_action_queue_tests
        "${_fresh}/tests/native_script_end_action_queue_tests.cpp")
    target_link_libraries(pal4_native_script_end_action_queue_tests PRIVATE pal4_angelscript_game_callbacks)
    add_test(NAME pal4_native_script_end_action_queue COMMAND pal4_native_script_end_action_queue_tests)
    add_executable(pal4_script_quest_dialog_native_boundary_tests "${_fresh}/tests/script_quest_dialog_tests.cpp")
    target_link_libraries(pal4_script_quest_dialog_native_boundary_tests PRIVATE pal4_angelscript_game_callbacks)
    add_test(NAME pal4_script_quest_dialog_native_boundary COMMAND pal4_script_quest_dialog_native_boundary_tests)
    add_executable(pal4_native_script_global_bindings_tests "${_fresh}/tests/native_script_global_bindings_tests.cpp")
    target_link_libraries(pal4_native_script_global_bindings_tests PRIVATE pal4_angelscript_game)
    add_test(NAME pal4_native_script_global_bindings COMMAND pal4_native_script_global_bindings_tests)
    add_executable(pal4_native_script_trade_runtime_tests "${_fresh}/tests/native_script_trade_runtime_tests.cpp")
    add_executable(pal4_native_script_puzzle_runtime_tests "${_fresh}/tests/native_script_puzzle_runtime_tests.cpp")
    target_link_libraries(pal4_native_script_puzzle_runtime_tests PRIVATE pal4_angelscript_game_callbacks)
    add_test(NAME pal4_native_script_puzzle_runtime COMMAND pal4_native_script_puzzle_runtime_tests)
    target_link_libraries(pal4_native_script_trade_runtime_tests PRIVATE pal4_angelscript_game_callbacks)
    add_test(NAME pal4_native_script_trade_runtime COMMAND pal4_native_script_trade_runtime_tests)
    add_executable(pal4_script_trade_wait_native_boundary_tests "${_fresh}/tests/script_trade_wait_tests.cpp")
    target_link_libraries(pal4_script_trade_wait_native_boundary_tests PRIVATE pal4_angelscript_game_callbacks)
    add_test(NAME pal4_script_trade_wait_native_boundary COMMAND pal4_script_trade_wait_native_boundary_tests)
    add_executable(pal4_native_script_combat_config_tests
        "${_fresh}/tests/native_script_combat_config_tests.cpp")
    target_link_libraries(pal4_native_script_combat_config_tests PRIVATE pal4_angelscript_game_callbacks pal4_native_combat_system_tail)
    add_test(NAME pal4_native_script_combat_config COMMAND pal4_native_script_combat_config_tests)
    add_executable(pal4_native_script_minimap_level_tests "${_fresh}/tests/native_script_minimap_level_tests.cpp")
    target_link_libraries(pal4_native_script_minimap_level_tests PRIVATE pal4_angelscript_game_callbacks)
    add_test(NAME pal4_native_script_minimap_level COMMAND pal4_native_script_minimap_level_tests)
    add_executable(pal4_angelscript_native_tests
        "${_fresh}/tests/angelscript_native_tests.cpp")
    target_compile_features(pal4_angelscript_native_tests PRIVATE cxx_std_20)
    target_link_libraries(pal4_angelscript_native_tests PRIVATE pal4_angelscript_220)
    add_test(NAME pal4_angelscript_native COMMAND pal4_angelscript_native_tests)

    add_executable(pal4_angelscript_game_tests
        "${_fresh}/tests/angelscript_game_tests.cpp")
    target_compile_features(pal4_angelscript_game_tests PRIVATE cxx_std_20)
    target_include_directories(pal4_angelscript_game_tests PRIVATE "${_sdk}/source")
    target_compile_options(pal4_angelscript_game_tests PRIVATE /Zc:strictStrings-)
    target_link_libraries(pal4_angelscript_game_tests PRIVATE pal4_angelscript_game)
    add_test(NAME pal4_angelscript_game COMMAND pal4_angelscript_game_tests)

    add_executable(pal4_angelscript_game_callbacks_tests
        "${_fresh}/tests/angelscript_game_callbacks_tests.cpp")
    target_link_libraries(pal4_angelscript_game_callbacks_tests PRIVATE
        pal4_angelscript_game_callbacks)
    add_test(NAME pal4_angelscript_game_callbacks
        COMMAND pal4_angelscript_game_callbacks_tests)

    add_executable(pal4_angelscript_frame_runner_tests
        "${_fresh}/tests/angelscript_frame_runner_tests.cpp")
    target_link_libraries(pal4_angelscript_frame_runner_tests PRIVATE
        pal4_angelscript_game_callbacks)
    add_test(NAME pal4_angelscript_frame_runner
        COMMAND pal4_angelscript_frame_runner_tests)

    add_executable(pal4_angelscript_flash_wait_tests
        "${_fresh}/tests/angelscript_flash_wait_tests.cpp")
    target_link_libraries(pal4_angelscript_flash_wait_tests PRIVATE
        pal4_angelscript_game_callbacks)
    add_test(NAME pal4_angelscript_flash_wait
        COMMAND pal4_angelscript_flash_wait_tests)
    add_executable(pal4_native_script_timed_wait_runtime_tests
        "${_fresh}/tests/native_script_timed_wait_runtime_tests.cpp")
    target_link_libraries(pal4_native_script_timed_wait_runtime_tests PRIVATE
        pal4_angelscript_game_callbacks)
    add_test(NAME pal4_native_script_timed_wait_runtime
        COMMAND pal4_native_script_timed_wait_runtime_tests)
endif()
