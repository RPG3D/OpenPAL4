#pragma once
#include "pal4/base_object_lifecycle.h"
#include "pal4/camera_data.h"
#include "pal4/pal_actor_control.h"
#include <cstdint>
#include <span>
#include <array>
#include <vector>
#include <string_view>
namespace pal4 {
class RandomNumberGenerator;
struct PlayerManagerObjectState;
struct NativeComparisonSkill {
    std::uint32_t id{},context{},counter{};
};
struct NativeComparisonCombatActor {
    std::uint32_t address{},id{};bool player{};
    std::array<std::int32_t,3> hp_sp_mp{};
    std::int32_t turn_count{};
    std::uint32_t ready{},action{};float action_progress{};
    std::array<float,3> position{},direction{};
    std::vector<NativeComparisonSkill> timed_skills,permanent_skills;
};
struct NativeComparisonCombatState {
    std::uint32_t state{},mode{};
    std::vector<NativeComparisonCombatActor> actors;
};
struct NativeComparisonScriptContext {
    const char* name{};
    std::uint32_t status{},waits{};
    std::int32_t sdk_state{},line{};
    // Process-local read-only provenance for bytecode/animation phase observers.
    std::uint32_t sdk_context_address{};
    std::int32_t sdk_function{};
    std::uint32_t wait_queue_size{},wait_queue_blocked{},action_count{};
};
struct NativeComparisonScriptState {
    std::string_view world,scene;
    // Empty until the application's registered global storage exists.
    std::vector<std::int32_t> global_variables;
    std::uint32_t inventory_address{};
    // Process-local provenance; zero until the existing scene GOB owner exists.
    std::uint32_t gob_manager_address{};
    // Existing scene owners only; never invoke lazy construction for observation.
    std::uint32_t npc_manager_address{},monster_manager_address{};
    std::int32_t money{};
    std::uint32_t common_dialog_address{};
    std::uint32_t select_dialog_address{};
    std::uint32_t world_map_address{};
    std::vector<std::uint32_t> opened_object_events;
    std::uint32_t arena_ready{},scene_depth{},fade_phase{},fade_active{};
    bool pending_scene{},timed_active{};
    float fade_elapsed{},fade_duration{};
    float fade_alpha{};
    std::array<std::uint8_t,3> fade_color{};
    bool loading_visible{};
    bool logo_active{},loading_active{},scene_active{};
    std::uint32_t scene_world{};
    std::uint64_t loading_presented{},fade_draw_calls{};
    std::vector<NativeComparisonScriptContext> contexts;
};
// Explicit comparison-only readout; no actor/camera/input writes.
inline constexpr std::uint32_t kComparisonSnapshotMessage = 0x8504u;
inline constexpr std::uint32_t kComparisonTextureSnapshotMessage = 0x8506u;
inline constexpr std::uint32_t kComparisonDrawSnapshotMessage = 0x8508u;
void BeginNativeComparisonDrawCapture(std::uint32_t request);
bool WriteNativeComparisonAtbTrace(std::uint32_t request);
void WriteNativeComparisonKeyboardBudgetSnapshot(std::uint32_t request,
    std::uint32_t keyboard_pending_samples, std::uint32_t keyboard_boundary_reason = 0u);
void ObserveNativeComparisonMovement(std::uint32_t command, float elapsed);
void WriteNativeComparisonSnapshot(std::uint32_t request,
    const BaseObjectState* actor, const PalActorControlState& control,
    const CameraDataState& camera, bool texture_pixels = false,
    std::span<const std::uint32_t> related_actors = {},
    std::uint32_t keyboard_pending_samples = 0u,
    const RandomNumberGenerator* random = nullptr, std::uint32_t initial_seed = 0u,
    const NativeComparisonCombatState* combat = nullptr, bool world_state_active = false,
    std::uint32_t keyboard_boundary_reason = 0u,
    const NativeComparisonScriptState* scripts = nullptr,
    bool numeric_only = false, const PlayerManagerObjectState* party = nullptr);
}
