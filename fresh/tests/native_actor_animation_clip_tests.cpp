#include "support/native_monster_path_replay.h"
#include "support/native_actor_load_order_probe.h"
#include "support/native_gob_retained_config_probe.h"
#include "support/native_world_texture_isolation_probe.h"
#include "support/native_scene_translation_replay.h"
#include "pal4/camera_render_pass_native.h"
#include "support/native_atomic_sphere_probe.h"
#include "support/native_actor_load_bounds_probe.h"
#include "support/native_tree_collision_probe.h"
#include "support/native_m07_npc_path_probe.h"
#include "pal4/camera_render_lifecycle_native.h"
#include "pal4/camera_render_geometry_native.h"
#include <fstream>
#include "pal4/native_hook_clump_names.h"
#include "pal4/native_shadow_atomic_flags.h"
#include "support/native_camera_effect_probe.h"
#include <rpspline.h>
#include "pal4/native_actor_animation_catalog.h"
#include "support/native_actor_instance_probe.h"
#include "support/native_combat_actor_model_probe.h"
#include "support/native_combat_monster_manager_probe.h"
#include "support/native_effect_dialog_loader_probe.h"
#include "support/native_named_camera_cleanup_probe.h"
#include "support/native_hook_effect_probe.h"
#include "support/native_strip_visual_probe.h"
#include "support/native_player_weapon_effect_probe.h"
#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/native_player_animation_set.h"
#include "pal4/native_actor_animation_clip_asset.h"
#include "pal4/native_game_database.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_actor_clump_asset.h"
#include "pal4/native_actor_control_dummy_asset.h"
#include "pal4/native_actor_control_session.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_game_object_animation_lifecycle.h"
#include "pal4/native_monster_info_reader.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_monster_scene_object_builder.h"
#include "support/native_scene_attached_effect_probe.h"
#include "support/native_behavior_animation_selection_probe.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_monster_storage_capture.h"
#include "pal4/native_arena_cache_scene_restore.h"
#include "pal4/native_arena_save_capture.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_arena_cache_owner.h"
#include "pal4/native_minimap_manager_owner.h"
#include "pal4/native_minimap_scene_ui_update.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_scene_record_catalog.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_monster_pursuit_path_runtime.h"
#include "pal4/native_base_object_motion_frame.h"
#include "pal4/native_monster_proximity_runtime.h"
#include "pal4/native_monster_action_dispatch_runtime.h"
#include "pal4/native_monster_arena_skill_frame.h"
#include "pal4/native_monster_pursuit_frame.h"
#include "pal4/native_monster_world_frame.h"
#include "pal4/native_monster_combat_collector.h"
#include "support/native_monster_interaction_probe.h"
#include "support/native_monster_encounter_probe.h"
#include "support/native_monster_idle_probe.h"
#include "pal4/native_script_event_scheduler_runtime.h"
#include "pal4/native_behavior_atomic_dispatch_runtime.h"
#include "pal4/native_behavior_class_one_tasks.h"
#include "pal4/native_behavior_path_endpoint_storage.h"
#include "pal4/game_vector_math.h"
#include "pal4/collision_actor.h"
#include "pal4/game_object_animation_record.h"
#include "pal4/native_arena_npc_scene_entry.h"
#include "pal4/native_arena_collision_tree.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_camera_object_resources.h"
#include "pal4/native_world_collision_query.h"
#include "support/rw_dict_schema_fake.h"
#include "support/native_dialog_stop_probe.h"
#include "support/native_dialog_frame_probe.h"
#include "support/native_dialog_playback_probe.h"
#include "support/native_dialog_particle_playback_probe.h"
#include "support/native_effect_document_probe.h"
#include "support/native_dialog_config_probe.h"
#include "support/native_effect_tasks_probe.h"
#include "support/native_effect_pool_probe.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_player_actor_assets.h"
#include "pal4/native_player_actor_reset_runtime.h"
#include "pal4/native_player_actor_reload_activity.h"
#include "pal4/native_player_reset_activity.h"
#include "pal4/native_player_material_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_player_weapon_cleanup.h"
#include "pal4/native_player_weapon_runtime.h"
#include "pal4/native_player_named_weapon_runtime.h"
#include "pal4/native_clump_environment_map.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_player_model_reload_runtime.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_player_manager_reset_runtime.h"
#include "pal4/native_player_role_load_runtime.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_renderware_dialog_mode.h"
#include "pal4/native_rw_dict_schema_runtime.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/native_actor_scene_parameter_runtime.h"
#include "pal4/base_object_scene_parameter.h"
#include "pal4/game_resource_manager.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_loader.h"
#include "pal4/rw_plugin_8451_native.h"
#include "pal4/rw_plugin_8449_native.h"
#include "pal4/rw_plugin_8450_native.h"
#include "pal4/rw_plugin_8452_native.h"

#include <windows.h>
#include <rphanim.h>
#include <rpworld.h>
#include <rpptank.h>
#include "support/native_particle_tank_probe.h"
#include "support/native_texture_dictionary_content_probe.h"
#include "support/native_particle_configuration_probe.h"
#include "support/native_particle_effect_factory_probe.h"
#include "support/native_dialog_particle_activation_probe.h"
#include <rpcollis.h>
#include <rpskin.h>
#include <rpnormmap.h>
#include <rpusrdat.h>
#include <rpuvanim.h>
#include <rpmatfx.h>
#include <rtvcat.h>
#include <rwcore.h>
#include <rtanim.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "support/native_ui_component_probe.h"
#include "support/native_ui_world_render_probe.h"

#include "support/native_test_display.h"

#include "support/native_package_png_probe.h"

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

RwFrame* FindHierarchy(RwFrame* frame, void* context) {
    auto*& found = *static_cast<RpHAnimHierarchy**>(context);
    if (!found) found = RpHAnimFrameGetHierarchy(frame);
    if (!found) (void)RwFrameForAllChildren(frame, FindHierarchy, context);
    return frame;
}

struct UserDataStats {
    std::uint32_t hooks{};
    std::uint32_t bottom{};
    std::uint32_t face{};
    std::uint32_t leg{};
    std::uint32_t string_arrays{};
    std::uint32_t multi_element_arrays{};
};

RpAtomic* InspectUserData(RpAtomic* atomic, void* context) {
    auto& stats = *static_cast<UserDataStats*>(context);
    auto* geometry = RpAtomicGetGeometry(atomic);
    if (!geometry) return atomic;
    bool hook = false;
    for (int array_index = 0;
         array_index < RpGeometryGetUserDataArrayCount(geometry);
         ++array_index) {
        auto* array = RpGeometryGetUserDataArray(geometry, array_index);
        if (!array || RpUserDataArrayGetFormat(array) != rpSTRINGUSERDATA)
            continue;
        ++stats.string_arrays;
        stats.multi_element_arrays += RpUserDataArrayGetNumElements(array) != 1;
        for (int element = 0; element < RpUserDataArrayGetNumElements(array);
             ++element) {
            const char* raw = RpUserDataArrayGetString(array, element);
            if (!raw) continue;
            const std::string_view value(raw);
            hook |= value.starts_with("hook_");
            stats.bottom += value.starts_with("hook_ef_bottom");
            stats.face += value.starts_with("FACE");
            stats.leg += value.starts_with("LEG");
            Check(!value.starts_with("FACE") || value.size() > 4u,
                  "original FACE tags exceed the prefix length");
            Check(!value.starts_with("LEG") || value.size() > 3u,
                  "original LEG tags exceed the prefix length");
        }
    }
    stats.hooks += hook;
    return atomic;
}

struct HiddenHookStats {
    std::uint32_t marked{};
    std::uint32_t callbacks{};
    RpAtomicCallBackRender callback{};
    RpAtomicCallBackRender visible_callback{};
};

struct RoleLoadHost final : pal4::PlayerManagerLoadRuntimeStub {
    std::uint32_t Log(std::uint32_t line) override { return line; }
    std::uint32_t GameDB() override { return 0x9000u; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        std::uint32_t result{};
        std::memcpy(&result, reinterpret_cast<const void*>(address),
                    sizeof(result));
        return result;
    }
};

struct ActorResetHost final : pal4::IBaseObjectActivityRuntime {
    pal4::NativePlayerActorAssets& actors;
    std::vector<char> calls;
    std::size_t cleanup_calls{};
    std::size_t before_count{};
    bool had_clump{};
    pal4::NativePlayerWeaponCleanup* weapons{};
    pal4::IBaseObjectActivityRuntime* bound{};

    explicit ActorResetHost(pal4::NativePlayerActorAssets& owner)
        : actors(owner) {}

    void ApplyMaterialTexture(pal4::BaseObjectState& state) override {
        before_count = actors.Count();
        had_clump = state.Read(0x14u) != 0u;
        calls.push_back('A');
    }
    void CleanupObject(pal4::BaseObjectState& state) override {
        if (bound) { bound->CleanupObject(state); return; }
        before_count = actors.Count();
        had_clump = state.Read(0x14u) != 0u;
        calls.push_back('O');
        if (weapons) weapons->Cleanup(state);
    }
    bool DialogNeedsUpdate(pal4::BaseObjectState& state,
                           std::uint32_t offset) override {
        if (bound) return bound->DialogNeedsUpdate(state, offset);
        calls.push_back(offset == 0x534u ? 'D' : 'd');
        return false;
    }
    void UpdateDialog(pal4::BaseObjectState& state, std::uint32_t offset) override {
        Check(bound != nullptr, "active player dialog requires the reset service");
        bound->UpdateDialog(state, offset);
    }
    void CleanupAnimationData(pal4::BaseObjectState& state,
                              std::uint32_t offset) override {
        if (bound) { bound->CleanupAnimationData(state, offset); return; }
        Check(offset == 0x170u && state.Read(0x530u) == 0u,
              "player reset clears hook before animation data");
        calls.push_back('N');
    }
    std::uint32_t GameInstance() override {
        calls.push_back('G');
        return 0x7000u;
    }
    void CleanupClump(pal4::BaseObjectState& state,
                      std::uint32_t texture_context) override {
        if (bound) { bound->CleanupClump(state, texture_context); return; }
        Check(texture_context == 0x71acu &&
                  actors.Count() == before_count - (had_clump ? 1u : 0u) &&
                  state.Read(0x14u) == 0u && state.Read(0x30u) == 0u &&
                  state.Read(0x550u) == 0u,
              "SDK actor is detached before resource-manager cleanup");
        ++cleanup_calls;
        calls.push_back('C');
    }
    std::uint32_t GetProperty(pal4::BaseObjectState& state) override {
        if (bound) return bound->GetProperty(state);
        calls.push_back('P');
        return 0x1234u;
    }
};

// Original weapon resources around an original actor's real hand hooks.
// Effect callbacks are explicit doubles; world and frame operations use the SDK.
struct WeaponCleanupFixture final : pal4::INativePlayerNamedWeaponHost {
    pal4::BaseObjectState& base;
    RpWorld* world{};
    std::array<RpClump*, 4u> weapons{};
    std::array<pal4::NativeGobClumpAsset, 4u> assets;
    std::vector<std::uint32_t> updated, displayed, released;
    unsigned created{}, temporary_resets{}, refreshed{};
    std::vector<std::pair<std::uint32_t,std::uint32_t>> effect_parents;
    WeaponCleanupFixture(pal4::BaseObjectState& state,
        const std::filesystem::path& root, std::string_view model) : base(state) {
        RwBBox bounds{{1000.0f,1000.0f,1000.0f},{-1000.0f,-1000.0f,-1000.0f}};
        world = RpWorldCreate(&bounds);
        Check(world != nullptr, "weapon fixture creates SDK world");
        pal4::NativePlayerTransformRuntime transforms;
        pal4::NativePlayerWeaponRuntime setup(*world, 0x7000u, *this, transforms);
        for (unsigned index=0; index<2u; ++index) {
            Check(assets[index].Load(root, "gamedata\\PALWeapon\\" + std::string(model) + "\\", model),
                  "original weapon model loads through the existing CPK clump reader");
            auto* clump = assets[index].Clump();
            auto* frame = RpClumpGetFrame(clump);
            unsigned textured_materials{};
            (void)RpClumpForAllAtomics(clump, [](RpAtomic* atomic, void* context) -> RpAtomic* {
                auto* geometry = RpAtomicGetGeometry(atomic);
                for (RwInt32 material=0; material<RpGeometryGetNumMaterials(geometry); ++material)
                    *static_cast<unsigned*>(context) +=
                        RpMaterialGetTexture(RpGeometryGetMaterial(geometry, material)) != nullptr;
                return atomic;
            }, &textured_materials);
            std::cout << "Original weapon geometry atomics=" << assets[index].AtomicCount()
                      << " textures=" << assets[index].TextureRequests()
                      << " missing=" << assets[index].MissingTextures()
                      << " decode=" << assets[index].TextureDecodeFailures()
                      << " textured=" << textured_materials
                      << " hand=" << (pal4::NativeActorUserData::FindAtomic(clump, "hook_hand") != nullptr) << '\n';
            Check(clump && frame && assets[index].AtomicCount() > 0u &&
                      textured_materials > 0u && assets[index].MissingTextures() == 0u &&
                      assets[index].TextureDecodeFailures() == 0u &&
                      pal4::NativeActorUserData::FindAtomic(clump, "hook_hand") != nullptr,
                  "original weapon geometry, textures and hand hook decode without missing resources");
            weapons[index] = clump;
            Check(pal4::NativeActorUserData::HideHooks(clump), "original weapon hooks are hidden");
            const auto hook = index % 2u == 0u ? "hook_RHWeapon" : "hook_LHWeapon";
            auto* actor = reinterpret_cast<RpClump*>(base.Read(0x14u));
            if (!pal4::NativeActorUserData::FindAtomic(actor, hook))
                (void)RpClumpForAllAtomics(actor, [](RpAtomic* atomic, void*) -> RpAtomic* {
                    auto* geometry = RpAtomicGetGeometry(atomic);
                    for (RwInt32 n=0; n<RpGeometryGetUserDataArrayCount(geometry); ++n) {
                        auto* data = RpGeometryGetUserDataArray(geometry,n);
                        if (data->format == rpSTRINGUSERDATA && data->numElements > 0) {
                            const char* text = RpUserDataArrayGetString(data,0);
                            if (text && std::string_view(text).starts_with("hook_"))
                                std::cerr << "Weapon fixture original hook=" << text << '\n';
                        }
                    }
                    return atomic;
                }, nullptr);
            Check(pal4::NativeActorUserData::FindAtomic(actor, hook) != nullptr,
                  "original actor has the expected weapon attachment hook");
            if (index < 2u) {
                const auto* hand = pal4::NativeActorUserData::FindAtomic(clump, "hook_hand");
                const auto source = *RwFrameGetMatrix(RpAtomicGetFrame(hand));
                const std::array<float,3u> scale{2.0f,3.0f,4.0f};
                const auto clump_address = reinterpret_cast<std::uint32_t>(clump);
                const auto scale_address = reinterpret_cast<std::uint32_t>(scale.data());
                Check((index == 0u ? pal4::BaseObjectWeapon::SetupRight(base, clump_address, scale_address, 0u, setup)
                                  : pal4::BaseObjectWeapon::SetupLeft(base, clump_address, scale_address, 0u, setup)) == 1u,
                      "shared original weapon setup executes through native SDK runtime");
                const auto* matrix = RwFrameGetMatrix(frame);
                const std::array<RwV3d,3u> input{source.right,source.up,source.at};
                std::array<RwV3d,3u> output{};
                (void)RwV3dTransformVectors(output.data(), input.data(), 3, matrix);
                const auto sign = index == 0u ? -1.0f : 1.0f;
                const std::array<RwV3d,3u> expected{{{sign*2.0f,0.0f,0.0f},
                    {0.0f,0.0f,3.0f},{0.0f,-sign*4.0f,0.0f}}};
                for (unsigned basis=0; basis<3u; ++basis)
                    Check(std::fabs(output[basis].x-expected[basis].x)<0.001f &&
                              std::fabs(output[basis].y-expected[basis].y)<0.001f &&
                              std::fabs(output[basis].z-expected[basis].z)<0.001f,
                          "native weapon inverse, nonuniform scale and hand rotations preserve expected basis");
                RwV3d origin{};
                (void)RwV3dTransformPoints(&origin, &source.pos, 1, matrix);
                Check(std::fabs(origin.x)<0.001f && std::fabs(origin.y)<0.001f && std::fabs(origin.z)<0.001f &&
                          RpClumpGetWorld(clump) == world &&
                          RwFrameGetParent(frame) == RpAtomicGetFrame(
                              pal4::NativeActorUserData::FindAtomic(actor,hook)),
                      "original hand origin maps to zero and weapon enters the correct world and actor hook");
                auto* effect = pal4::NativeActorUserData::FindAtomic(clump, "hook_effect");
                Check(effect_parents.back() == std::pair{
                          index == 0u ? 0x520u : 0x528u,
                          reinterpret_cast<std::uint32_t>(effect ? RpAtomicGetFrame(effect) : frame)},
                      "native setup routes effect parenting to the original weapon hook or root");
            }
            Check(RwFrameGetParent(frame) != nullptr, "weapon frame attaches to original hook");
            constexpr std::array offsets{0x518u,0x51cu,0x22cu,0x39cu};
            base.Write(offsets[index], reinterpret_cast<std::uint32_t>(clump));
        }
        Check(created == 2u && temporary_resets == 2u && refreshed == 2u,
              "both original weapons require effect creation, temporary reset and playback service");
        for (auto offset : {0x4f8u,0x500u,0x508u,0x510u}) {
            base.Write(offset, 1u); base.Write(offset+4u, 1u);
        }
    }
    ~WeaponCleanupFixture() {
        for (auto* clump : weapons) if (clump) {
            if (RpClumpGetWorld(clump)) (void)RpWorldRemoveClump(world, clump);
            if (RwFrameGetParent(RpClumpGetFrame(clump)))
                (void)RwFrameRemoveChild(RpClumpGetFrame(clump));
        }
        if (world) (void)RpWorldDestroy(world);
    }
    pal4::BaseObjectEffectHandle LoadXmlEffect(std::uint32_t, std::uint32_t, std::uint32_t) override {
        throw std::logic_error("XML effects are not requested");
    }
    bool EffectNeedsUpdate(pal4::BaseObjectState& state, std::uint32_t offset) override {
        Check(&state == &base && (offset == 0x520u || offset == 0x528u ||
                  offset == 0x534u || offset == 0x53cu), "player effect cleanup target");
        return state.Read(offset+4u) != 0u;
    }
    std::uint32_t EffectSingleton() override { return 0x8100u; }
    bool EffectAvailable(std::uint32_t) override { return false; }
    pal4::BaseObjectEffectCreation CreateEffect(std::uint32_t system, std::uint32_t name,
        std::uint32_t position, std::uint32_t first, std::uint32_t second) override {
        Check(system == 0x8100u && name == 0u && position == 0x84b1dcu && first == 1u && second == 0u,
              "native weapon setup preserves exact original effect creation arguments");
        ++created; return {{1u,1u},{2u,1u}};
    }
    void ResetTemporary(pal4::BaseObjectEffectHandle& handle) override {
        Check(handle.first == 2u && handle.second == 1u, "native setup resets the returned temporary handle");
        handle = {}; ++temporary_resets;
    }
    void SetEffectProperties(pal4::BaseObjectState& state, std::uint32_t offset, std::uint32_t frame) override {
        Check(&state == &base && state.Read(offset) == 1u && state.Read(offset+4u) == 1u,
              "native setup publishes the persistent effect before parent assignment");
        effect_parents.emplace_back(offset,frame);
    }
    void SetEffectVisibility(pal4::BaseObjectState&, std::uint32_t, std::uint32_t) override {
        throw std::logic_error("fixture does not request a body-only effect");
    }
    void RefreshEffect(pal4::BaseObjectState&, std::uint32_t) override { ++refreshed; }
    void UpdateEffect(pal4::BaseObjectState& state, std::uint32_t offset) override {
        updated.push_back(offset); state.Write(offset, 0xffffffffu); state.Write(offset+4u, 0u);
    }
    bool EffectReady(pal4::BaseObjectState& state, std::uint32_t offset) override {
        Check(&state == &base && offset >= 0x4f8u && offset <= 0x510u,
              "named weapon effect target");
        return state.Read(offset+4u) != 0u;
    }
    void DisplayAndCleanupEffect(pal4::BaseObjectState& state, std::uint32_t offset) override {
        displayed.push_back(offset); state.Write(offset, 0xffffffffu); state.Write(offset+4u, 0u);
    }
    void CleanupNamedResource(pal4::BaseObjectState& state, std::uint32_t offset,
                               std::uint32_t context) override {
        (void)state; (void)offset; (void)context;
        throw std::logic_error("named runtime must release its own resources");
    }
};

struct ActorSlotResetHost final : pal4::PlayerManagerSlotRuntimeStub {
    pal4::NativePlayerActorResetRuntime& runtime;

    explicit ActorSlotResetHost(pal4::NativePlayerActorResetRuntime& value)
        : runtime(value) {}

    void BaseReset(std::uint32_t slot) override {
        auto& state = *reinterpret_cast<pal4::BaseObjectState*>(slot);
        (void)pal4::BaseObjectActivity::Reset(state, runtime);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(value));
    }
};

RpAtomic* InspectHiddenHooks(RpAtomic* atomic, void* context) {
    auto& stats = *static_cast<HiddenHookStats*>(context);
    const auto offset = pal4::NativeRwPlugin8451Lifecycle::State().geometry_offset;
    Check(offset > 0, "PAL4 hook plugin offset");
    std::uint32_t marked{};
    std::memcpy(&marked, reinterpret_cast<const char*>(atomic) + offset + 4,
                sizeof(marked));
    if (marked) {
        ++stats.marked;
        const auto callback = RpAtomicGetRenderCallBack(atomic);
        Check(callback != nullptr, "hidden hook has a render callback");
        if (!stats.callback) stats.callback = callback;
        stats.callbacks += callback == stats.callback;
    } else if (!stats.visible_callback) {
        stats.visible_callback = RpAtomicGetRenderCallBack(atomic);
    }
    return atomic;
}

class RenderWareSession final {
public:
    void Open() {
        const auto display = pal4::test::TestDisplay();
        const auto& bounds = display.bounds;
        RECT client{0, 0, 64, 64};
        Check(AdjustWindowRect(&client, WS_OVERLAPPEDWINDOW, FALSE),
              "size hidden test window for a 64x64 client raster");
        window_ = CreateWindowA("STATIC", "PAL4 actor animation", WS_OVERLAPPEDWINDOW,
                                bounds.left, bounds.top,
                                client.right - client.left,
                                client.bottom - client.top, nullptr, nullptr,
                                GetModuleHandleA(nullptr), nullptr);
        Check(window_ != nullptr, "create RenderWare test window");
        MONITORINFOEXA opened_monitor{sizeof(opened_monitor)};
        Check(GetMonitorInfoA(MonitorFromWindow(window_, MONITOR_DEFAULTTONULL),
                              &opened_monitor) &&
                  std::string_view(opened_monitor.szDevice) == display.device,
              "RenderWare test window opens on the secondary 1600x900 screen");
        initialized_ = RwEngineInit(nullptr, rwENGINEINITFREELISTS,
                                    4u * 1024u * 1024u) != 0;
        Check(initialized_, "initialize RenderWare SDK");
        Check(RpWorldPluginAttach(), "attach SDK world plugin");
        Check(RpSplinePluginAttach(), "attach SDK spline plugin for original TrackTask resources");
        Check(RpPTankPluginAttach(), "attach SDK particle tank plugin");
        Check(RpCollisionPluginAttach(), "attach SDK collision plugin");
        (void)RpUserDataPluginAttach();
        Check(RpHAnimPluginAttach(), "attach SDK HAnim plugin");
        Check(RtAnimInitialize(), "initialize SDK animation");
        Check(RpSkinPluginAttach(), "attach SDK skin plugin");
        Check(RpNormMapPluginAttach(), "attach SDK normal-map plugin for original pick materials");
        Check(RpMatFXPluginAttach(), "attach SDK material effects plugin");
        Check(RpUVAnimPluginAttach(), "attach SDK UV animation plugin");
        Check(pal4::NativeRwPlugin8451Lifecycle::Attach(),
              "attach PAL4 hook Atomic plugin");
        Check(pal4::NativeRwPlugin8449Lifecycle::Attach() &&
                  pal4::NativeRwPlugin8450Lifecycle::Attach() &&
                  pal4::NativeRwPlugin8452Lifecycle::Attach(),
              "attach PAL4 scene Atomic plugins");
        Check(dialog_mode_.PrepareBeforeOpen(), "prepare D3D9 dialog mode");
        RwEngineOpenParams parameters{window_};
        opened_ = RwEngineOpen(&parameters) != 0;
        Check(opened_, "open RenderWare SDK");
        Check(dialog_mode_.EnableBeforeStart(true), "enable D3D9 dialog mode");
        started_ = RwEngineStart() != 0;
        Check(started_ && dialog_mode_.FinishAfterStart(),
              "start RenderWare SDK");
    }

    ~RenderWareSession() {
        if (started_) (void)RwEngineStop();
        if (opened_) (void)RwEngineClose();
        if (initialized_) (void)RwEngineTerm();
        if (window_) (void)DestroyWindow(window_);
    }

private:
    pal4::NativeRenderWareDialogMode dialog_mode_;
    HWND window_ = nullptr;
    bool initialized_ = false;
    bool opened_ = false;
    bool started_ = false;
};

class EmptyMonsterAnimationRuntime final :
    public pal4::NativeGameObjectAnimationLifecycle {
public:
    using NativeGameObjectAnimationLifecycle::NativeGameObjectAnimationLifecycle;
    std::uint32_t FindHierarchy(std::uint32_t) override {
        throw std::logic_error("monster animation hierarchy is not queried");
    }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
                              std::uint32_t, std::int32_t) override {
        throw std::logic_error("monster animation processing is not queried");
    }
};

// Inject only path rejection; event, script, transform and HAnim work stays native.
struct WallProbe {
    bool found{};
    std::array<float, 6> ray{};
};

RpAtomic* FindWallProbe(RpAtomic* atomic, void* context) {
    auto& probe = *static_cast<WallProbe*>(context);
    if (probe.found) return atomic;
    auto* geometry = RpAtomicGetGeometry(atomic);
    if (!geometry || RpGeometryGetNumTriangles(geometry) == 0) return atomic;
    const auto* triangles = RpGeometryGetTriangles(geometry);
    auto* morph = RpGeometryGetMorphTarget(geometry, 0);
    if (!morph) return atomic;
    const auto* vertices = RpMorphTargetGetVertices(morph);
    if (!vertices || !triangles) return atomic;
    for (RwInt32 index = 0; index < RpGeometryGetNumTriangles(geometry); ++index) {
        RwV3d local[3], world[3];
        for (std::size_t corner = 0; corner < 3u; ++corner)
            local[corner] = vertices[triangles[index].vertIndex[corner]];
        RwV3dTransformPoints(world, local, 3,
                            RwFrameGetLTM(RpAtomicGetFrame(atomic)));
        RwV3d a, b, normal;
        RwV3dSub(&a, &world[1], &world[0]);
        RwV3dSub(&b, &world[2], &world[0]);
        RwV3dCrossProduct(&normal, &a, &b);
        if (RwV3dLength(&normal) < 0.001f) continue;
        (void)RwV3dNormalize(&normal, &normal);
        const RwV3d center{
            (world[0].x + world[1].x + world[2].x) / 3.0f,
            (world[0].y + world[1].y + world[2].y) / 3.0f,
            (world[0].z + world[1].z + world[2].z) / 3.0f};
        probe.ray = {center.x + normal.x, center.y + normal.y,
                     center.z + normal.z, center.x - normal.x,
                     center.y - normal.y, center.z - normal.z};
        probe.found = true;
        break;
    }
    return atomic;
}

void VerifyCameraFrame(pal4::NativeWorldAsset& world) {
    pal4::NativeCameraObjectResources resources({0, 0, 64, 64}, 1.0f,
                                               2000.0f, 0.5f);
    pal4::NativeCameraManagerOwner cameras;
    Check(cameras.Construct() && cameras.InitializeCameras(resources),
          "camera frame regression initializes SDK cameras");
    const auto main_camera = cameras.ActiveCameraPointer();
    {
        pal4::test::NativeNamedCameraCleanupProbe named(cameras,resources,world.World());
        cameras.ResetSceneProperties();
        named.VerifyReleased();
    }
    Check(main_camera == cameras.Data(0u).At(0u) &&
              cameras.ActivateByName("ScriptCamera") &&
              cameras.ActiveCameraPointer() == cameras.Data(1u).At(0u) &&
              cameras.ActiveCameraPointer() != main_camera,
          "active native camera follows script camera selection");
    Check(cameras.ActivateByName("MainCamera") &&
              cameras.ActiveCameraPointer() == main_camera,
          "active native camera restores MainCamera");
    auto& data = cameras.Data(0u);
    const auto write = [&](std::size_t offset, float value) {
        data.At(offset) = std::bit_cast<std::uint32_t>(value);
    };
    const auto read = [&](std::size_t offset) {
        return std::bit_cast<float>(data.At(offset));
    };
    // Without a world there are no environment hits. A distant cached point
    // also exercises the native bounds transform without retaining an obstacle.
    write(0x48u, 20.0f);
    write(0x4cu, 40.0f);
    write(0x1b4u, 10.0f);
    write(0x2acu, 100000.0f);
    write(0x2b0u, 100000.0f);
    write(0x2b4u, 100000.0f);
    (void)resources.UpdateMatrix(data, 1u);
    auto* camera = reinterpret_cast<RwCamera*>(data.At(0u));
    const auto initial_eye = *RwMatrixGetPos(
        RwFrameGetLTM(RwCameraGetFrame(camera)));
    (void)cameras.AdvanceActiveCamera(0.25f);
    const auto recovered_eye = *RwMatrixGetPos(
        RwFrameGetLTM(RwCameraGetFrame(camera)));
    const auto dx = recovered_eye.x - initial_eye.x;
    const auto dy = recovered_eye.y - initial_eye.y;
    const auto dz = recovered_eye.z - initial_eye.z;
    Check(std::abs(read(0x48u) - 22.5f) < 0.001f &&
              std::abs(std::sqrt(dx * dx + dy * dy + dz * dz) - 2.5f) < 0.001f,
          "native camera restores distance and updates the SDK frame");
    data.At(0x1a8u) = 0u;
    (void)cameras.AdvanceActiveCamera(0.25f);
    Check(std::abs(read(0x48u) - 22.5f) < 0.001f,
          "disabled camera distance update preserves distance");
    data.At(0x1a8u) = 1u;

    WallProbe probe;
    RpClump* obstacle = nullptr;
    for (std::size_t slot = 0; slot < 5u && !probe.found; ++slot) {
        if (auto* clump = world.ClumpAt(slot)) {
            (void)RpClumpForAllAtomics(clump, FindWallProbe, &probe);
            if (probe.found) obstacle = clump;
        }
    }
    Check(probe.found && obstacle &&
              cameras.AddSceneClump(reinterpret_cast<std::uint32_t>(obstacle)),
          "camera collision fixture uses an original Q01/N01 triangle");
    Check(RpWorldAddCamera(world.World(), camera) != nullptr,
          "camera environment query borrows the original world");
    write(0x48u, 40.0f);
    write(0x4cu, 40.0f);
    write(0x1b8u, 0.01f);
    for (std::size_t axis = 0; axis < 3u; ++axis)
        write(0x74u + axis * 4u, probe.ray[axis]);
    for (std::size_t ray = 0; ray < 4u; ++ray)
        for (std::size_t axis = 0; axis < probe.ray.size(); ++axis)
            write(0x1c0u + ray * 24u + axis * 4u, probe.ray[axis]);
    (void)cameras.AdvanceActiveCamera(0.0f);
    Check(data.At(0x2d4u) == 1u && data.At(0x2e8u) < 4u &&
              std::isfinite(read(0x48u)) && read(0x48u) > 0.01f &&
              read(0x48u) < 2.0f &&
              std::abs(read(0x48u) - read(0x2b8u)) < 0.001f,
          "camera rays hit original geometry and cache a compressed distance");
    pal4::CollisionSegmentHit hit;
    Check(!pal4::NativeWorldCollisionQuery::Closest(
              {{probe.ray[0], probe.ray[1], probe.ray[2]},
               {probe.ray[3], probe.ray[4], probe.ray[5]}}, nullptr, hit),
          "shared native world segment query rejects a missing world");
    Check(cameras.RemoveSceneClump(reinterpret_cast<std::uint32_t>(obstacle)),
          "camera regression releases its borrowed scene clump");
    pal4::test::NativeNamedCameraCleanupProbe named_final(cameras,resources,world.World());
    cameras.Destroy();
    named_final.VerifyReleased(true);
    std::cout << "camera frame: distance recovery and original triangle hit verified\n";
}

class RejectedReturnPath final : public pal4::IMonsterPursuitReturnRuntime {
public:
    explicit RejectedReturnPath(pal4::NativeMonsterPursuitPathRuntime& runtime)
        : native_(runtime) {}
    pal4::MonsterPursuitTarget Target(std::uint32_t target) override {
        return native_.Target(target);
    }
    std::uint32_t FindPursuitPath(float elapsed,
                                 std::array<float, 3>& point) override {
        return native_.FindPursuitPath(elapsed, point);
    }
    bool ApplyPursuitPath(std::uint32_t, const std::array<float, 3>&,
                          const std::array<float, 3>&, std::uint32_t,
                          std::uint32_t) override {
        ++rejections;
        return false;
    }
    float MaxPursuitSpeed(std::uint32_t source, float fallback) override {
        return native_.MaxPursuitSpeed(source, fallback);
    }
    std::uint32_t SetPursuitSpeed(std::uint32_t movement, float speed) override {
        return native_.SetPursuitSpeed(movement, speed);
    }
    std::uint32_t ResetPursuit(pal4::MonsterBehaviorManagerState& state,
                              std::uint32_t object) override {
        ++resets;
        return native_.ResetPursuit(state, object);
    }
    std::uint32_t rejections = 0u;
    std::uint32_t resets = 0u;
private:
    pal4::NativeMonsterPursuitPathRuntime& native_;
};

// Deliberately holds the original interaction-busy flag while movement is
// tested. Any attempt to pass the encounter gate is a test failure.
class BusyInteractionFixture final : public pal4::IMonsterInteractionRuntime {
public:
    explicit BusyInteractionFixture(const std::uint32_t* control) : control_(control) {}
    bool PlayerControlBusy() override {
        if (!control_[0x2dcu / 4u])
            throw std::logic_error("encounter script fixture boundary reached");
        ++checks;
        return true;
    }
    std::uint32_t checks = 0u;
    pal4::MonsterInteractionTarget Target(std::uint32_t) override { Unexpected(); }
    double PlanarDistance(float, float) override { Unexpected(); }
    float PlayerDistanceBase() override { Unexpected(); }
    std::array<float, 3> TargetForward(std::uint32_t) override { Unexpected(); }
    std::array<float, 3> ManagerForward(std::uint32_t) override { Unexpected(); }
    void NormalizeDirection(std::array<float, 3>&) override { Unexpected(); }
    double AngleForForwardDot(float) override { Unexpected(); }
    std::string ReadScriptName(std::uint32_t,
        const std::array<std::uint32_t, 4>&, const char*) override { Unexpected(); }
    void InitializeScripts() override { Unexpected(); }
    std::string ArenaScriptPath() override { Unexpected(); }
    bool ScriptCached(const std::string&) override { Unexpected(); }
    std::uint32_t CreateInterpreter(const std::string&, const std::string&,
                                    const std::string&) override { Unexpected(); }
    bool ExecuteInterpreter(std::uint32_t) override { Unexpected(); }
    void HandleInterpreterError(std::uint32_t) override { Unexpected(); }
    void ReleaseInterpreter(std::uint32_t) override { Unexpected(); }
    void ShowMissingScriptError() override { Unexpected(); }
    void CleanupScripts() override { Unexpected(); }
    std::int32_t PendingStateCount(std::uint32_t) override { Unexpected(); }
    void ReactivateManager(std::uint32_t, std::uint32_t) override { Unexpected(); }
    void RestoreRenderCallback(std::uint32_t) override { Unexpected(); }
private:
    [[noreturn]] static void Unexpected() {
        throw std::logic_error("busy encounter fixture unexpectedly passed its gate");
    }
    const std::uint32_t* control_;
};

class UnboundEncounterFixture final : public pal4::IMonsterEncounterRuntime {
public:
    void DispatchCombatPositions(std::uint32_t, float) override {
        throw std::logic_error("combat positions are outside this fixture");
    }
    void InitializeCombat(std::uint32_t, std::int32_t) override {
        throw std::logic_error("combat initialization is outside this fixture");
    }
};

class UnboundBehaviorTaskFixture final : public pal4::INativeBehaviorTaskFactory {
public:
    std::uint32_t Create(std::uint32_t tree, std::int32_t class_id,
        std::uint32_t container, const char* name,
        const pal4::BehaviorAtomicWaitOptions& options) override {
        ++calls;
        last_tree = tree;
        last_class = class_id;
        last_container = container;
        last_name = name;
        last_options = options;
        throw std::logic_error("native behavior task fixture boundary");
    }
    void Bind(std::uint32_t, std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unconstructed native behavior task binding");
    }
    std::uint32_t calls = 0u;
    std::uint32_t last_tree = 0u;
    std::int32_t last_class = -1;
    std::uint32_t last_container = 0u;
    std::string last_name;
    pal4::BehaviorAtomicWaitOptions last_options{};
};

class ClassOneHelperServiceFixture final : public pal4::INativeBehaviorClassOneHelpers {
public:
    explicit ClassOneHelperServiceFixture(pal4::NativeBaseObjectMemory& memory,
        std::uint32_t target) : memory_(memory), target_(target), storage_(
            memory, next_identifier_, [](std::uint32_t, bool) {
                throw std::logic_error("class-one native association cleanup is not bound");
            }) {}
    void Construct(std::uint32_t helper) override {
        ++construction_attempts;
        if (!allow_construct)
            throw std::logic_error("native class-one helper construction boundary");
        storage_.Construct(helper);
        Check(memory_.ReadDword(helper) == 0x842898u &&
                  memory_.ReadDword(helper + 4u) == 0x3f800000u &&
                  memory_.ReadDword(helper + 0x5cu) == 0x3f800000u &&
                  memory_.ReadByte(helper + 0xb8u) == 0u,
              "class-one task constructs actual endpoint helper before target acceptance");
    }
    void Discard(std::uint32_t helper) override {
        storage_.Destruct(helper);
        ++discards;
    }
    void Accept(std::uint32_t target, std::uint32_t helper) override {
        Check(target == target_, "class-one accept uses C03 target");
        if (reject_accept) throw std::logic_error("class-one accept fixture failure");
        ++accepts;
        memory_.WriteByte(helper + 0xb8u, 1u);
    }
    void Update(std::uint32_t target, std::uint32_t helper) override {
        Check(target == target_ && memory_.ReadByte(helper + 0xb8u),
              "class-one update receives active C03 service record");
        ++updates;
    }
    void Release(std::uint32_t target, std::uint32_t helper) override {
        Check(target == target_, "class-one release uses C03 target");
        ++releases;
        storage_.Destruct(helper);
        (void)memory_.Free(helper);
    }
    bool allow_construct{};
    bool reject_accept{};
    std::uint32_t construction_attempts{};
    std::uint32_t accepts{}, updates{}, releases{}, discards{};
private:
    pal4::NativeBaseObjectMemory& memory_;
    std::uint32_t target_;
    std::uint32_t next_identifier_{};
    pal4::NativeBehaviorPathEndpointStorage storage_;
};

}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    _set_abort_behavior(0u, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    try {
        Check(argc == 2 || (argc==4 && (std::string_view(argv[2])=="--monster-path-replay" || std::string_view(argv[2])=="--scene-translation-replay" || std::string_view(argv[2])=="--scene-movement-probe")) || (argc==3 && (std::string_view(argv[2])=="--actor-load-bounds" || std::string_view(argv[2])=="--cave-worlds" || std::string_view(argv[2])=="--ui-components" || std::string_view(argv[2])=="--property-preview" || std::string_view(argv[2])=="--system-property" || std::string_view(argv[2])=="--equipment-preview" || std::string_view(argv[2])=="--smith-preview" || std::string_view(argv[2])=="--ui-world-render" || std::string_view(argv[2])=="--m07-npc-path" || std::string_view(argv[2])=="--world-texture-isolation" || std::string_view(argv[2])=="--combat-world-materials")), "original game root argument");
        const std::filesystem::path root(argv[1]);
        pal4::test::NativeDesktopObservation desktop;
        RenderWareSession session;
        session.Open();
        if(argc==3 && std::string_view(argv[2])=="--actor-load-bounds") {
            pal4::NativeBaseObjectMemory memory;
            pal4::test::RwDictSchemaFake schema;
            EmptyMonsterAnimationRuntime animation(memory);
            pal4::test::VerifyNative472LoadBounds(root,memory,schema,animation);
            Check(memory.OutstandingAllocations()==0u,"472 bounds scene resources retire");
            desktop.Check("472 bounds probe preserves focus and cursor");return 0;
        }
        if(argc==3 && std::string_view(argv[2])=="--cave-worlds"){
            for(const char* section:{"1","2","3"}){
                pal4::NativeWorldAsset world;
                const bool loaded=world.LoadArenaSection(root,"M01",section);
                std::cout<<"cave="<<section<<" loaded="<<loaded<<" sectors="<<world.SectorCount()<<" textures="<<world.TextureCount()<<" missing="<<world.MissingTextures()<<" first_missing="<<world.FirstMissingTexture()<<" error="<<world.LastError()<<'\n';
                if(loaded){
                    std::size_t invalid=0;
                    RpWorldForAllWorldSectors(world.World(),[](RpWorldSector* sector,void* context)->RpWorldSector*{
                        if(sector->texCoords[0])for(unsigned i=0;i<sector->numVertices;++i)
                            if(!std::isfinite(sector->texCoords[0][i].u) || !std::isfinite(sector->texCoords[0][i].v))++*static_cast<std::size_t*>(context);
                        return sector;
                    },&invalid);
                    std::cout<<"cave="<<section<<" nonfinite_uv0_vertices="<<invalid<<'\n';
                }
            }
            desktop.Check("cave world probe preserves focus and cursor");return 0;
        }
        if(argc==3 && std::string_view(argv[2])=="--combat-world-materials") {
            pal4::test::VerifyNativeCombatWorldMaterials(root);
            desktop.Check("Combat world material regression preserves focus and cursor");
            return 0;
        }
        if(argc==3 && std::string_view(argv[2])=="--world-texture-isolation") {
            pal4::test::VerifyNativeWorldTextureIsolation(root);
            desktop.Check("world texture isolation preserves desktop focus and cursor");
            return 0;
        }
        if(argc==3 && std::string_view(argv[2])=="--m07-npc-path") {
            pal4::test::VerifyNativeM07NpcPath(root);
            desktop.Check("M07 NPC path regression preserves desktop focus and cursor");
            return 0;
        }
        if(argc==4){
            if(std::string_view(argv[2])=="--scene-translation-replay")pal4::test::ReplayNativeSceneTranslation(root,argv[3]);
            else if(std::string_view(argv[2])=="--scene-movement-probe")pal4::test::ProbeNativeSceneMovement(root,argv[3]);
            else pal4::test::ReplayNativeMonsterPaths(root,argv[3]);
            desktop.Check("native path/translation replay preserves focus and cursor");return 0;
        }
        pal4::test::VerifyNativeAtomicSpheres();
        pal4::test::VerifyNativeActorLoadPoseBounds(root);
        pal4::test::VerifyNativeOriginalTreeCollision(root);
        pal4::test::VerifyNativePackagePng(root);
        if(argc==3 && std::string_view(argv[2])=="--ui-world-render"){
            pal4::test::VerifyNativeUiWorldRender(root);desktop.Check("native UI world preserves desktop focus and cursor");return 0;}
        if(argc==3){pal4::test::VerifyNativeUiComponents(root,std::string_view(argv[2])=="--property-preview",std::string_view(argv[2])=="--system-property",std::string_view(argv[2])=="--equipment-preview",std::string_view(argv[2])=="--smith-preview");
            desktop.Check("native UI components preserve desktop focus and cursor");return 0;}
        pal4::test::VerifyNativeActorInstance(root);
        pal4::test::VerifyNativeCombatActorModel(root);
        pal4::test::VerifyNativeCombatMonsterManager(root);
        pal4::test::VerifyNativeCameraEffect(true);
        pal4::test::VerifyNativeParticleTank();
        pal4::test::VerifyNativeTextureDictionaryContent(root);
        pal4::test::VerifyNativeParticleConfiguration(root);
        pal4::test::VerifyNativeParticleEffectFactory(root);
        pal4::test::VerifyNativeDialogParticleActivation(root);
        pal4::test::VerifyNativeDialogStop();
        pal4::test::VerifyNativeDialogFrame();
        pal4::test::VerifyNativeDialogPlayback(root);
        pal4::test::VerifyNativeDialogParticlePlayback(root);
        pal4::test::VerifyNativeEffectDialogLoader(root);
        pal4::test::VerifyNativePlayerWeaponEffects(root);
        {
            pal4::NativeBaseObjectMemory memory;
            pal4::test::RwDictSchemaFake schema;
            EmptyMonsterAnimationRuntime animation(memory);
            pal4::test::VerifyNativeSceneAttachedEffects(root,memory,schema,animation);
            Check(memory.OutstandingAllocations()==0u,"attached491 actor buffers retire with their creating owner");
            pal4::test::VerifyNativeBehaviorAnimationSelection(root,memory,schema,animation);
            Check(memory.OutstandingAllocations()==0u,"behavior selection actor buffers retire with their creating owner");
        }
        pal4::test::VerifyNativeEffectDocument(root);
        pal4::test::VerifyNativeDialogConfig(root);
        pal4::test::VerifyNativeEffectTasks(root);
        pal4::test::VerifyNativeEffectPool();
        pal4::NativeActorControlDummyAsset dummy;
        if (!dummy.Load(root))
            std::cerr << "JDumy: " << dummy.LastError() << '\n';
        Check(dummy.Main() && dummy.AtomicCount() != 0u &&
                  dummy.MissingTextures() == 0u &&
                  !dummy.Load(root),
              "stream and own original ActorControl JDumy clump");
        dummy.Unload();
        Check(dummy.Main() == nullptr && dummy.AtomicCount() == 0u,
              "release original ActorControl JDumy clump");
        {
            pal4::NativeBaseObjectMemory memory;
            pal4::test::RwDictSchemaFake schema;
            pal4::NativeActorControlSession actor_control(
                memory, schema, {60, 60}, nullptr, &dummy, root, 0x5000u);
            (void)actor_control.Construct();
            actor_control.Runtime().InitializeDatabase(
                actor_control.State().words.data() + 35u);
            Check(dummy.Main() &&
                      actor_control.State().words[75u] == dummy.Address() &&
                      actor_control.State().words[63u] == 0u,
                  "ActorControl database binds original JDumy Clump");
            actor_control.Runtime().ShutdownDatabase(
                actor_control.State().words.data() + 35u, 0u);
            Check(dummy.Main() == nullptr &&
                      actor_control.State().words[75u] == 0u,
                  "ActorControl database releases original JDumy Clump");
            actor_control.Destroy();
            Check(memory.OutstandingAllocations() == 0u,
                  "ActorControl owner releases names after JDumy shutdown");
        }
        pal4::NativePackageArchive archive;
        Check(archive.Open((root / "gamedata" / "PALActor.cpk").string().c_str()),
              "open original actor archive for animation clips");
        pal4::NativePackageArchive objects;
        Check(objects.Open((root / "gamedata" / "palobject.cpk").string().c_str()),
              "open original object archive for Machine clip");
        pal4::NativeActorAnimationClipAsset machine_clip;
        Check(machine_clip.Load(objects, "OM07", "OM07", "OM07.anm") &&
                  machine_clip.Animation() &&
                  machine_clip.Animation()->numFrames > 0 &&
                  machine_clip.Animation()->duration > 0.0f,
              "decode Q01 Machine clip through shared SDK loader");
        machine_clip.Unload();
        Check(machine_clip.Animation() == nullptr,
              "release Q01 Machine SDK clip");
        pal4::NativeGobClumpAsset machine_model;
        Check(machine_model.Load(root, "gamedata\\PALObject\\OM07\\", "OM07"),
              "load original Q01 Machine clump");
        RpHAnimHierarchy* machine_hierarchy = nullptr;
        (void)RwFrameForAllChildren(RpClumpGetFrame(machine_model.Clump()),
                                    FindHierarchy, &machine_hierarchy);
        Check(machine_hierarchy && machine_hierarchy->currentAnim &&
                  machine_hierarchy->numNodes > 0,
              "Q01 Machine clump contains a usable HAnim hierarchy");
        machine_model.Unload();
        // M_011 names HY\\xianhe\\XH; use the existing SDK asset owners for
        // its nested Effect.cpk directory as well as ordinary scene objects.
        pal4::NativeGobClumpAsset effect_model;
        Check(!effect_model.Load(root, "gamedata\\effect\\clump\\..\\", "XH"),
              "effect resource directory rejects parent traversal");
        if (!effect_model.Load(root, "gamedata\\effect\\clump\\HY\\xianhe\\", "XH"))
            std::cerr << effect_model.LastError() << '\n';
        Check(effect_model.Clump() && effect_model.AtomicCount() > 0u &&
                  effect_model.MissingTextures() == 0u &&
                  effect_model.TextureDecodeFailures() == 0u &&
                  effect_model.MemberDirectory() == "clump\\HY\\xianhe",
              "load actual M_011 crane geometry and textures through SDK");
        pal4::NativePackageArchive effects;
        Check(effects.Open(effect_model.PackagePath().string().c_str()),
              "open original effect package for crane animation");
        pal4::NativeActorAnimationClipAsset effect_clip;
        Check(!effect_clip.Load(effects, "clump\\..\\xianhe", "XH", "XH.anm"),
              "animation directory rejects parent traversal");
        Check(effect_clip.Load(effects, effect_model.MemberDirectory(), "XH", "XH.anm") &&
                  effect_clip.Animation()->numFrames > 0 &&
                  effect_clip.Animation()->duration > 0.0f,
              "decode actual M_011 crane animation through existing SDK loader");
        RpHAnimHierarchy* effect_hierarchy = nullptr;
        (void)RwFrameForAllChildren(RpClumpGetFrame(effect_model.Clump()),
                                   FindHierarchy, &effect_hierarchy);
        Check(effect_hierarchy && effect_hierarchy->currentAnim &&
                  effect_hierarchy->numNodes > 0,
              "M_011 crane model provides native animation hierarchy");
        effect_hierarchy->flags |= 0x3000;
        Check(RpHAnimHierarchyAttach(effect_hierarchy) == effect_hierarchy &&
                  RpClumpForAllAtomics(effect_model.Clump(),
                      [](RpAtomic* atomic, void* context) -> RpAtomic* {
                          (void)RpSkinAtomicSetHAnimHierarchy(atomic,
                              static_cast<RpHAnimHierarchy*>(context));
                          return atomic;
                      }, effect_hierarchy) == effect_model.Clump() &&
                  RtAnimInterpolatorSetCurrentAnim(effect_hierarchy->currentAnim,
                      effect_clip.Animation()) &&
                  RtAnimInterpolatorSetCurrentTime(effect_hierarchy->currentAnim, 0.0f) &&
                  RpHAnimHierarchyUpdateMatrices(effect_hierarchy),
              "bind M_011 crane skin and animation using original SDK sequence");
        Check(RtAnimInterpolatorAddAnimTime(effect_hierarchy->currentAnim, 0.25f) &&
                  RpHAnimHierarchyUpdateMatrices(effect_hierarchy) &&
                  std::abs(effect_hierarchy->currentAnim->currentTime - 0.25f) < 0.0001f,
              "advance actual M_011 crane animation through native interpolator");
        std::cout << "M_011 crane atomics=" << effect_model.AtomicCount()
                  << " textures=" << effect_model.TextureRequests()
                  << " nodes=" << effect_hierarchy->numNodes
                  << " duration=" << effect_clip.Animation()->duration << '\n';
        effect_model.Unload();
        effect_clip.Unload();
        pal4::NativeHookClumpNames shadow_names;
        Check(shadow_names.Load(root / "gamedata/Common/rpHookNameTable.hnt") && shadow_names.Count()==2,
              "load original shadow hook exception names");
        for (const std::string_view model : {"101", "103", "106", "105"}) {
            pal4::NativeActorClumpAsset clump;
            Check(!clump.LoadShadow(),"shadow phase rejects an unloaded main");
            Check(clump.LoadMain(root, model) && clump.Main() && !clump.Shadow() && !clump.ShadowLight(),
                  "main phase owns geometry without allocating the later shadow");
            auto* retained_main=clump.Main();
            Check(clump.LoadShadow() && clump.Main()==retained_main,
                  "later shadow phase retains the original main clump");
            auto* retained_shadow=clump.Shadow();
            Check(!clump.LoadShadow() && clump.Main()==retained_main && clump.Shadow()==retained_shadow,
                  "duplicate shadow phase cannot replace existing resources");
            using TexturePointers=std::vector<RwTexture*>;
            const auto collect=[](RpAtomic* atomic,void* data)->RpAtomic* {
                auto* geometry=RpAtomicGetGeometry(atomic);
                if(geometry)for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i)
                    if(auto* texture=RpMaterialGetTexture(RpGeometryGetMaterial(geometry,i)))
                        static_cast<TexturePointers*>(data)->push_back(texture);
                return atomic;
            };
            TexturePointers main_textures,shadow_textures;
            RpClumpForAllAtomics(clump.Main(),collect,&main_textures);
            if(clump.Shadow())RpClumpForAllAtomics(clump.Shadow(),collect,&shadow_textures);
            Check(!main_textures.empty() && !shadow_textures.empty(),"original main/shadow texture resources");
            for(auto* texture:main_textures)Check(RwTextureGetFilterMode(texture)==rwFILTERLINEAR,"main mode1 retains linear filter");
            for(auto* texture:shadow_textures) {
                Check(RwTextureGetFilterMode(texture)==rwFILTERLINEARMIPLINEAR,"shadow stream retains its mip filter");
                Check(std::find(main_textures.begin(),main_textures.end(),texture)==main_textures.end(),
                    "DFF path dictionaries isolate main/shadow texture identity");
            }
            auto* shadow_light=clump.ShadowLight();
            auto* shadow_frame=shadow_light?RpLightGetFrame(shadow_light):nullptr;
            Check(shadow_light && RpLightGetType(shadow_light)==rpLIGHTDIRECTIONAL && shadow_frame &&
                      RwFrameGetParent(shadow_frame)==RpClumpGetFrame(clump.Shadow()) &&
                      RpLightGetWorld(shadow_light)==nullptr &&
                      std::fabs(RwFrameGetMatrix(shadow_frame)->at.z)<0.1f,
                  "original actor shadow owns its rotated light/frame under the shadow clump");
            using GeometryFlags=std::vector<std::pair<RpGeometry*,RwUInt32>>;
            GeometryFlags shadow_flags;
            (void)RpClumpForAllAtomics(clump.Shadow(),[](RpAtomic* atomic,void* data)->RpAtomic* {
                auto* geometry=RpAtomicGetGeometry(atomic);
                static_cast<GeometryFlags*>(data)->emplace_back(geometry,RpGeometryGetFlags(geometry));
                return atomic;
            },&shadow_flags);
            std::vector<bool> hook_exceptions;
            unsigned initially_lit=0;
            for(const auto& [geometry,flags]:shadow_flags) {
                initially_lit+=(flags&0x60u)!=0;
                bool matched=false;
                const int count=RpGeometryGetUserDataArrayCount(geometry);
                for(int i=0;i<count;++i) {
                    auto* data=RpGeometryGetUserDataArray(geometry,i);
                    if(data->format!=rpSTRINGUSERDATA)continue;
                    const auto* text=RpUserDataArrayGetString(data,i);
                    matched|=shadow_names.Matches(text?text:"");
                }
                hook_exceptions.push_back(matched);
            }
            Check(!shadow_flags.empty(),"real shadow geometry count");
            pal4::ApplyNativeShadowAtomicFlags(clump.Shadow(),shadow_names,false);
            for(std::size_t i=0;i<shadow_flags.size();++i) {
                const auto& [geometry,flags]=shadow_flags[i];
                Check(RpGeometryGetFlags(geometry)==(hook_exceptions[i]?flags:(flags&~0x60u)),
                    "real shadow geometries clear unmatched light bits");
            }
            pal4::ApplyNativeShadowAtomicFlags(clump.Shadow(),shadow_names,true);
            for(std::size_t i=0;i<shadow_flags.size();++i) {
                const auto& [geometry,flags]=shadow_flags[i];
                Check(RpGeometryGetFlags(geometry)==(hook_exceptions[i]?flags:(flags|0x60u)),
                    "real shadow post callback sets unmatched light bits");
            }
            std::cout << "Shadow flags model=" << model << " geometries=" << shadow_flags.size()
                      << " initially_lit=" << initially_lit << '\n';
            RpHAnimHierarchy* hierarchy = nullptr;
            (void)RwFrameForAllChildren(RpClumpGetFrame(clump.Main()),
                                        FindHierarchy, &hierarchy);
            Check(hierarchy && hierarchy->numNodes > 0,
                  "original actor clump contains HAnim hierarchy");
            UserDataStats metadata;
            (void)RpClumpForAllAtomics(clump.Main(), InspectUserData,
                                       &metadata);
            UserDataStats shadow_metadata;
            (void)RpClumpForAllAtomics(clump.Shadow(), InspectUserData,
                                       &shadow_metadata);
            std::cout << "Model " << model << " hooks=" << metadata.hooks
                      << " bottom=" << metadata.bottom
                      << " face=" << metadata.face
                      << " leg=" << metadata.leg
                      << " string_arrays=" << metadata.string_arrays
                      << " multi_element=" << metadata.multi_element_arrays
                      << " shadow_hooks=" << shadow_metadata.hooks
                      << '\n';
            Check(metadata.multi_element_arrays == 0u &&
                      shadow_metadata.multi_element_arrays == 0u,
                  "original string UserData arrays contain one element");
            pal4::NativeActorAnimationCatalog catalog;
            Check(catalog.Load(root, model),
                  "read original actor animation catalog");
            for (const auto& [action, filename] : catalog.Entries()) {
                pal4::NativeActorAnimationClipAsset clip;
                if (!clip.Load(archive, model, filename))
                    std::cerr << "Model " << model << " action " << action
                              << ": " << clip.LastError() << '\n';
                Check(clip.Animation() && clip.Animation()->numFrames > 0 &&
                          clip.Animation()->duration > 0.0f,
                      "decode original actor clip through SDK");
                clip.Unload();
                Check(clip.Animation() == nullptr,
                      "release original actor SDK animation");
            }
            clump.Unload();
            Check(!clump.ShadowLight(),"actor clump unload retires shadow light before its frame hierarchy");
            pal4::NativeAnimatedActorAsset actor;
            {
                pal4::test::ActorLoadOrderProbe order(actor);
                if (!actor.Load(root, model))
                    std::cerr << "Model " << model << ": " << actor.LastError() << '\n';
                Check(order.Passed(),"actual SDK animation allocations precede shadow publication");
            }
            Check(actor.Main() && actor.Shadow() && actor.ShadowLight() && actor.Hierarchy() &&
                      actor.ActionCount() == catalog.Entries().size() &&
                      actor.CurrentAction() == catalog.Entries().front().first &&
                      actor.BottomEffectHook() &&
                      actor.FaceMaterial() && !actor.LegMaterial() &&
                      actor.HiddenHookCount() == metadata.hooks,
                  "own and bind all original actor actions");
            HiddenHookStats hidden;
            (void)RpClumpForAllAtomics(actor.Main(), InspectHiddenHooks,
                                       &hidden);
            HiddenHookStats shadow_hidden;
            (void)RpClumpForAllAtomics(actor.Shadow(), InspectHiddenHooks,
                                       &shadow_hidden);
            Check(hidden.marked == metadata.hooks &&
                      hidden.callbacks == metadata.hooks &&
                      hidden.callback != hidden.visible_callback &&
                      shadow_hidden.marked == shadow_metadata.hooks &&
                      shadow_hidden.callbacks == shadow_metadata.hooks,
                  "all hook Atomics use one hidden render callback");
            pal4::GameResourceManager transform;
            (void)transform.Construct();
            transform.words[5] = reinterpret_cast<std::uint32_t>(actor.Main());
            transform.words[12] = reinterpret_cast<std::uint32_t>(actor.Shadow());
            pal4::NativePlayerTransformRuntime transform_runtime;
            const auto transformed = transform.UpdateClumpTransform(
                {2.0f, 3.0f, 4.0f}, false, transform_runtime);
            const auto* frame = RwFrameGetLTM(RpClumpGetFrame(actor.Main()));
            Check(transformed != 0u && frame &&
                      std::fabs(frame->pos.x - 2.0f) < 0.01f &&
                      std::fabs(frame->pos.y - 3.0f) < 0.01f &&
                      std::fabs(frame->pos.z - 4.0f) < 0.01f &&
                      std::bit_cast<float>(transform.words[56]) == 2.0f &&
                      std::bit_cast<float>(transform.words[57]) == 3.0f &&
                      std::bit_cast<float>(transform.words[58]) == 4.0f,
                  "rewritten player translation updates SDK clump frame");
            {
                (void)transform.CalculateBoundingBox(transform_runtime);
                RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
                struct WorldOwner { RpWorld* value;~WorldOwner(){if(value)RpWorldDestroy(value);} } world{RpWorldCreate(&bounds)};
                Check(world.value!=nullptr,"shadow frame world fixture");
                pal4::NativeCameraRenderLifecycleRuntime lifecycle(pal4::NativeCameraRenderPassRuntime::RenderSector);
                pal4::CameraRenderState shadow_state;
                struct StateOwner {
                    pal4::CameraRenderState& state;pal4::NativeCameraRenderLifecycleRuntime& lifecycle;
                    ~StateOwner(){(void)pal4::CameraRenderObject::Cleanup(state,lifecycle);}
                } state_owner{shadow_state,lifecycle};
                Check(pal4::CameraRenderObject::Initialize(shadow_state,reinterpret_cast<std::uint32_t>(world.value),
                    reinterpret_cast<std::uint32_t>(actor.Shadow()),reinterpret_cast<std::uint32_t>(actor.ShadowLight()),
                    reinterpret_cast<std::uint32_t>(transform.words.data()+52),reinterpret_cast<std::uint32_t>(transform.words.data()+56),
                    {6,true},lifecycle)!=0,"original actor shadow camera initialize");
                pal4::NativeCameraRenderPassRuntime pass(shadow_names);
                pal4::NativeCameraRenderGeometryRuntime geometry;
                pal4::CameraRenderGlobals globals;
                globals.render_enabled=true;globals.stereo=true;globals.post_process_mode=2;
                (void)pal4::CameraRenderPass::RenderScene(shadow_state,0,0,0,false,globals,geometry,pass);
                Check(shadow_state.current_raster==shadow_state.stereo_raster &&
                    !RwCameraGetRaster(reinterpret_cast<RwCamera*>(shadow_state.auxiliary_camera)),
                    "original shadow scene selects processed32 raster and unbinds auxiliary");
                auto* image=RwImageCreate(32,32,32);
                Check(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,
                    reinterpret_cast<RwRaster*>(shadow_state.current_raster)),"read actual actor shadow mask");
                unsigned dark=0,bright=0;
                for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
                    const auto* pixel=reinterpret_cast<RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image)+x*4);
                    dark+=pixel->red<8 && pixel->green<8 && pixel->blue<8;
                    bright+=pixel->red>32 || pixel->green>32 || pixel->blue>32;
                }
                // Export through the test's CRT stream; the active SDK package FS can be read-only.
                const auto output="H:/PAL4/objs/shadow-pass-20261023-"+std::string(model)+".bmp";
                std::ofstream file(output,std::ios::binary);
                BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
                header.bfSize=header.bfOffBits+32*32*4;
                BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=32;info.biHeight=-32;
                info.biPlanes=1;info.biBitCount=32;info.biSizeImage=32*32*4;
                file.write(reinterpret_cast<const char*>(&header),sizeof(header));
                file.write(reinterpret_cast<const char*>(&info),sizeof(info));
                for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
                    const auto* pixel=reinterpret_cast<RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image)+x*4);
                    const char bgra[]{static_cast<char>(pixel->blue),static_cast<char>(pixel->green),static_cast<char>(pixel->red),static_cast<char>(255)};
                    file.write(bgra,4);
                }
                Check(file.good(),"write actual actor shadow mask evidence");
                RwImageDestroy(image);
                std::cout << "Shadow scene model=" << model << " dark=" << dark << " bright=" << bright << '\n';
                Check(dark>50 && bright>5,"real actor silhouette survives64 to32 and two processing rounds");
            }
            pal4::BaseObjectState scene_actor;
            scene_actor.Write(0x14u, reinterpret_cast<std::uint32_t>(actor.Main()));
            scene_actor.Write(0xecu, std::bit_cast<std::uint32_t>(10.0f));
            pal4::NativeActorSceneParameterRuntime scene_runtime;
            const auto look_at = pal4::BaseObjectSceneParameter::Refresh(
                scene_actor, scene_runtime);
            Check(std::fabs(look_at[0] - 2.0f) < 0.01f &&
                      std::fabs(look_at[1] - 21.0f) < 0.01f &&
                      std::fabs(look_at[2] - 4.0f) < 0.01f &&
                      scene_actor.Read(0x1f4u) ==
                          std::bit_cast<std::uint32_t>(look_at[0]) &&
                      scene_actor.Read(0x1f8u) ==
                          std::bit_cast<std::uint32_t>(look_at[1]),
                  "player scene parameter reads SDK Clump LTM and height");
            transform.UpdateClumpRotation({0.0f, 90.0f, 0.0f}, false,
                                          transform_runtime);
            frame = RwFrameGetLTM(RpClumpGetFrame(actor.Main()));
            Check(frame &&
                      std::bit_cast<float>(transform.words[41]) == 90.0f &&
                      std::fabs(frame->at.x) > 0.99f &&
                      std::fabs(frame->at.z) < 0.01f,
                  "rewritten player rotation updates SDK clump frame");
            for (const auto& [action, filename] : catalog.Entries()) {
                const bool played =
                    actor.SelectAction(action) && actor.Advance(0.016f);
                if (!played)
                    std::cerr << "Model " << model << " action " << action
                              << " from " << filename << " cannot play\n";
                Check(played && actor.CurrentAction() == action,
                      "play every original actor action through SDK HAnim");
            }
            Check(!actor.SelectAction("missing") &&
                      actor.SelectAction(catalog.Entries().back().first) &&
                      actor.Advance(0.016f) && !actor.Advance(-1.0f),
                  "switch and advance the original actor hierarchy");
            actor.Unload();
            Check(!actor.Main() && !actor.Shadow() && !actor.ShadowLight() && !actor.Hierarchy() &&
                      !actor.BottomEffectHook() &&
                      !actor.FaceMaterial() && !actor.LegMaterial() &&
                      actor.HiddenHookCount() == 0u &&
                      actor.ActionCount() == 0u,
                  "release the complete original actor resource graph");
        }
        pal4::NativeAnimatedActorAsset scene_npc;
        if (!scene_npc.Load(root, "201J"))
            std::cerr << "NPC model 201J: " << scene_npc.LastError() << '\n';
        Check(scene_npc.Main() && !scene_npc.Shadow() &&
                  scene_npc.Hierarchy() &&
                  scene_npc.ActionCount() != 0u &&
                  scene_npc.SelectAction("J01") &&
                  scene_npc.CurrentAction() == "J01",
              "original scene NPC model and default action use actor SDK owner");
        scene_npc.Unload();
        Check(!scene_npc.Main() && scene_npc.ActionCount() == 0u,
              "scene NPC actor resources release before SDK shutdown");
        for (const std::string_view model : {"421", "422", "423"}) {
            pal4::NativeAnimatedActorAsset monster;
            if (!monster.Load(root, model))
                std::cerr << "Monster model " << model << ": "
                          << monster.LastError() << '\n';
            Check(monster.Main() && monster.Hierarchy() &&
                      monster.ActionCount() > 0u &&
                      monster.Advance(0.016f),
                  "M06/3A monster model and first action decode through actor SDK owner");
            monster.Unload();
            Check(!monster.Main() && monster.ActionCount() == 0u,
                  "M06/3A monster actor resources release before SDK shutdown");
        }
        {
            pal4::NativeMonsterInfoReader reader;
            Check(reader.Load(root, "M06", "3A") &&
                      reader.Records().size() == 3u,
                  "read original M06/3A monster scene records");
            pal4::NativeBaseObjectMemory memory;
            pal4::test::RwDictSchemaFake schema;
            EmptyMonsterAnimationRuntime animation(memory);
            pal4::NativePlayerTransformRuntime transforms;
            pal4::NativeMonsterManagerOwner manager;
            Check(manager.Construct() != 0u,
                  "construct manager for original monster actors");
            pal4::NativeMonsterSceneObjectBuilder builder(
                root, manager, memory, schema, animation, transforms);
            Check(manager.InitializeScene(
                      reader.Records(),
                      [&](const pal4::NativeMonsterInfoRecord& record) {
                          builder.Build(record);
                      }) &&
                      manager.ObjectCount() == 3u &&
                      manager.FindObject(reader.Records()[0].name) != 0u &&
                      manager.FindObject(reader.Records()[1].name) != 0u &&
                      manager.FindObject(reader.Records()[2].name) != 0u,
                  "original M06/3A actors bind to three owned monster objects");
            for (const auto& record : reader.Records()) {
                const auto object = manager.FindObject(record.name);
                const auto label = memory.ReadDword(object + 4u);
                Check(label && std::string_view(reinterpret_cast<const char*>(label)) == record.name,
                    "scene monster publishes original instance name for encounter/return");
            }
            auto allocations_before_collision =
                manager.OutstandingAllocations();
            std::uint32_t active_monsters = 0u;
            std::uint32_t first_monster = 0u;
            manager.VisitObjects([&](const std::uint32_t monster) {
                if (first_monster == 0u) first_monster = monster;
                active_monsters += *reinterpret_cast<const std::uint32_t*>(
                    static_cast<std::uintptr_t>(monster + 0x1f0u)) != 0u;
                return true;
            });
            Check(active_monsters == 3u,
                  "all original M06/3A monsters enter the collision pass");
            {
                Check(pal4::NativeMonsterStorageCapture::CaptureScene(manager).empty(),
                      "original M06/3A disabled storage gates omit all three monster snapshots");
                pal4::NativeSceneGobAssets empty_objects;
                pal4::NativeMinimapManagerOwner minimap;
                Check(minimap.Initialize(root),"initialize minimap for live monster cache restore");
                const auto minimap_snapshot=minimap.CaptureSaveSnapshot();
                bool unloaded_rejected=false;
                try{(void)pal4::NativeArenaSaveCapture::Capture("M06","3A",empty_objects,manager,minimap);}
                catch(const std::logic_error&){unloaded_rejected=true;}
                Check(unloaded_rejected && manager.ObjectCount()==3u,
                    "live arena save rejects a missing GOB creating owner instead of inventing an empty scene");
                pal4::NativeArenaCacheSection section;
                pal4::NativeArenaCacheEntry entry;
                entry.names={"M06","3A"};
                entry.minimap_header=minimap_snapshot.header;
                entry.minimap_elements=minimap_snapshot.elements;
                for(const auto& record:reader.Records()) {
                    pal4::BinaryXmlNode snapshot{"soMonster",{{"valName",3u,record.name},
                        {"valVisible",1u,std::int32_t(0)}}};
                    const auto encoded=pal4::BinaryXmlConfig::Serialize(snapshot);
                    pal4::NativeArenaCacheChild child{11u,{}};
                    Check(encoded && child.document.Parse(*encoded),"encode original monster visibility snapshot");
                    entry.groups[1].push_back(std::move(child));
                }
                section.entries.push_back(std::move(entry));
                pal4::NativeGameSaveArchiveBuffer archive;
                pal4::NativeArenaCacheSaveSection writer(archive);
                Check(writer.Serialize(section),"write live monster scene cache fixture");
                pal4::NativeArenaCacheOwner cache;
                std::size_t consumed{};
                Check(cache.LoadSection(std::span(reinterpret_cast<const std::uint8_t*>(archive.Bytes().data()),
                    archive.Bytes().size()),consumed),"load live monster scene cache");
                Check(pal4::NativeArenaCacheSceneRestore::Restore(cache,"M06","3A",empty_objects,minimap,&manager)==1u,
                      "scene cache resolves actual original monster manager and owned components");
                unsigned gated{};
                for(const auto& record:reader.Records()) {
                    const auto object=manager.FindObject(record.name);
                    const auto* gate=record.buffer_cache.Root().FindAttribute("PAL4_MST_NEED_STORAGE");
                    const bool required=gate && static_cast<std::int32_t>(gate->ToInt(0u))>0;
                    gated+=required;
                    Check(memory.ReadDword(object+0x1f0u)==(required?0u:record.fields[0u]),
                          "actual monster scene restoration obeys its original buffer-cache gate");
                    manager.OwnedComponent(object).SetStateFlag(record.fields[0u]);
                }
                std::cout << "Original monster cache restored3 config-enabled=" << gated << '\n';
                bool released=false;
                try { (void)cache.Restore("M06","3A"); } catch(const std::logic_error&) { released=true; }
                Check(released,"scene cache releases borrowed monster manager callbacks");
            }
            pal4::NativeWorldAsset monster_world;
            if (!monster_world.LoadArenaSection(root, "Q01", "N01"))
                throw std::runtime_error("load original Q01/N01 test world: " +
                                         monster_world.LastError());
            pal4::NativeSceneMovementAsset monster_movement;
            VerifyCameraFrame(monster_world);
            Check(monster_movement.Load(root, "Q01", "N01") &&
                      monster_movement.Floor() != nullptr,
                  "load original Q01/N01 movement floor");
            std::array<std::uint32_t, 128> monster_arena{};
            monster_arena[4u] = reinterpret_cast<std::uint32_t>(
                monster_world.World());
            monster_arena[12u] = 1u;
            pal4::NativeArenaCollisionTree monster_collision;
            const auto monster_arena_address =
                reinterpret_cast<std::uint32_t>(monster_arena.data());
            Check(monster_collision.Build(monster_arena_address,
                                          monster_movement.Floor()) &&
                      monster_collision.FaceCount() > 0,
                  "build original Q01/N01 test collision tree");
            struct MonsterEffectDirectory {
                std::filesystem::path old=std::filesystem::current_path();
                ~MonsterEffectDirectory(){std::filesystem::current_path(old);}
            } monster_effect_directory;
            std::filesystem::current_path(root);
            pal4::NativePkFileManagerRuntime monster_packages;
            std::array<std::uint32_t,20> monster_filesystem{};monster_filesystem[10]=0x1010u;
            monster_packages.ProcessPackFiles(monster_packages.InitializePackFileManager(),
                reinterpret_cast<std::uint32_t>(monster_filesystem.data()));
            Check(monster_packages.LoadAllScripts()!=0u,"monster scene effect packages");
            pal4::test::NativeParticleProbeCamera monster_effect_camera;
            monster_effect_camera.Begin();(void)monster_effect_camera.EndPixels();
            pal4::NativeEffectSceneSession monster_effects(root,monster_packages);
            monster_effects.BindCamera(reinterpret_cast<std::uint32_t>(monster_effect_camera.camera));
            monster_effects.BindArena(monster_arena_address);
            pal4::NativeArenaNpcSceneEntry monster_entry;
            monster_entry.BindAttachedEffectRuntime(&monster_effects.AttachedEffects());
            Check(monster_entry.BindMonsters(manager, monster_arena_address,
                                             monster_movement,
                                             monster_collision) == 3u &&
                      monster_entry.Count() == 3u,
                  "enter three original monsters into the native arena adapter");
            manager.VisitObjects([&](const std::uint32_t monster) {
                Check(*reinterpret_cast<const std::uint32_t*>(
                          static_cast<std::uintptr_t>(monster + 0x568u)) != 0u &&
                          RpClumpGetWorld(reinterpret_cast<RpClump*>(
                              *reinterpret_cast<const std::uint32_t*>(
                                  static_cast<std::uintptr_t>(monster + 0x14u)))) ==
                              monster_world.World(),
                      "M06/3A monster owns a collision tree and world clump");
                return true;
            });
            Check(monster_movement.RegisteredObjectAtomics() > 0u,
                  "M06/3A monster clumps register with Movement");
            first_monster = manager.FindObject(reader.Records()[0u].name);
            std::array<std::uint32_t, 185> player_control{};
            pal4::NativeMonsterPursuitPathRuntime pursuit(
                manager, monster_collision, memory,
                reinterpret_cast<std::uint32_t>(player_control.data()));
            auto& monster_state = *reinterpret_cast<
                pal4::MonsterBehaviorManagerState*>(first_monster);
            auto& movement_state = *reinterpret_cast<pal4::CollisionActorState*>(
                first_monster + 0x564u);
            const auto& face = *reinterpret_cast<pal4::D3dTriangleFaceState*>(
                monster_collision.FaceAt(0u));
            std::array<std::uint32_t, 128> target_actor{};
            target_actor[0x1f0u / 4u] = 1u;
            std::array<std::uint32_t, 3> prior_position{};
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                const auto center = pal4::D3dTriangleFace::FloatAt(
                    face, 64u + axis * 4u);
                prior_position[axis] = monster_state.words[0xacu / 4u + axis];
                monster_state.words[0xacu / 4u + axis] =
                    std::bit_cast<std::uint32_t>(center);
                monster_state.words[0x6e8u / 4u + axis] =
                    std::bit_cast<std::uint32_t>(center);
                target_actor[0xacu / 4u + axis] =
                    std::bit_cast<std::uint32_t>(center * 0.9f +
                        pal4::D3dTriangleFace::FloatAt(
                            face, 28u + axis * 4u) * 0.1f);
            }
            const auto target_address = reinterpret_cast<std::uint32_t>(
                target_actor.data());
            const auto actor_allocations = memory.OutstandingAllocations();
            Check(pal4::MonsterPursuitCollisionSpeed::Process(
                      monster_state, first_monster, 0.1f, target_address,
                      pursuit) &&
                      pal4::CollisionActor::PathEnabled(movement_state) != 0u &&
                      movement_state.words[21u] != 0u &&
                      memory.OutstandingAllocations() > actor_allocations,
                  "monster pursuit writes a real path into its owned CollisionActor");
            const auto speed = pursuit.MaxPursuitSpeed(
                monster_state.words[0x654u / 4u], 50.0f);
            Check(std::bit_cast<float>(movement_state.words[9u]) == speed &&
                      speed > 0.0f,
                  "monster pursuit applies the original BufferCache speed");
            monster_state.words[0x6e0u / 4u] = 1u;
            monster_state.words[0x6e4u / 4u] = target_address;
            (void)manager.ProcessCollisions(0.1f, &pursuit);
            Check(monster_state.words[0x6e0u / 4u] == 1u,
                  "manager collision pass routes chase through the native path adapter");
            player_control[184u] = 1u;
            (void)manager.ProcessCollisions(0.1f, &pursuit);
            Check(monster_state.words[0x6e0u / 4u] == 2u &&
                      monster_state.words[0x6e4u / 4u] == 0u,
                  "player control flag switches a pursuing monster to its return path");
            player_control[184u] = 0u;
            target_actor[0x1f0u / 4u] = 0u;
            const auto path_node = movement_state.words[23u];
            Check(!pal4::MonsterPursuitCollisionSpeed::Process(
                      monster_state, first_monster, 0.1f, target_address,
                      pursuit) && movement_state.words[23u] == path_node,
                  "inactive pursuit target leaves the current path untouched");
            monster_state.words[0x6f8u / 4u] = 123u;
            monster_state.words[0x6e4u / 4u] = target_address;
            (void)pal4::MonsterPursuitReturnState::Enter(
                monster_state, first_monster, 0.1f, pursuit);
            Check(monster_state.words[0x6e0u / 4u] == 2u &&
                      monster_state.words[0x6e4u / 4u] == 0u &&
                      monster_state.words[0x6fcu / 4u] == 123u &&
                      pal4::CollisionActor::PathEnabled(movement_state) != 0u,
                  "monster return reuses its saved position and native path");
            const auto records = memory.ReadDword(first_monster + 0x178u);
            std::uint32_t moving_record = 0u;
            for (std::uint32_t index = 0u; index < memory.ReadDword(records - 4u);
                 ++index) {
                const auto record = records + index *
                    sizeof(pal4::GameObjectAnimationRecordState);
                const auto* name = reinterpret_cast<const char*>(
                    memory.ReadDword(record + 20u));
                if (std::string_view(name) != "C01" &&
                    std::string_view(name) != "Z01") {
                    moving_record = record;
                    break;
                }
            }
            Check(moving_record != 0u, "original monster has a non-idle action");
            auto* monster_clump = reinterpret_cast<RpClump*>(
                memory.ReadDword(first_monster + 0x14u));
            RpHAnimHierarchy* monster_hierarchy = nullptr;
            (void)RwFrameForAllChildren(RpClumpGetFrame(monster_clump),
                                        FindHierarchy, &monster_hierarchy);
            if (!monster_hierarchy)
                monster_hierarchy = RpHAnimFrameGetHierarchy(
                    RpClumpGetFrame(monster_clump));
            Check(monster_hierarchy && RtAnimInterpolatorSetCurrentAnim(
                      monster_hierarchy->currentAnim,
                      reinterpret_cast<RtAnimAnimation*>(
                          memory.ReadDword(moving_record + 32u))),
                  "select a non-idle native monster clip before reset");
            // Exercise the shared position stage independently of the still
            // unbound monster behavior scheduler. All movement and HAnim work
            // below uses the actual original-resource object.
            target_actor[0x1f0u / 4u] = 1u;
            Check(pal4::MonsterPursuitCollisionSpeed::Process(
                      monster_state, first_monster, 0.1f, target_address, pursuit),
                  "restore a nonzero monster chase path for frame stepping");
            auto& monster_base = *reinterpret_cast<pal4::BaseObjectState*>(
                first_monster);
            const auto position = [&] {
                return std::array<float, 3>{
                    std::bit_cast<float>(movement_state.words[3u]),
                    std::bit_cast<float>(movement_state.words[4u]),
                    std::bit_cast<float>(movement_state.words[5u])};
            };
            const auto distance = [](const auto& a, const auto& b) {
                float squared = 0.0f;
                for (std::size_t axis = 0u; axis < 3u; ++axis)
                    squared += (a[axis] - b[axis]) * (a[axis] - b[axis]);
                return std::sqrt(squared);
            };
            const auto start_position = position();
            std::uint32_t motion_steps = 0u;
            std::uint32_t animation_steps = 0u;
            bool moved = false;
            const auto step_motion = [&] {
                const auto before = position();
                const auto result = pal4::NativeBaseObjectMotionFrame::Update(
                    monster_base, 0.01f, 0u, monster_collision, transforms,
                    memory, schema, [&](float delta) {
                        Check(RtAnimInterpolatorAddAnimTime(
                                  monster_hierarchy->currentAnim, delta) &&
                                  RpHAnimHierarchyUpdateMatrices(monster_hierarchy),
                              "advance real monster HAnim during position frame");
                        ++animation_steps;
                    });
                ++motion_steps;
                Check(result.resource_updates == 1u &&
                          result.animation_updates == 1u &&
                          result.collision_updates == 1u,
                      "shared monster position frame runs each native stage");
                const auto after = position();
                Check(distance(before, after) <= speed * 0.01f + 0.02f,
                      "monster displacement respects its speed and simulation delta");
                if (distance(before, after) > 0.02f) {
                    moved = true;
                    const auto* frame = RwFrameGetLTM(
                        RpClumpGetFrame(monster_clump));
                    Check(distance(after, std::array<float, 3>{
                              frame->pos.x, frame->pos.y, frame->pos.z}) < 0.02f,
                          "monster clump follows its collision actor position");
                    const auto yaw = std::bit_cast<float>(
                        monster_base.Read(0xa4u)) * 0.01745329252f;
                    const auto dx = after[0u] - before[0u];
                    const auto dz = after[2u] - before[2u];
                    const auto horizontal = std::sqrt(dx * dx + dz * dz);
                    Check(horizontal > 0.0f &&
                              (std::sin(yaw) * dx + std::cos(yaw) * dz) /
                                  horizontal > 0.9f,
                          "monster yaw faces its actual motion");
                }
            };
            for (std::uint32_t step = 0u; step < 4096u &&
                 pal4::CollisionActor::PathEnabled(movement_state) != 0u; ++step)
                step_motion();
            const std::array<float, 3> target_position{
                std::bit_cast<float>(target_actor[0xacu / 4u]),
                std::bit_cast<float>(target_actor[0xb0u / 4u]),
                std::bit_cast<float>(target_actor[0xb4u / 4u])};
            Check(moved && distance(start_position, position()) > 0.02f &&
                      pal4::CollisionActor::PathEnabled(movement_state) == 0u &&
                      distance(position(), target_position) < 0.02f &&
                      animation_steps == motion_steps,
                  "monster reaches the real path target within bounded frames");
            const auto reached_position = position();
            step_motion();
            Check(distance(reached_position, position()) < 0.001f,
                  "monster remains stopped after path completion");
            std::cout << "Monster native position frames=" << motion_steps
                      << " speed=" << speed << " arrival_error="
                      << distance(position(), target_position) << '\n';
            memory.WriteDword(first_monster + 0x17cu, moving_record);
            for (const auto index : {10u, 15u, 18u, 19u})
                memory.WriteDword(first_monster + 0x170u + index * 4u, 1u);
            monster_state.words[0x708u / 4u] = 123u;
            monster_state.words[0x70cu / 4u] = 456u;
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                monster_state.words[0xacu / 4u + axis] =
                    std::bit_cast<std::uint32_t>(1000000.0f);
                monster_state.words[0xa0u / 4u + axis] =
                    std::bit_cast<std::uint32_t>(33.0f);
            }
            auto& script = *reinterpret_cast<pal4::GomTaskContainerState*>(
                first_monster + pal4::MonsterBehaviorManager::kScriptOffset);
            const auto empty_task_node = memory.Allocate(12u);
            memory.WriteDword(empty_task_node, script.sentinel);
            memory.WriteDword(empty_task_node + 4u, script.sentinel);
            memory.WriteDword(empty_task_node + 8u, 0u);
            memory.WriteDword(script.sentinel, empty_task_node);
            memory.WriteDword(script.sentinel + 4u, empty_task_node);
            script.count = 1u;
            script.ready = 0u;
            (void)pal4::CollisionActor::StopMovement(movement_state);
            RejectedReturnPath rejected_return(pursuit);
            (void)pal4::MonsterPursuitReturnState::Enter(
                monster_state, first_monster, 0.1f, rejected_return);
            Check(rejected_return.rejections == 1u && rejected_return.resets == 1u,
                  "rejected return request dispatches the owned native reset");
            const auto idle_record = memory.ReadDword(first_monster + 0x17cu);
            const std::string_view idle_name(reinterpret_cast<const char*>(
                memory.ReadDword(idle_record + 20u)));
            Check(monster_state.words[0x6e0u / 4u] == 0u &&
                      monster_state.words[0x708u / 4u] == 0u &&
                      monster_state.words[0x70cu / 4u] == 0u &&
                      (idle_name == "C01" || idle_name == "Z01") &&
                      monster_hierarchy->currentAnim->pCurrentAnim ==
                          reinterpret_cast<RtAnimAnimation*>(
                              memory.ReadDword(idle_record + 32u)) &&
                      script.count == 0u && script.ready == 1u &&
                      memory.ReadDword(script.sentinel) == script.sentinel,
                  "rejected return path restores native idle animation and clears script tasks");
            for (const auto index : {10u, 15u, 18u, 19u})
                Check(memory.ReadDword(first_monster + 0x170u + index * 4u) == 0u,
                      "monster reset uses the rewritten direct animation switch");
            for (std::uint32_t axis = 0u; axis < 3u; ++axis)
                Check(monster_state.words[0xacu / 4u + axis] == prior_position[axis] &&
                          monster_state.words[0xa0u / 4u + axis] ==
                              reader.Records()[0u].rotation_bits[axis],
                      "monster reset restores original info position and rotation");
            const auto* restored_frame = RwFrameGetLTM(RpClumpGetFrame(monster_clump));
            Check(restored_frame && std::fabs(restored_frame->pos.x -
                      std::bit_cast<float>(prior_position[0u])) < 0.01f &&
                      std::fabs(restored_frame->pos.z -
                      std::bit_cast<float>(prior_position[2u])) < 0.01f &&
                      (monster_state.words[(0x658u + 24u) / 4u] & 0xffu) == 1u,
                  "monster reset updates the real clump frame and restores event processing");
            const auto after_reset = memory.OutstandingAllocations();
            for (std::uint32_t repeat = 0u; repeat < 3u; ++repeat)
                (void)manager.ResetOwnedPursuit(first_monster);
            Check(memory.OutstandingAllocations() == after_reset,
                  "repeated monster behavior reload has no allocation growth");
            pal4::GameMathTables proximity_math;
            (void)proximity_math.CalculateSquareRootTable();
            pal4::NativeMonsterProximityRuntime proximity(
                memory, reinterpret_cast<std::uint32_t>(player_control.data()),
                monster_movement, transforms, proximity_math);
            Check(proximity.ControlledActor().address == 0u &&
                      !pal4::MonsterProximityChecker::Check(
                          monster_state, proximity),
                  "native proximity rejects an absent controlled actor");
            const auto second_monster = manager.FindObject(
                reader.Records()[1u].name);
            player_control[1u] = second_monster;
            auto& target_resource =
                *reinterpret_cast<pal4::GameResourceManager*>(second_monster);
            auto& monster_resource =
                *reinterpret_cast<pal4::GameResourceManager*>(first_monster);
            (void)monster_resource.UpdateClumpTransform(
                {0.0f, 100000.0f, 0.0f}, false, transforms);
            (void)target_resource.UpdateClumpTransform(
                {1.0f, 100000.0f, 0.0f}, false, transforms);
            const auto controlled = proximity.ControlledActor();
            std::cout << "Native monster half-height=" << controlled.half_height
                      << " radius=" << proximity.DetectionRadius(
                          monster_state.words[0x654u / 4u]) << '\n';
            Check(controlled.address == second_monster && controlled.active &&
                      controlled.position[0u] == 1.0f &&
                      controlled.position[1u] == 100000.0f &&
                      controlled.half_height > 0.0f &&
                      controlled.collision_position[0u] == std::bit_cast<float>(
                          memory.ReadDword(second_monster + 0xe0u)) &&
                      proximity.DetectionRadius(
                          monster_state.words[0x654u / 4u]) > 1.0f &&
                      proximity.PlanarDistance(3.0f, 4.0f) ==
                          pal4::GameVectorMath::ApproximateMagnitude(
                              {3.0f, 0.0f, 4.0f}, proximity_math),
                  "native proximity reads the actual controlled object and original radius");
            Check(pal4::MonsterProximityChecker::Check(monster_state, proximity) &&
                      monster_state.words[0x6e4u / 4u] == second_monster &&
                      monster_state.words[0x6ecu / 4u] ==
                          std::bit_cast<std::uint32_t>(100000.0f),
                  "native proximity stores target and encounter position");
            memory.WriteDword(second_monster + 0x1f0u, 0u);
            Check(!pal4::MonsterProximityChecker::Check(monster_state, proximity),
                  "native proximity rejects the actual inactive target");
            memory.WriteDword(second_monster + 0x1f0u, 1u);
            (void)target_resource.UpdateClumpTransform(
                {100000.0f, 100000.0f, 0.0f}, false, transforms);
            Check(!pal4::MonsterProximityChecker::Check(monster_state, proximity),
                  "native proximity rejects a target beyond its detection radius");
            (void)target_resource.UpdateClumpTransform(
                {1.0f, 110000.0f, 0.0f}, false, transforms);
            Check(!pal4::MonsterProximityChecker::Check(monster_state, proximity),
                  "native proximity rejects a target outside vertical reach");
            WallProbe wall_probe;
            Check(monster_movement.Wall() != nullptr,
                  "original Q01/N01 has a wall collision clump");
            (void)RpClumpForAllAtomics(monster_movement.Wall(), FindWallProbe,
                                      &wall_probe);
            Check(wall_probe.found && proximity.CollisionBlocked(
                      wall_probe.ray, proximity.CollisionClump()) &&
                      !proximity.CollisionBlocked(
                          {0.0f, 100000.0f, 0.0f, 1.0f, 100000.0f, 0.0f},
                          proximity.CollisionClump()),
                  "native proximity detects an original wall triangle and a clear ray");
            const auto place_collision_center = [&](std::uint32_t object,
                                                    std::size_t ray_offset) {
                pal4::ResourceVec3 destination{};
                float* axes[] = {&destination.x, &destination.y, &destination.z};
                for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                    const auto offset = std::bit_cast<float>(
                        memory.ReadDword(object + 0xe0u + axis * 4u)) -
                        std::bit_cast<float>(
                            memory.ReadDword(object + 0xacu + axis * 4u));
                    *axes[axis] = wall_probe.ray[ray_offset + axis] - offset;
                }
                (void)reinterpret_cast<pal4::GameResourceManager*>(object)->
                    UpdateClumpTransform(destination, false, transforms);
            };
            place_collision_center(first_monster, 0u);
            place_collision_center(second_monster, 3u);
            const auto occluded_actor = proximity.ControlledActor();
            Check(proximity.PlanarDistance(
                      occluded_actor.position[0u] -
                          std::bit_cast<float>(monster_state.words[0xacu / 4u]),
                      occluded_actor.position[2u] -
                          std::bit_cast<float>(monster_state.words[0xb4u / 4u])) <=
                          proximity.DetectionRadius(
                              monster_state.words[0x654u / 4u]) &&
                      std::fabs(occluded_actor.position[1u] -
                          std::bit_cast<float>(monster_state.words[0xb0u / 4u])) <
                          occluded_actor.half_height * 2.0f,
                  "occluded native target passes the distance and height gates");
            Check(!pal4::MonsterProximityChecker::Check(monster_state, proximity) &&
                      monster_state.words[0x6e4u / 4u] == 0u,
                  "original wall blocks the full native monster detection path");
            (void)monster_resource.UpdateClumpTransform(
                {0.0f, 100000.0f, 0.0f}, false, transforms);
            (void)target_resource.UpdateClumpTransform(
                {1.0f, 100000.0f, 0.0f}, false, transforms);
            Check(pal4::MonsterProximityChecker::Check(monster_state, proximity),
                  "full native proximity resumes after moving clear of the wall");
            (void)monster_resource.UpdateClumpTransform(
                {0.01f, 100000.0f, 0.0f}, false, transforms);
            Check(pal4::MonsterProximityChecker::CheckReturnTargetReached(
                      monster_state, first_monster, proximity) &&
                      monster_state.words[0xacu / 4u] == 0u &&
                      RwFrameGetLTM(RpClumpGetFrame(monster_clump))->pos.x == 0.0f,
                  "native return arrival snaps the real object and clump to its saved position");
            (void)monster_resource.UpdateClumpTransform(
                {1001.0f, 100000.0f, 0.0f}, false, transforms);
            Check(pal4::MonsterProximityChecker::BeyondReturnThreshold(
                      monster_state, proximity),
                  "native return threshold uses the existing approximate distance");
            player_control[1u] = 0u;
            (void)manager.ResetOwnedPursuit(first_monster);
            (void)manager.ResetOwnedPursuit(second_monster);
            std::cout << "PASS: native monster proximity, wall visibility and return arrival\n";
            const auto third_monster = manager.FindObject(reader.Records()[2u].name);
            auto& third_state = *reinterpret_cast<pal4::MonsterBehaviorManagerState*>(
                third_monster);
            auto& third_resource = *reinterpret_cast<pal4::GameResourceManager*>(
                third_monster);
            (void)monster_resource.UpdateClumpTransform(
                {0.0f, 100000.0f, 0.0f}, false, transforms);
            (void)target_resource.UpdateClumpTransform(
                {100000.0f, 100000.0f, 0.0f}, false, transforms);
            (void)third_resource.UpdateClumpTransform(
                {1.0f, 100000.0f, 0.0f}, false, transforms);
            monster_state.words[0x6e0u / 4u] = 1u;
            monster_state.words[0x6e4u / 4u] = second_monster;
            manager.OwnedComponent(third_monster).Runtime().Encounters().AddName(third_state, "test");
            const auto history = third_state.words[0x724u / 4u];
            pal4::NativeMonsterActionDispatchRuntime group_action(
                manager, memory, proximity);
            Check(group_action.InteractionRadius(
                      monster_state.words[0x654u / 4u]) > 1.0f,
                  "original source has a group pursuit radius");
            Check(pal4::MonsterActionDispatcher::Process(
                      first_monster, group_action) != 0u &&
                      third_state.words[0x6e0u / 4u] == 1u &&
                      third_state.words[0x6e4u / 4u] == second_monster &&
                      third_state.words[0x6e8u / 4u] ==
                          std::bit_cast<std::uint32_t>(1.0f) &&
                      third_state.words[0x728u / 4u] == history &&
                      memory.ReadDword(history + 4u) == 0u &&
                      memory.ReadDword(history + 8u) == 0u &&
                      memory.ReadDword(history + 12u) == 0u &&
                      (third_state.words[(0x658u + 24u) / 4u] & 0xffu) == 0u &&
                      memory.ReadDword(second_monster + 0x6e0u) == 0u,
                  "native group dispatch enters nearby pursuit and clears real history/events");
            const auto group_record = memory.ReadDword(third_monster + 0x17cu);
            const std::string_view group_action_name(reinterpret_cast<const char*>(
                memory.ReadDword(group_record + 20u)));
            auto* group_clump = reinterpret_cast<RpClump*>(
                memory.ReadDword(third_monster + 0x14u));
            RpHAnimHierarchy* group_hierarchy = nullptr;
            (void)RwFrameForAllChildren(RpClumpGetFrame(group_clump),
                                        FindHierarchy, &group_hierarchy);
            if (!group_hierarchy)
                group_hierarchy = RpHAnimFrameGetHierarchy(RpClumpGetFrame(group_clump));
            Check((group_action_name == "Z02" || group_action_name == "C03") &&
                      group_hierarchy &&
                      group_hierarchy->currentAnim->pCurrentAnim ==
                          reinterpret_cast<RtAnimAnimation*>(
                              memory.ReadDword(group_record + 32u)),
                  "native group pursuit switches both animation record and HAnim");
            const auto group_allocations = memory.OutstandingAllocations();
            (void)pal4::MonsterActionDispatcher::Process(first_monster, group_action);
            Check(memory.OutstandingAllocations() == group_allocations &&
                      memory.ReadDword(third_monster + 0x17cu) == group_record,
                  "repeated group pursuit preserves active state and allocations");
            (void)manager.ResetOwnedPursuit(first_monster);
            (void)manager.ResetOwnedPursuit(second_monster);
            (void)manager.ResetOwnedPursuit(third_monster);
            std::cout << "PASS: native monster group pursuit entry and animation\n";
            pal4::NativeMonsterArenaSkillFrame skill_frame(
                memory, manager, proximity_math);
            std::array<std::uint32_t, 14u> skill_manager{};
            std::array<std::uint32_t, 11u> skill_record{};
            std::array<std::uint32_t, 1u> skill_records{
                reinterpret_cast<std::uint32_t>(skill_record.data())};
            const auto skill_manager_address = reinterpret_cast<std::uint32_t>(
                skill_manager.data());
            const auto skill_array = reinterpret_cast<std::uint32_t>(
                skill_records.data());
            Check(skill_frame.Visit(skill_manager_address, first_monster) == 0u,
                  "empty native arena skill list keeps the original end result");
            skill_manager[1u] = skill_array;
            skill_manager[2u] = skill_array + 4u;
            skill_manager[3u] = skill_array + 4u;
            auto place_skill = [&](std::uint32_t kind, float dx, float dy) {
                skill_record[0u] = 1u;
                skill_record[1u] = kind;
                skill_record[2u] = std::bit_cast<std::uint32_t>(
                    std::bit_cast<float>(monster_state.words[0xacu / 4u]) + dx);
                skill_record[3u] = std::bit_cast<std::uint32_t>(
                    std::bit_cast<float>(monster_state.words[0xb0u / 4u]) + dy);
                skill_record[4u] = monster_state.words[0xb4u / 4u];
            };
            place_skill(2u, 10.0f, 0.0f);
            Check(skill_frame.Visit(skill_manager_address, first_monster) ==
                      skill_array + 4u && skill_record[0u] == 1u &&
                      monster_state.words[0x730u / 4u] == 2u &&
                      monster_state.words[0x734u / 4u] ==
                          std::bit_cast<std::uint32_t>(10.0f),
                  "native enthrall record sets ten-second monster status without consumption");
            monster_state.words[0x738u / 4u] = 123u;
            place_skill(2u, 0.0f, 200.0f);
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            Check(monster_state.words[0x738u / 4u] == 123u,
                  "skill radius uses three-dimensional distance");
            place_skill(1u, 10000.0f, 0.0f);
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            Check(monster_state.words[0x730u / 4u] == 0u &&
                      monster_state.words[0x734u / 4u] == 0u &&
                      monster_state.words[0x738u / 4u] == 0u,
                  "other active skill kinds clear monster status even when distant");
            Check(manager.SelectOwnedAnimation(first_monster, "Z02") ||
                      manager.SelectOwnedAnimation(first_monster, "C03"),
                  "native skill fixture selects a moving clip");
            movement_state.words[10u] = 0x123401u;
            for (const auto index : {6u, 7u, 8u})
                movement_state.words[index] = std::bit_cast<std::uint32_t>(9.0f);
            place_skill(0u, 50.0f, 0.0f);
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            Check(skill_record[0u] == 1u &&
                      pal4::CollisionActor::PathEnabled(movement_state) != 0u,
                  "snare outside forty units does not stop the monster");
            place_skill(0u, 10.0f, 0.0f);
            const auto skill_allocations = memory.OutstandingAllocations();
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            const auto snared_record = memory.ReadDword(first_monster + 0x17cu);
            const auto snared_name = skill_frame.AnimationName(first_monster + 0x170u);
            Check(skill_record[0u] == 0u &&
                      monster_state.words[0x730u / 4u] == 1u &&
                      monster_state.words[0x734u / 4u] ==
                          std::bit_cast<std::uint32_t>(5.0f) &&
                      monster_state.words[0x738u / 4u] == 0u &&
                      movement_state.words[10u] == 0x123400u &&
                      movement_state.words[6u] == 0u &&
                      movement_state.words[7u] == 0u &&
                      movement_state.words[8u] == 0u &&
                      (snared_name == "C01" || snared_name == "Z01") &&
                      monster_hierarchy->currentAnim->pCurrentAnim ==
                          reinterpret_cast<RtAnimAnimation*>(
                              memory.ReadDword(snared_record + 32u)),
                  "native snare consumes shared record, stops movement and switches actual HAnim to idle");
            const auto second_status = memory.ReadDword(second_monster + 0x730u);
            (void)skill_frame.Visit(skill_manager_address, second_monster);
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            Check(memory.ReadDword(second_monster + 0x730u) == second_status &&
                      memory.ReadDword(first_monster + 0x17cu) == snared_record &&
                      memory.OutstandingAllocations() == skill_allocations,
                  "consumed snare cannot retrigger on another monster or allocate each frame");
            place_skill(0u, 0.0f, 0.0f);
            memory.WriteDword(first_monster + 0x170u + 40u, 777u);
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            Check(memory.ReadDword(first_monster + 0x170u + 40u) == 777u,
                  "already idle snared monster keeps animation progress");
            place_skill(1u, 0.0f, 0.0f);
            (void)skill_frame.Visit(skill_manager_address, first_monster);
            (void)manager.ResetOwnedPursuit(first_monster);
            std::cout << "PASS: native arena skill traversal, snare stop and actual idle animation\n";
            // Compose 0x433850 with actual path, movement, owner cleanup,
            // group action, proximity and skill callbacks. Encounter scripts
            // are explicitly gated by the original +0x2DC control field.
            player_control[1u] = second_monster;
            player_control[0x2dcu / 4u] = 1u;
            player_control[0x2e0u / 4u] = 0u;
            BusyInteractionFixture interaction(player_control.data());
            UnboundEncounterFixture encounter;
            std::uint32_t skill_visits = 0u;
            bool reject_skill_manager = false;
            pal4::NativeMonsterPursuitFrame pursuit_frame(
                memory, manager, proximity, pursuit, group_action, skill_frame,
                transforms, interaction, encounter, [&] {
                    if (reject_skill_manager)
                        throw std::logic_error("fixture arena skill manager unavailable");
                    ++skill_visits;
                    return skill_manager_address;
                });
            (void)pal4::CollisionActor::StopMovement(movement_state);
            (void)pal4::NativeBaseObjectMotionFrame::Update(
                monster_base, 0.0f, 0u, monster_collision, transforms, memory,
                schema, [](float) {});
            const auto frame_origin = position();
            (void)monster_resource.UpdateClumpTransform(
                {frame_origin[0u], frame_origin[1u], frame_origin[2u]}, false, transforms);
            (void)target_resource.UpdateClumpTransform(
                {start_position[0u], start_position[1u], start_position[2u]}, false, transforms);
            monster_state.words[0x704u / 4u] = 0u;
            monster_state.words[0x6f8u / 4u] = std::bit_cast<std::uint32_t>(0.1f);
            Check(memory.ReadDword(first_monster + 0x724u) == 0u,
                  "pursuit frame fixture starts without a history allocation");
            manager.OwnedComponent(first_monster).Runtime().Encounters().AddName(monster_state, "test");
            const auto frame_history = monster_state.words[0x724u / 4u];
            Check(pursuit_frame.Update(first_monster, 0.01f) &&
                      monster_state.words[0x6e0u / 4u] == 1u &&
                      monster_state.words[0x6e4u / 4u] == second_monster &&
                      monster_state.words[0x728u / 4u] == frame_history &&
                      memory.ReadDword(frame_history + 4u) == 0u &&
                      memory.ReadDword(frame_history + 8u) == 0u &&
                      memory.ReadDword(frame_history + 12u) == 0u,
                  "composed native pursuit discovers actual target and releases history string");
            auto advance_frame = [&] {
                const auto active = pursuit_frame.Update(first_monster, 0.01f);
                (void)pal4::NativeBaseObjectMotionFrame::Update(
                    monster_base, 0.01f, 0u, monster_collision, transforms, memory,
                    schema, [&](float delta) {
                        Check(RtAnimInterpolatorAddAnimTime(
                                  monster_hierarchy->currentAnim, delta) &&
                                  RpHAnimHierarchyUpdateMatrices(monster_hierarchy),
                              "composed pursuit advances actual HAnim");
                    });
                return active;
            };
            bool frame_moved = false;
            for (std::uint32_t step = 0u; step < 32u; ++step) {
                (void)advance_frame();
                frame_moved = frame_moved || distance(frame_origin, position()) > 0.02f;
                if (frame_moved && !pal4::CollisionActor::PathEnabled(movement_state)) break;
            }
            Check(frame_moved && interaction.checks > 0u &&
                      monster_state.words[0x6e0u / 4u] == 1u,
                  "composed native pursuit applies path and moves while interaction is busy");
            monster_state.words[0x704u / 4u] = 1u;
            Check(advance_frame() && monster_state.words[0x6e0u / 4u] == 2u &&
                      monster_state.words[0x6e4u / 4u] == 0u,
                  "composed native pursuit stop flag requests a real return path");
            for (std::uint32_t step = 0u; step < 256u; ++step)
                if (!advance_frame()) break;
            std::cout << "Composed pursuit state=" << monster_state.words[0x6e0u / 4u]
                      << " return_error=" << distance(frame_origin, position())
                      << " timers=" << monster_state.words[0x708u / 4u] << ','
                      << monster_state.words[0x70cu / 4u]
                      << " skill_visits=" << skill_visits
                      << " allocations=" << memory.OutstandingAllocations() << '\n';
            Check(monster_state.words[0x6e0u / 4u] == 0u &&
                      distance(frame_origin, position()) < 0.02f &&
                      monster_state.words[0x708u / 4u] == 0u &&
                      monster_state.words[0x70cu / 4u] == 0u &&
                      skill_visits > 3u &&
                      memory.ReadDword(first_monster + 0x658u + 8u) ==
                          memory.ReadDword(first_monster + 0x658u + 4u) + 4u,
                  "composed native pursuit returns to saved position, visits skills each frame and restores fuzzy events");
            const auto completed_frame_allocations = memory.OutstandingAllocations();
            for (std::uint32_t cycle = 0u; cycle < 3u; ++cycle) {
                monster_state.words[0x704u / 4u] = 0u;
                Check(advance_frame(), "repeated native cycle reacquires the target");
                for (std::uint32_t step = 0u; step < 32u; ++step) {
                    (void)advance_frame();
                    if (!pal4::CollisionActor::PathEnabled(movement_state)) break;
                }
                monster_state.words[0x704u / 4u] = 1u;
                Check(advance_frame(), "repeated native cycle begins return");
                for (std::uint32_t step = 0u; step < 256u; ++step)
                    if (!advance_frame()) break;
                Check(monster_state.words[0x6e0u / 4u] == 0u &&
                          distance(frame_origin, position()) < 0.02f &&
                          memory.OutstandingAllocations() == completed_frame_allocations,
                      "completed native pursuit cycles retain stable allocations");
            }
            reject_skill_manager = true;
            bool rejected_skill_manager = false;
            try {
                (void)pursuit_frame.Update(first_monster, 0.01f);
            } catch (const std::logic_error& error) {
                rejected_skill_manager = std::string_view(error.what()) ==
                    "fixture arena skill manager unavailable";
            }
            reject_skill_manager = false;
            Check(rejected_skill_manager &&
                      !pursuit_frame.Update(first_monster, 0.01f),
                  "missing arena service fails explicitly and frame scope recovers after exception");
            player_control[0x2dcu / 4u] = 0u;
            player_control[1u] = 0u;
            monster_state.words[0x704u / 4u] = 0u;
            (void)manager.ResetOwnedPursuit(first_monster);
            (void)manager.ResetOwnedPursuit(second_monster);
            (void)manager.ResetOwnedPursuit(third_monster);
            std::cout << "PASS: composed native discovery, chase frames and return\n";
            auto& first_behavior = manager.OwnedComponent(first_monster).Runtime();
            UnboundBehaviorTaskFixture task_factory;
            pal4::NativeBehaviorAtomicDispatchRuntime atomic_dispatch(
                first_behavior.EventDocuments(), first_behavior.Tasks(), task_factory);
            pal4::NativeScriptEventSchedulerRuntime scheduler(
                memory, first_behavior.EventDocuments(), first_behavior.Tasks(),
                atomic_dispatch);
            pal4::IScriptEventSchedulerRuntime* active_scheduler = &scheduler;
            std::uint32_t scheduler_requests = 0u;
            std::uint32_t outer_animation_steps = 0u;
            pal4::NativeMonsterWorldFrame world_frame(
                manager, pursuit_frame, pursuit, monster_collision, transforms,
                memory, schema, [&](std::uint32_t object)
                    -> pal4::IScriptEventSchedulerRuntime& {
                    Check(object == first_monster, "outer scheduler resolves actual owner");
                    ++scheduler_requests;
                    return *active_scheduler;
                }, [&](std::uint32_t object, float delta) {
                    Check(object == first_monster && manager.AnimationBinding(object).AdvanceFrame(delta),
                          "outer frame advances the shared native monster animation binding");
                    ++outer_animation_steps;
                });
            // Exercise the recovered manager entry as well as the real first
            // monster's outer frame. Other actors are stationary target fixtures;
            // their active callbacks are visited but are not driven by this probe.
            auto update_outer = [&](float delta) {
                std::uint32_t result{};
                (void)manager.ProcessWorldObjects(delta, 0u,
                    [&](std::uint32_t object, float step, std::uint32_t mode) {
                        if (object == first_monster)
                            result = world_frame.Update(object, step, mode);
                    });
                return result;
            };
            player_control[1u] = second_monster;
            player_control[0x2dcu / 4u] = 1u;
            (void)pal4::CollisionActor::StopMovement(movement_state);
            (void)pal4::NativeBaseObjectMotionFrame::Update(
                monster_base, 0.0f, 0u, monster_collision, transforms, memory,
                schema, [](float) {});
            const auto outer_origin = position();
            (void)monster_resource.UpdateClumpTransform(
                {outer_origin[0u], outer_origin[1u], outer_origin[2u]}, false, transforms);
            (void)target_resource.UpdateClumpTransform(
                {start_position[0u], start_position[1u], start_position[2u]}, false, transforms);
            const auto outer_skill_visits = skill_visits;
            monster_state.words[0x1f0u / 4u] = 0u;
            Check(update_outer(0.01f) == 0u &&
                      skill_visits == outer_skill_visits && outer_animation_steps == 0u &&
                      world_frame.LastMotion().collision_updates == 0u,
                  "inactive outer monster frame skips pursuit, events and motion");
            monster_state.words[0x1f0u / 4u] = 1u;
            monster_state.words[0x730u / 4u] = 1u;
            monster_state.words[0x734u / 4u] = std::bit_cast<std::uint32_t>(0.02f);
            monster_state.words[0x738u / 4u] = 0u;
            for (std::uint32_t step = 0u; step < 2u; ++step) {
                const auto result = update_outer(0.01f);
                Check(monster_state.words[0x730u / 4u] == 1u &&
                          skill_visits == outer_skill_visits && scheduler_requests == 0u &&
                          world_frame.LastMotion().animation_updates == 1u &&
                          result == world_frame.LastMotion().position_result,
                      "snare holds through exact duration while outer position stage still runs");
            }
            (void)update_outer(0.01f);
            Check(monster_state.words[0x730u / 4u] == 0u &&
                      monster_state.words[0x738u / 4u] == monster_state.words[0x734u / 4u] &&
                      skill_visits == outer_skill_visits && scheduler_requests == 0u &&
                      outer_animation_steps == 3u,
                  "snare expiration clamps timer and runs collision gate then motion without pursuit/events");
            (void)update_outer(0.01f);
            Check(monster_state.words[0x6e0u / 4u] == 1u &&
                      skill_visits == outer_skill_visits + 1u && scheduler_requests == 0u,
                  "next outer frame discovers target and skips idle event scheduling");
            bool outer_moved = false;
            for (std::uint32_t step = 0u; step < 32u; ++step) {
                (void)update_outer(0.01f);
                outer_moved = outer_moved || distance(outer_origin, position()) > 0.02f;
                if (outer_moved && !pal4::CollisionActor::PathEnabled(movement_state)) break;
            }
            Check(outer_moved && scheduler_requests == 0u,
                  "actual outer monster entry drives pursuit and position together");
            monster_state.words[0x704u / 4u] = 1u;
            bool task_boundary = false;
            try {
                for (std::uint32_t step = 0u; step < 256u; ++step)
                    (void)update_outer(0.01f);
            } catch (const std::logic_error& error) {
                task_boundary = std::string_view(error.what()) ==
                    "native behavior task fixture boundary";
            }
            Check(task_boundary && monster_state.words[0x6e0u / 4u] == 0u &&
                      task_factory.calls == 1u && task_factory.last_class == 1 &&
                      task_factory.last_tree == first_monster + 0x698u &&
                      task_factory.last_container == first_monster + 0x688u &&
                      task_factory.last_name == "beh-vtm-atomic" &&
                      task_factory.last_options.mode == 2 &&
                      task_factory.last_options.wait_time == 4.0f &&
                      task_factory.last_options.expect_time == 10.0f,
                  "outer return reaches native scheduler and original class-one task with original timing");
            const auto candidate_array = memory.ReadDword(first_monster + 0x658u + 36u);
            Check(candidate_array != 0u &&
                      memory.ReadDword(first_monster + 0x658u + 40u) == candidate_array &&
                      memory.ReadDword(first_monster + 0x658u + 44u) == candidate_array + 4u,
                  "scheduler exception retains actual candidate storage and reset end");
            const auto scheduler_allocations = memory.OutstandingAllocations();
            for (std::uint32_t attempt = 0u; attempt < 3u; ++attempt) {
                bool same_boundary = false;
                try {
                    (void)update_outer(0.01f);
                } catch (const std::logic_error& error) {
                    same_boundary = std::string_view(error.what()) ==
                        "native behavior task fixture boundary";
                }
                Check(same_boundary && memory.OutstandingAllocations() == scheduler_allocations &&
                          memory.ReadDword(first_monster + 0x658u + 36u) == candidate_array,
                      "native scheduler reuses condition/candidate storage and releases outer scope on failure");
            }
            {
                auto helpers = std::make_shared<ClassOneHelperServiceFixture>(
                    memory, first_monster + 0x6b8u);
                pal4::NativeBehaviorClassOneTasks class_one_tasks(memory,
                    first_behavior.Tasks(), first_monster,
                    first_behavior.BehaviorTreeNil(), helpers);
                pal4::NativeBehaviorAtomicDispatchRuntime native_atomics(
                    first_behavior.EventDocuments(), first_behavior.Tasks(), class_one_tasks);
                pal4::NativeScriptEventSchedulerRuntime native_scheduler(memory,
                    first_behavior.EventDocuments(), first_behavior.Tasks(), native_atomics);
                active_scheduler = &native_scheduler;
                (void)update_outer(0.0f);
                auto& task_container = *reinterpret_cast<pal4::GomTaskContainerState*>(
                    first_monster + 0x688u);
                const auto task_address = memory.ReadDword(
                    memory.ReadDword(task_container.sentinel) + 8u);
                auto& task = first_behavior.Tasks().TaskAt(task_address);
                Check(task_container.count == 1u && !task_container.ready &&
                          first_behavior.Tasks().RegisteredTasks() == 1u &&
                          task.vtable == 0x8427d4u && task.phase == 2u &&
                          task.wait_mode == 2u && task.wait_time == 4.0f &&
                          task.expect_time == 10.0f && task.wait_elapsed == 0.0f &&
                          memory.ReadDword(task_address + 0x3cu) == first_monster + 0x6b8u &&
                          memory.ReadDword(task_address + 0x40u) == 0u &&
                          std::string_view(reinterpret_cast<const char*>(task.name)) ==
                              "beh-vtm-atomic",
                      "original class-one factory tree creates, binds C03 target and begins real Gom wait");
                const auto waiting_allocations = memory.OutstandingAllocations();
                for (std::uint32_t second = 0u; second < 3u; ++second)
                    (void)update_outer(1.0f);
                Check(task.phase == 2u && task.wait_elapsed == 3.0f &&
                          helpers->construction_attempts == 0u &&
                          memory.OutstandingAllocations() == waiting_allocations,
                      "original four-second wait does not construct a helper early");
                for (std::uint32_t retry = 0u; retry < 3u; ++retry) {
                    bool helper_boundary = false;
                    try {
                        (void)update_outer(retry ? 0.0f : 1.0f);
                    } catch (const std::logic_error& error) {
                        helper_boundary = std::string_view(error.what()) ==
                            "native class-one helper construction boundary";
                    }
                    Check(helper_boundary && task.phase == 2u &&
                              task.wait_elapsed == 4.0f &&
                              memory.ReadDword(task_address + 0x40u) == 0u &&
                              helpers->construction_attempts == retry + 1u &&
                              memory.OutstandingAllocations() == waiting_allocations,
                          "class-one begin retries release failed helper allocations without dangling pointers");
                }
                helpers->allow_construct = true;
                helpers->reject_accept = true;
                bool rejected_accept = false;
                try {
                    (void)update_outer(0.0f);
                } catch (const std::logic_error& error) {
                    rejected_accept = std::string_view(error.what()) ==
                        "class-one accept fixture failure";
                }
                Check(rejected_accept && helpers->discards == 1u && task.phase == 2u &&
                          memory.ReadDword(task_address + 0x40u) == 0u &&
                          memory.OutstandingAllocations() == waiting_allocations,
                      "failed C03 acceptance destroys unaccepted helper and clears task pointer");
                (void)first_behavior.Tasks().ClearTaskContainer(first_monster + 0x688u);
                Check(task_container.count == 0u && task_container.ready &&
                          first_behavior.Tasks().RegisteredTasks() == 0u &&
                          memory.OutstandingAllocations() == scheduler_allocations,
                      "interrupted native class-one wait releases task, name and list node");
                helpers->reject_accept = false;
                for (std::uint32_t cycle = 0u; cycle < 3u; ++cycle) {
                    (void)update_outer(0.0f);
                    const auto running_address = memory.ReadDword(
                        memory.ReadDword(task_container.sentinel) + 8u);
                    auto& running = first_behavior.Tasks().TaskAt(running_address);
                    (void)update_outer(4.0f);
                    Check(running.phase == 3u && running.run_elapsed == 0.0f &&
                              helpers->accepts == cycle + 1u,
                          "Gom wait enters class-one creation without ticking on entry frame");
                    for (std::uint32_t second = 0u; second < 9u; ++second)
                        (void)update_outer(1.0f);
                    Check(running.phase == 3u && running.run_elapsed == 9.0f &&
                              helpers->releases == cycle,
                          "class-one task remains running below original expected duration");
                    (void)update_outer(1.0f);
                    Check(running.phase == 4u && running.finish_latch == 14.0f &&
                              memory.ReadDword(running_address + 0x40u) == 0u &&
                              helpers->releases == cycle + 1u,
                          "expected duration releases C03 helper and latches total task time");
                    (void)update_outer(0.0f);
                    Check(task_container.count == 0u && task_container.ready &&
                              first_behavior.Tasks().RegisteredTasks() == 0u &&
                              memory.OutstandingAllocations() == scheduler_allocations,
                          "completed class-one cycles free native task/list/name without allocation growth");
                }
                Check(helpers->updates == 30u,
                      "class-one ticks dispatch existing run method through injected service");
                (void)update_outer(0.0f);
                (void)update_outer(4.0f);
                (void)first_behavior.Tasks().ClearTaskContainer(first_monster + 0x688u);
                Check(helpers->accepts == 4u && helpers->releases == 4u &&
                          first_behavior.Tasks().RegisteredTasks() == 0u &&
                          memory.OutstandingAllocations() == scheduler_allocations,
                      "deleting a running class-one task releases accepted helper exactly once");
                for (const auto class_id : {0, 99}) {
                    bool rejected_factory = false;
                    try {
                        (void)class_one_tasks.Create(first_monster + 0x698u,
                            class_id, first_monster + 0x688u, "unsupported",
                            {2, 4.0f, 10.0f});
                    } catch (const std::logic_error&) { rejected_factory = true; }
                    Check(rejected_factory && task_container.count == 0u &&
                              first_behavior.Tasks().RegisteredTasks() == 0u &&
                              memory.OutstandingAllocations() == scheduler_allocations,
                          "unsupported and absent native factories fail without publishing a null task");
                }
                {
                    pal4::NativeBehaviorClassOneTasks temporary_factory(memory,
                        first_behavior.Tasks(), first_monster,
                        first_behavior.BehaviorTreeNil(), helpers);
                    const auto transient = temporary_factory.Create(
                        first_monster + 0x698u, 1, first_monster + 0x688u,
                        "retained-service", {3, 0.0f, 0.0f});
                    temporary_factory.Bind(transient, first_monster + 0x6acu, 0u);
                    (void)first_behavior.Tasks().BeginWaiting(first_monster + 0x688u);
                }
                for (std::uint32_t stage = 0u; stage < 3u; ++stage)
                    (void)first_behavior.Tasks().UpdateTaskContainer(first_monster + 0x688u, 0.0f);
                Check(helpers->accepts == 5u && helpers->releases == 5u &&
                          helpers->updates == 31u &&
                          first_behavior.Tasks().RegisteredTasks() == 0u &&
                          memory.OutstandingAllocations() == scheduler_allocations,
                      "task registry retains lifecycle service after factory facade destruction");
                active_scheduler = &scheduler;
            }
            std::cout << "PASS: original class-one task creation, C03 binding, four-second wait and cancellation\n";
            player_control[0x2dcu / 4u] = 0u;
            player_control[1u] = 0u;
            monster_state.words[0x704u / 4u] = 0u;
            (void)manager.ResetOwnedPursuit(first_monster);
            (void)manager.ResetOwnedPursuit(second_monster);
            (void)manager.ResetOwnedPursuit(third_monster);
            std::cout << "PASS: native outer monster frame, snare expiry and original idle task boundary\n";
            pal4::test::VerifyMonsterIdle(root,memory,manager,monster_movement,transforms,first_monster,
                monster_collision,schema,proximity_math,proximity,player_control.data(),reinterpret_cast<std::uint32_t>(monster_world.World()));
            // First real grid admission retains its pointer-vector capacity.
            // The helper probe verifies subsequent cycles do not grow it.
            allocations_before_collision=manager.OutstandingAllocations();
            pal4::NativeMonsterCombatCollector combat_group(manager, memory, proximity);
            monster_state.words[0x6e4u / 4u] = second_monster;
            (void)combat_group.Collect(first_monster);
            const auto combat_source = memory.ReadDword(first_monster + 0x654u);
            const auto combat_count = memory.ReadDword(combat_source + 0x40u);
            const auto combat_ids = memory.ReadDword(combat_source + 0x44u);
            const auto combat_records = monster_state.words[0x714u / 4u];
            const auto combat_names = monster_state.words[0x724u / 4u];
            Check(combat_count && monster_state.words[0x718u / 4u] == combat_records + combat_count * 12u &&
                      monster_state.words[0x728u / 4u] == combat_names + 16u,
                  "real monster collector uses original scene source count and initiator name");
            for (std::uint32_t i = 0; i < combat_count; ++i)
                Check(memory.ReadDword(combat_records + i * 12u) == memory.ReadDword(combat_ids + i * 4u),
                      "real original-resource combat ids survive native collection");
            Check(std::string_view(reinterpret_cast<const char*>(memory.ReadDword(combat_names + 4u))) ==
                      reinterpret_cast<const char*>(memory.ReadDword(first_monster + 4u)),
                  "real collected name matches the resource's instance label");
            (void)manager.ResetOwnedPursuit(first_monster);
            (void)manager.EnterOwnedPursuit(first_monster, second_monster);
            Check(monster_state.words[0x718u / 4u] == combat_records &&
                      monster_state.words[0x728u / 4u] == combat_names &&
                      !memory.ReadDword(combat_names + 4u),
                  "actual new pursuit entry clears host encounter lists through the shared game boundary");
            (void)manager.ResetOwnedPursuit(first_monster);
            pal4::test::VerifyMonsterInteraction(root,memory,manager,monster_entry,
                monster_movement,proximity_math,proximity,player_control.data(),
                first_monster,second_monster);
            pal4::test::VerifyMonsterEncounter(root,memory,manager,monster_movement,
                proximity,first_monster,second_monster);
            monster_entry.Clear();
            monster_effects.Clear();
            monster_effects.UnbindArena();
            Check(monster_entry.Count() == 0u &&
                      monster_movement.RegisteredObjectAtomics() == 0u,
                  "M06/3A monster exit releases Movement and world bindings");
            manager.VisitObjects([&](const std::uint32_t monster) {
                Check(*reinterpret_cast<const std::uint32_t*>(
                          static_cast<std::uintptr_t>(monster + 0x568u)) == 0u &&
                          RpClumpGetWorld(reinterpret_cast<RpClump*>(
                              *reinterpret_cast<const std::uint32_t*>(
                                  static_cast<std::uintptr_t>(monster + 0x14u)))) ==
                              nullptr,
                      "M06/3A monster exit clears collision and world pointers");
                return true;
            });
            monster_collision.Release();
            monster_movement.Unload();
            monster_world.Unload();
            (void)manager.ProcessCollisions(0.1f);
            Check(manager.OutstandingAllocations() ==
                      allocations_before_collision &&
                      manager.ObjectCount() == 3u,
                  "idle original monsters pass the Scene collision gate");
            auto* pursuit_state = reinterpret_cast<std::uint32_t*>(
                static_cast<std::uintptr_t>(first_monster + 0x6e0u));
            *pursuit_state = 1u;
            *reinterpret_cast<std::uint32_t*>(
                static_cast<std::uintptr_t>(first_monster + 0x6e4u)) = 1u;
            bool pursuit_unbound = false;
            try {
                (void)manager.ProcessCollisions(0.1f);
            } catch (const std::logic_error&) {
                pursuit_unbound = true;
            }
            *pursuit_state = 0u;
            *reinterpret_cast<std::uint32_t*>(
                static_cast<std::uintptr_t>(first_monster + 0x6e4u)) = 0u;
            Check(pursuit_unbound &&
                      manager.OutstandingAllocations() ==
                          allocations_before_collision,
                  "pursuing monster reaches its explicit native path boundary");
            for (std::size_t index = 0u; index < reader.Records().size();
                 ++index) {
                const auto monster = manager.FindObject(
                    reader.Records()[index].name);
                const auto processor = monster +
                    pal4::MonsterBehaviorManager::kProcessorOffset;
                const auto first = *reinterpret_cast<const std::uint32_t*>(
                    static_cast<std::uintptr_t>(processor + 4u));
                const auto last = *reinterpret_cast<const std::uint32_t*>(
                    static_cast<std::uintptr_t>(processor + 8u));
                Check((first == 0u ? 0u : (last - first) / 4u) ==
                          (index < 2u ? 1u : 0u),
                      "original monster scene initializes expected behavior events");
                if (index < 2u) {
                    const auto event = *reinterpret_cast<const std::uint32_t*>(
                        static_cast<std::uintptr_t>(first));
                    const auto atomic = *reinterpret_cast<const std::uint32_t*>(
                        static_cast<std::uintptr_t>(event + 24u));
                    Check(*reinterpret_cast<const std::uint32_t*>(
                              static_cast<std::uintptr_t>(atomic + 52u)) == 5u,
                          "original monster scene stores complete fuzzy atomic attributes");
                }
            }
            pal4::NativePalGameIVTextureStorage reset_game([](std::uint32_t dictionary) {
                (void)RwTexDictionaryDestroy(reinterpret_cast<RwTexDictionary*>(dictionary));
            });
            const auto reset_game_address=reset_game.Construct();
            pal4::NativePkFileManagerRuntime reset_packages;
            std::array<std::uint32_t,20> reset_filesystem{};
            reset_filesystem[10]=0x1010u;
            const auto reset_package_manager=reset_packages.InitializePackFileManager();
            reset_packages.ProcessPackFiles(reset_package_manager,
                reinterpret_cast<std::uint32_t>(reset_filesystem.data()));
            pal4::NativePlayerMaterialRuntime reset_materials(root,reset_packages);
            const auto retained_manager=manager.Address();
            unsigned reset_count=0u;
            manager.CleanupScene([&](std::uint32_t object) {
                const auto retained_source=reinterpret_cast<const std::uint32_t*>(object)[0x654u/4u];
                manager.CleanupOwnedActivity(object,reset_game_address,reset_materials);
                const auto* words=reinterpret_cast<const std::uint32_t*>(object);
                Check(!words[0x14u/4u] && !words[0x30u/4u] && !words[0x178u/4u] &&
                          retained_source && words[0x654u/4u]==retained_source &&
                          !words[0x704u/4u] && !words[0x730u/4u],
                      "original monster activity reset retires resources and pursuit state while retaining source before deletion");
                ++reset_count;
            });
            Check(reset_count==3u && manager.Address()==retained_manager &&
                      manager.ObjectCount()==0u && manager.InfoCount()==0u && !manager.GridAddress() &&
                      memory.OutstandingAllocations()==0u,
                  "three original M06 monsters use actual activity cleanup while retaining empty manager");
            manager.CleanupScene({});
            Check(manager.ProcessCollisions(0.1f)!=0u,"cleaned M06 manager supports intermediate Scene collision pass");
            reset_materials.ClearCache();
            reset_packages.Shutdown();
            reset_game.Destroy();
            Check(reset_game.CoreAllocationCount()==0u,"monster activity reset releases native game context");
            Check(manager.InitializeScene({}) &&
                      manager.ObjectCount() == 0u &&
                      memory.OutstandingAllocations() == 0u,
                  "monster scene reload releases objects and RenderWare assets");
            manager.Destroy();
            Check(manager.OutstandingAllocations() == 0u,
                  "monster manager releases information and grid allocations");
        }
        {
            pal4::NativeBaseObjectMemory memory;
            pal4::test::RwDictSchemaFake schema;
            pal4::NativePlayerTransformRuntime transforms;
            pal4::NativePalGameIVTextureStorage game([](std::uint32_t dictionary) {
                (void)RwTexDictionaryDestroy(reinterpret_cast<RwTexDictionary*>(dictionary));
            });
            (void)game.Construct();
            pal4::NativePkFileManagerRuntime gob_packages;
            std::array<std::uint32_t,20> gob_filesystem{};
            gob_filesystem[10]=0x1010u;
            const auto gob_package_manager=gob_packages.InitializePackFileManager();
            gob_packages.ProcessPackFiles(gob_package_manager,
                reinterpret_cast<std::uint32_t>(gob_filesystem.data()));
            pal4::NativeEffectSceneSession gob_effects(root,gob_packages);
            pal4::NativeSceneGobAssets objects;
            Check(objects.Load(root,"Q01","Q01",transforms,memory,schema,
                               gob_packages,gob_effects.GobEffects()) && objects.Size()==48u,
                  "load all six original Q01 GOB types for real SDK activity cleanup");
            pal4::test::VerifyNativeGobRetainedConfig(objects);
            const auto manager=objects.ManagerAddress();
            const auto header=memory.ReadDword(manager+4u);
            const auto factories=memory.ReadDword(manager+0x18u);
            const auto persistent_allocations=objects.RetainedManagerAllocations();
            Check(objects.AnimationBindingCount()!=0u && persistent_allocations!=0u,
                  "populated GOB cleanup fixture owns real animation clips and persistent manager storage");
            {
                pal4::NativeMonsterManagerOwner monsters;(void)monsters.Construct();
                pal4::NativeMinimapManagerOwner minimap;
                minimap.BindSceneObjects(objects);
                Check(minimap.Initialize(root) && minimap.LoadScene(root,"Q01","Q01"),
                    "initialize real Q01 minimap for current scene save capture");
                const auto expected_grid=minimap.CaptureSaveSnapshot();
                Check(expected_grid.header==1u && expected_grid.elements.size()==1u,
                    "unmodified original Q01 minimap includes its actual initial grid mask");
                const auto gob_allocations=memory.OutstandingAllocations();
                const auto map_allocations=minimap.OutstandingAllocations();
                const auto snapshot=pal4::NativeArenaSaveCapture::Capture("Q01","Q01",objects,monsters,minimap);
                Check(snapshot.names==std::array<std::string,2>{"Q01","Q01"} && snapshot.groups[0].size()==36u &&
                          snapshot.groups[1].empty() && snapshot.minimap_header==expected_grid.header &&
                          snapshot.minimap_elements==expected_grid.elements &&
                          memory.OutstandingAllocations()==gob_allocations && minimap.OutstandingAllocations()==map_allocations &&
                          objects.Size()==48u && objects.ManagerAddress()==manager,
                    "actual Q01 capture produces36 stored GOB children and minimap without changing live creating owners");
                pal4::NativeArenaCacheOwner cache;pal4::NativeGameSaveArchiveBuffer archive;
                Check(cache.SaveCurrent(snapshot,archive),"write actual composed scene capture through shared arena cache");
                std::size_t consumed{};
                const auto parsed=pal4::NativeArenaCacheSaveSection::ParsePrefix(
                    std::span(reinterpret_cast<const std::uint8_t*>(archive.Bytes().data()),archive.Bytes().size()),consumed);
                Check(parsed && consumed==archive.Bytes().size() && parsed->entries.size()==1u &&
                          parsed->entries[0].names==snapshot.names && parsed->entries[0].groups[0].size()==snapshot.groups[0].size() &&
                          parsed->entries[0].minimap_header==snapshot.minimap_header &&
                          parsed->entries[0].minimap_elements==snapshot.minimap_elements,
                    "actual scene capture preserves exact cache section boundary and child count");
                for(std::size_t i=0;i<snapshot.groups[0].size();++i){
                    const auto& expected=snapshot.groups[0][i];const auto& actual=parsed->entries[0].groups[0][i];
                    const auto before=expected.document.Serialize(),after=actual.document.Serialize();
                    Check(expected.id==actual.id && before && after && *before==*after,
                        "all actual scene child types/order/XML remain byte-stable through cache writer/parser");
                }
                cache.Destroy();Check(!cache.OutstandingAllocations(),"composed scene cache releases all native snapshots");
                minimap.Destroy();monsters.Destroy();
                std::cout<<"Native live arena save capture: Q01 real48 GOBs ->36 stored children, exact XML/cache roundtrip and creator ownership pass\n";
            }
            objects.CleanupScene(game);
            Check(objects.ManagerAddress()==manager && !objects.Size() &&
                      !objects.AnimationBindingCount() &&
                      objects.FactoryCount()==11u && memory.ReadDword(manager+4u)==header &&
                      memory.ReadDword(manager+0x18u)==factories && !memory.ReadDword(manager+12u) &&
                      memory.OutstandingAllocations()==persistent_allocations,
                  "all original GOB SDK assets retire while manager, headers and factories remain");
            objects.ActivateAll();
            objects.DeactivateAll();
            objects.CleanupScene(game);
            Check(objects.Load(root,"Q01","N01",transforms,memory,schema,
                               gob_packages,gob_effects.GobEffects()) && objects.Size()==4u &&
                      objects.ManagerAddress()==manager && memory.ReadDword(manager+4u)==header &&
                      memory.ReadDword(manager+0x18u)==factories && objects.FindObject("MO001"),
                  "subsequent original GOB scene reuses manager and headers with new object registrations");
            objects.CleanupScene(game);
            Check(memory.OutstandingAllocations()==persistent_allocations,
                  "second original GOB cleanup returns to retained-manager allocation count");
            objects.Clear();
            Check(!memory.OutstandingAllocations(),"final GOB destruction frees retained manager storage");
            game.Destroy();
            Check(!game.CoreAllocationCount(),"GOB cleanup fixture releases actual game texture storage");
        }
        pal4::NativePackageArchive ui_archive;
        Check(ui_archive.Open((root / "gamedata" / "ui.cpk").string().c_str()),
              "open original UV animation archive");
        pal4::NativeActorClumpAsset schema_clump;
        Check(schema_clump.Load(root, "101"),
              "load actor Clump for native dictionary traversal");
        pal4::NativeRwDictSchemaRuntime schema_runtime(ui_archive);
        pal4::RwDictSchema schema;
        Check(schema.LoadDict("uiWorld\\zjm\\ZJM_trans.uva",
                              schema_runtime) &&
                  schema.dict != 0u &&
                  schema.SetClump(
                      reinterpret_cast<std::uint32_t>(schema_clump.Main()),
                      schema_runtime) &&
                  schema.list != 0u,
              "rewritten dictionary reads original UV data through SDK");
        Check(schema.Cleanup(schema_runtime) != 0u &&
                  schema.dict == 0u && schema.list == 0u &&
                  RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema()) ==
                      nullptr,
              "SDK dictionary and material list release after traversal");
        pal4::PlayerManagerObjectState player_slots;
        pal4::NativeGameDatabase database;
        Check(database.Load(root), "load original Role database for actors");
        {
            pal4::NativeGameArenaManagerOwner arena;
            pal4::NativeMinimapManagerOwner minimap;
            pal4::NativeCeguiUiSession ui;
            pal4::NativeSceneRecordCatalog scenes(database);
            pal4::NativeSceneGobAssets objects;
            pal4::NativeMinimapSceneUiUpdate hud(root, arena, minimap, ui,
                scenes, objects, 800, 600);
            // The bounded read adapter borrows a manager identity; no DLL UI
            // or countdown is started by this persistence boundary check.
            std::array<std::uint32_t, 90u> manager{};
            const auto identity = reinterpret_cast<std::uint32_t>(manager.data());
            auto* owner = reinterpret_cast<std::uint8_t*>(hud.ActiveComponentAddress() - 0x28u);
            Check(hud.TimerFrameDuration(identity) == 0u,
                "live portrait timer initially reads its owned zero field");
            for (const auto duration : {37u, 0x80000000u, 0xffffffffu}) {
                std::memcpy(owner + 0xf0u, &duration, sizeof(duration));
                std::array<std::uint8_t, 0xf8u> before;
                std::memcpy(before.data(), owner, before.size());
                Check(hud.TimerFrameDuration(identity) == duration &&
                          std::memcmp(before.data(), owner, before.size()) == 0,
                    "portrait timer getter reads all live bits without mutating HUD");
            }
            bool rejected = false;
            try { (void)hud.TimerFrameDuration(0u); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected, "portrait save timer rejects missing manager identity");
            std::cout << "Native portrait save timer: live_zero_nonzero_bits_and_nonmutation=passed\n";
        }
        for (auto& slot : player_slots.slots) {
            pal4::GameResourceManager manager;
            (void)manager.Construct();
            std::memcpy(slot.base.Data(), &manager, sizeof(manager));
        }
        pal4::NativePlayerActorAssets players(root);
        pal4::NativePlayerSlotCopyRuntime copier;
        pal4::NativePlayerTransformRuntime transforms;
        RoleLoadHost role_host;
        pal4::NativeRoleRecordCatalog role_records(database);
        {
            pal4::NativeBaseObjectMemory role_animation_memory;
            EmptyMonsterAnimationRuntime role_lifecycle(role_animation_memory);
            for(auto& slot:player_slots.slots) {
                auto& state=*reinterpret_cast<pal4::GameObjectEmbeddedAnimationState*>(
                    reinterpret_cast<std::uintptr_t>(&slot)+0x170u);
                (void)pal4::GameObjectEmbeddedAnimation::InitializeState(state,0u,role_lifecycle);
            }
            pal4::NativePlayerAnimationSet role_animations(player_slots,players,
                role_animation_memory,role_lifecycle,root);
            pal4::NativePlayerRoleLoadRuntime roles(role_records,role_host,copier,
                &players,&transforms,nullptr,nullptr,&role_animations);
            Check(pal4::PlayerManagerLoader::Load(
                      reinterpret_cast<std::uint32_t>(&player_slots),roles)==0x49u && players.Count()==4u,
                  "original Role loader binds all four player actor resources");
            for(std::size_t index=0;index<4;++index) {
                const auto& base=player_slots.slots[index].base;
                const auto* first=reinterpret_cast<const RtAnimInterpolator*>(base.Read(0x180u));
                const auto* second=reinterpret_cast<const RtAnimInterpolator*>(base.Read(0x184u));
                Check(role_animations.At(index).Bound() && base.Read(0x174u)==base.Read(0x14u) &&
                          base.Read(0x178u) && base.Read(0x17cu) && first && second && first!=second &&
                          !first->pCurrentAnim && !second->pCurrentAnim &&
                          first->numNodes==players.At(index).Hierarchy()->numNodes &&
                          first->maxInterpKeyFrameSize==players.At(index).Hierarchy()->currentAnim->maxInterpKeyFrameSize,
                      "every loaded Role has counted records and two initial SDK blend snapshots before scene entry");
            }
            std::array<const pal4::NativeActorAnimationStateBinding*,4> role_bindings{};
            for(std::size_t index=0;index<4;++index)role_bindings[index]=&role_animations.At(index);
            roles.RollbackActors();
            Check(players.Count()==0u && role_animation_memory.OutstandingAllocations()==0u,
                  "Role rollback releases all animation records before unloading their SDK clips");
            Check(pal4::PlayerManagerLoader::Load(reinterpret_cast<std::uint32_t>(&player_slots),roles)==0x49u && players.Count()==4u,
                  "Role loader retries after actual four-actor rollback");
            for(std::size_t index=0;index<4;++index)
                Check(&role_animations.At(index)==role_bindings[index] && role_bindings[index]->Bound(),
                      "Role rollback and retry preserve stable per-slot animation owners");
            roles.CommitActors();
            role_animations.Release();
            Check(role_animation_memory.OutstandingAllocations()==0u,
                  "four Role animation owners release records before their borrowed actor resources");
        }
        constexpr std::array<std::string_view, 4> models{
            "101", "103", "106", "105"};
        constexpr std::array<std::string_view, 4> names{
            "Player-TIANHE", "Player-LINSHA", "Player-MENGLI",
            "Player-ZIYING"};
        for (std::size_t index = 0; index < models.size(); ++index) {
            const auto& base = player_slots.slots[index].base;
            const auto& actor = players.At(index);
            Check(base.Read(4u) != 0u &&
                      std::string_view(reinterpret_cast<const char*>(
                          static_cast<std::uintptr_t>(base.Read(4u)))) ==
                          names[index] &&
                      base.Read(0xcu) != 0u &&
                      std::string_view(reinterpret_cast<const char*>(
                          static_cast<std::uintptr_t>(base.Read(0xcu)))) ==
                          models[index] &&
                      base.Read(0x14u) ==
                      reinterpret_cast<std::uint32_t>(actor.Main()) &&
                      base.Read(0x18u) == 1u &&
                      base.Read(0x24u) == 1u &&
                      base.Read(0x30u) ==
                      reinterpret_cast<std::uint32_t>(actor.Shadow()) &&
                      base.Read(0x530u) == reinterpret_cast<std::uint32_t>(
                          actor.BottomEffectHook()) &&
                      base.Read(0x550u) == reinterpret_cast<std::uint32_t>(
                          actor.FaceMaterial()) &&
                      base.Read(0x560u) == 0u &&
                      player_slots.Word((index * 0xb14u + 0xa1cu) / 4u) !=
                          0u &&
                      player_slots.Word((index * 0xb14u + 0x654u +
                                         167u * 4u) / 4u) == index,
                  "player slot binds owned clumps and Atomic metadata");
        }
        pal4::PlayerManagerSlotState extra_slot;
        Check(players.Count() == 4u &&
                  !players.Load(reinterpret_cast<std::uint32_t>(&extra_slot),
                                "Player-TIANHE", "101") &&
                  players.At(0).ActionCount() == 56u &&
                  players.At(1).ActionCount() == 52u &&
                  players.At(2).ActionCount() == 43u &&
                  players.At(3).ActionCount() == 42u,
              "four player slots retain all original actor actions");
        const auto first_slot = reinterpret_cast<std::uint32_t>(
            &player_slots.slots[0]);
        const std::array<std::uint32_t, 6> saved_material_strings{
            player_slots.slots[0].base.Read(0x544u),
            player_slots.slots[0].base.Read(0x548u),
            player_slots.slots[0].base.Read(0x54cu),
            player_slots.slots[0].base.Read(0x554u),
            player_slots.slots[0].base.Read(0x558u),
            player_slots.slots[0].base.Read(0x55cu)};
        {
            auto& base = player_slots.slots[0].base;
            pal4::NativeBaseObjectMemory animation_memory;
            EmptyMonsterAnimationRuntime lifecycle(animation_memory);
            auto& animation = *reinterpret_cast<pal4::GameObjectEmbeddedAnimationState*>(
                first_slot + 0x170u);
            (void)pal4::GameObjectEmbeddedAnimation::InitializeState(animation, 0u, lifecycle);
            pal4::NativePlayerAnimationSet animations(player_slots, players,
                animation_memory, lifecycle, root);
            auto& binding = animations.At(0u);
            binding.Bind();
            const auto initial_from=animation.words[4u],initial_to=animation.words[5u];
            Check(initial_from && initial_to && initial_from!=initial_to,
                  "ordinary Bind creates both SDK snapshots before the first blend request");
            Check(binding.Select("C01") && binding.SelectBlended("C01", 0.1f) &&
                      binding.AdvanceBlend(0.05f) && animation.words[4u]==initial_from && animation.words[5u]==initial_to,
                  "original player binding owns live animation and blend state");
            Check(binding.Select("z03"),
                  "M06 lowercase script action selects original uppercase Z03");
            const auto z03_record = animation.words[3];
            const auto z03_event_begin = animation_memory.ReadDword(z03_record + 40u);
            const auto z03_event_end = animation_memory.ReadDword(z03_record + 44u);
            const auto z03_parameters = binding.AnimationParameters("z03");
            Check(binding.Select("Z03") && animation.words[3] == z03_record &&
                      animation_memory.ReadDword(z03_record + 40u) == z03_event_begin &&
                      animation_memory.ReadDword(z03_record + 44u) == z03_event_end &&
                      binding.AnimationParameters("Z03").data() == z03_parameters.data() &&
                      binding.AnimationParameters("Z03").size() == z03_parameters.size() &&
                      binding.SelectBlended("z03", 0.1f) &&
                      animation.words[3] == z03_record && binding.AdvanceBlend(0.05f) &&
                      binding.Select("c01"),
                  "action aliases share original clip, AMF events and parameters in both switches");
            bool rejected = false;
            try { binding.BindModel("103"); }
            catch (const std::logic_error&) { rejected = true; }
            Check(rejected && binding.Bound(),
                  "bound actor cannot change model before animation release");
            binding.Release();
            Check(animation_memory.OutstandingAllocations() == 0u &&
                      base.Read(0x17cu) == 0u && base.Read(0x180u) == 0u &&
                      base.Read(0x184u) == 0u &&
                      players.Load(first_slot, "Player-TIANHE", "103"),
                  "old animation records and blend references release before model replacement");
            rejected = false;
            try { binding.BindModel("101"); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected && !binding.Bound() &&
                      animation_memory.OutstandingAllocations() == 0u,
                  "animation binding rejects a different loaded model");
            binding.BindModel("103");
            Check(players.At(0u).ActionCount() == 52u &&
                      binding.Select("C01") && binding.AdvanceFrame(0.01f) &&
                      animation_memory.ReadDword(base.Read(0x17cu) - 4u) == 52u &&
                      animation_memory.ReadDword(base.Read(0x17cu) + 32u) ==
                          reinterpret_cast<std::uint32_t>(players.At(0u).AnimationAt(0u)),
                  "stable player binding selects replacement model clips and lazy event data");
            binding.Release();
            Check(animation_memory.OutstandingAllocations() == 0u &&
                      players.Load(first_slot, "Player-TIANHE", "101"),
                  "replacement model animation releases without retained clip records");
            binding.BindModel("101");
            Check(players.At(0u).ActionCount() == 56u && binding.Select("C01") &&
                      animation_memory.ReadDword(base.Read(0x17cu) - 4u) == 56u,
                  "same binding restores original player model and action catalog");
            binding.Release();
            Check(animation_memory.OutstandingAllocations() == 0u,
                  "repeated model binding releases every native animation allocation");
            pal4::NativePkFileManagerRuntime packages;
            std::array<std::uint32_t, 20> filesystem{};
            filesystem[10] = 0x1010u;
            const auto package_manager = packages.InitializePackFileManager();
            packages.ProcessPackFiles(package_manager,
                reinterpret_cast<std::uint32_t>(filesystem.data()));
            const auto package_allocations = packages.AllocationCount();
            ActorResetHost effects(players);
            pal4::NativePlayerMaterialRuntime materials(root, packages);
            pal4::NativePlayerActorReloadActivity activity(players, effects,
                [&](pal4::BaseObjectState& found) -> pal4::NativeActorAnimationStateBinding& {
                    return animations.For(found);
                }, materials);
            pal4::NativePlayerModelReloadRuntime reload(player_slots, animation_memory,
                packages, activity, transforms);
            const auto requested_model = animation_memory.ReadDword(first_slot + 0xa1cu);
            const char replacement[] = "103", original[] = "101";
            binding.BindModel("101");
            Check(binding.Select("C01") && binding.SelectBlended("C01", 0.1f),
                  "native reload starts with owned original animation references");
            animation_memory.WriteDword(first_slot + 0xa1cu,
                reinterpret_cast<std::uint32_t>(replacement));
            (void)pal4::ScriptProcessAndCleanup::Process(first_slot, reload);
            Check(binding.Bound() && binding.Select("C01") &&
                      players.At(0u).ActionCount() == 52u &&
                      base.Read(0x550u) == reinterpret_cast<std::uint32_t>(
                          players.At(0u).FaceMaterial()) && effects.cleanup_calls == 1u &&
                      std::string_view(reinterpret_cast<const char*>(base.Read(8u))) ==
                          "PALActor\\103\\" && base.Read(0x548u) != 0u &&
                      std::string_view(reinterpret_cast<const char*>(base.Read(0x544u))) ==
                          std::string("PALActor\\103\\") + RwTextureGetName(
                              RpMaterialGetTexture(players.At(0u).FaceMaterial())) &&
                      filesystem[10] == 0x1010u &&
                      packages.AllocationCount() == package_allocations,
                  "shared save reload replaces original resources, animation and FACE material");
            auto* face = players.At(0u).FaceMaterial();
            const std::string original_texture_name = RwTextureGetName(RpMaterialGetTexture(face));
            std::string crc_path="gamedata\\palactor\\103\\"+original_texture_name;
            for(auto& ch:crc_path)if(ch>='A' && ch<='Z')ch=static_cast<char>(ch-'A'+'a');
            pal4::LegacyCrc32 texture_crc;
            const auto restored_name=std::to_string(texture_crc.ComputeAndStore(
                reinterpret_cast<const std::uint8_t*>(crc_path.data()),static_cast<std::uint32_t>(crc_path.size())));
            auto* original_raster = RwTextureGetRaster(RpMaterialGetTexture(face));
            const auto original_width = RwRasterGetWidth(original_raster);
            const auto original_height = RwRasterGetHeight(original_raster);
            Check(RpMaterialSetTexture(face, nullptr) != nullptr &&
                      pal4::BaseObjectMaterialTexture::ApplyFace(base, materials) == 1u &&
                      RpMaterialGetTexture(face) &&
                      restored_name == RwTextureGetName(RpMaterialGetTexture(face)) &&
                      RwTextureGetFilterMode(RpMaterialGetTexture(face)) == rwFILTERLINEAR &&
                      RwRasterGetWidth(RwTextureGetRaster(RpMaterialGetTexture(face))) == original_width &&
                      RwRasterGetHeight(RwTextureGetRaster(RpMaterialGetTexture(face))) == original_height &&
                      materials.CachedTextureCount() == 1u && filesystem[10] == 0x1010u,
                  "native material restore decodes the original FACE texture from CPK");
            const auto* restored_texture = RpMaterialGetTexture(face);
            Check(pal4::BaseObjectMaterialTexture::ApplyFace(base, materials) == 1u &&
                      RpMaterialGetTexture(face) == restored_texture && materials.CachedTextureCount() == 1u,
                  "repeated original material restore reuses its cached texture");
            rejected = false;
            try { (void)pal4::BaseObjectMaterialTexture::SetFace(base, "bad/path", materials); }
            catch (const std::invalid_argument&) { rejected = true; }
            Check(rejected && filesystem[10] == 0x1010u &&
                      pal4::BaseObjectMaterialTexture::ApplyFace(base, materials) == 1u,
                  "failed material lookup releases package callbacks and permits retry");
            pal4::BaseObjectState leg;
            leg.Write(0x560u, reinterpret_cast<std::uint32_t>(face));
            leg.Write(0x554u, base.Read(0x544u)); leg.Write(0x558u, base.Read(0x548u));
            Check(pal4::BaseObjectMaterialTexture::ApplyLeg(leg, materials) == 1u &&
                      RpMaterialGetTexture(face) == restored_texture && filesystem[10] == 0x1010u,
                  "LEG restoration uses the same native texture boundary");
            materials.ClearCache();
            Check(materials.CachedTextureCount() == 0u && RpMaterialGetTexture(face) &&
                      restored_name == RwTextureGetName(RpMaterialGetTexture(face)) &&
                      RwTextureGetFilterMode(RpMaterialGetTexture(face)) == rwFILTERLINEAR,
                  "material retains its SDK texture reference after cache release");
            pal4::NativePackageArchive weapon_archive;
            Check(weapon_archive.Open((root / "gamedata/palweapon.cpk").string().c_str()),
                  "open original weapon archive for item/model evidence");
            const auto& equipment = database.Tables().at("Equip");
            const auto model_field = std::find_if(equipment.fields.begin(), equipment.fields.end(),
                [](const auto& field) { return field.name == "EA_MODEL"; });
            Check(model_field != equipment.fields.end(), "original Equip schema has its model field");
            const auto model_index = static_cast<std::size_t>(model_field - equipment.fields.begin());
            const auto sword = std::find_if(equipment.rows.begin(), equipment.rows.end(),
                [](const auto& row) { return std::get<std::uint32_t>(row[0]) == 3501u; });
            Check(sword != equipment.rows.end(), "original NewGame sword exists");
            const auto& weapon_model = std::get<std::string>((*sword)[model_index]);
            Check(weapon_model == "WTA" &&
                      weapon_archive.Find((weapon_model + "\\" + weapon_model + ".dff").c_str()) >= 0,
                  "NewGame sword3501 EA_MODEL resolves to the original WTA weapon CPK member");
            std::cout << "Original weapon Equip3501 EA_MODEL=" << weapon_model << '\n';
            const auto original_weapon_effect=weapon_archive.ReadMember((weapon_model+"\\"+weapon_model+"_WTA1.eff").c_str());
            std::cout << "Original WTA_WTA1.eff bytes=" << (original_weapon_effect ? original_weapon_effect->size() : 0u);
            if (original_weapon_effect && original_weapon_effect->size()>=32u) {
                std::array<std::uint32_t,8u> flags{};
                std::memcpy(flags.data(),original_weapon_effect->data(),32u);
                std::cout << " flags=";
                for (auto value : flags) std::cout << value << ',';
            }
            std::cout << '\n';
            pal4::RwDictSchema caller_uv_schema;
            Check(caller_uv_schema.LoadDict("uiWorld\\zjm\\ZJM_trans.uva",schema_runtime),
                  "retain original caller UV dictionary across named weapon operations");
            auto* caller_uv_dict=reinterpret_cast<RtDict*>(caller_uv_schema.dict);
            const bool weapon_has_uv=weapon_archive.Find((weapon_model+"\\"+weapon_model+".uva").c_str())>=0;
            std::cout << "Original WTA UV dictionary member=" << weapon_has_uv << '\n';
            WeaponCleanupFixture weapon_fixture(base, root, weapon_model);
            pal4::test::VerifyNativeHookEffect(root,weapon_fixture.weapons[0]);
            pal4::test::VerifyNativeStripVisual(root);
            Check(original_weapon_effect && original_weapon_effect->size()>=44u,
                  "original WTA effect metadata has header and three name lengths");
            std::array<std::uint32_t,8u> original_effect_fields{};
            std::memcpy(original_effect_fields.data(),original_weapon_effect->data(),32u);
            Check(original_effect_fields[4]==1u,"original WTA enables environment-map branch");
            std::size_t effect_cursor=32u;
            for (unsigned name_index=0;name_index<3u;++name_index) {
                Check(effect_cursor+4u<=original_weapon_effect->size(),"effect name length is in original member");
                std::uint32_t length{};
                std::memcpy(&length,original_weapon_effect->data()+effect_cursor,4u);
                effect_cursor+=4u;
                Check(length<=original_weapon_effect->size()-effect_cursor,"original effect name payload fits member");
                if (length) std::cout << "Original WTA effect name" << name_index << '='
                    << std::string(reinterpret_cast<const char*>(original_weapon_effect->data()+effect_cursor),length-1u) << '\n';
                effect_cursor+=length;
            }
            RwTexture* environment_texture{};
            (void)RpClumpForAllAtomics(weapon_fixture.weapons[0],[](RpAtomic* atomic,void* data) -> RpAtomic* {
                auto* geometry=RpAtomicGetGeometry(atomic);
                for (RwInt32 n=0;n<RpGeometryGetNumMaterials(geometry);++n)
                    if (auto* texture=RpMaterialGetTexture(RpGeometryGetMaterial(geometry,n))) {
                        *static_cast<RwTexture**>(data)=texture;
                        return nullptr;
                    }
                return atomic;
            },&environment_texture);
            Check(environment_texture!=nullptr,"native environment test borrows an actual SDK texture");
            std::array<std::uint32_t,3u> environment_config{0u,reinterpret_cast<std::uint32_t>(environment_texture),original_effect_fields[6]};
            Check(pal4::NativeClumpEnvironmentMap::Apply(reinterpret_cast<std::uint32_t>(weapon_fixture.weapons[0]),environment_config),
                  "shared environment pipeline executes on original WTA through SDK");
            struct EnvironmentCheck { RwTexture* texture; float coefficient; unsigned materials{}; };
            EnvironmentCheck environment_check{environment_texture,std::bit_cast<float>(original_effect_fields[6])};
            (void)RpClumpForAllAtomics(weapon_fixture.weapons[0],[](RpAtomic* atomic,void* data) -> RpAtomic* {
                auto& check=*static_cast<EnvironmentCheck*>(data);
                Check(atomic->pipeline==RpMatFXGetD3D9Pipeline(rpMATFXD3D9ATOMICPIPELINE),
                      "environment callback selects the real SDK MatFX atomic pipeline");
                auto* geometry=RpAtomicGetGeometry(atomic);
                for (RwInt32 n=0;n<RpGeometryGetNumMaterials(geometry);++n) {
                    auto* material=RpGeometryGetMaterial(geometry,n);
                    Check(RpMatFXMaterialGetEffects(material)==rpMATFXEFFECTENVMAP &&
                              RpMatFXMaterialGetEnvMapTexture(material)==check.texture &&
                              RpMatFXMaterialGetEnvMapFrame(material)==nullptr &&
                              !RpMatFXMaterialGetEnvMapFrameBufferAlpha(material) &&
                              std::fabs(RpMatFXMaterialGetEnvMapCoefficient(material)-check.coefficient)<0.00001f,
                          "SDK environment materials retain texture, null frame, disabled framebuffer alpha and original coefficient");
                    ++check.materials;
                }
                return atomic;
            },&environment_check);
            Check(environment_check.materials>0u,"native environment callback visits original weapon materials");

            std::vector<std::pair<RwTexture*,std::uint32_t>> weapon_texture_modes;
            (void)RpClumpForAllAtomics(weapon_fixture.weapons[0],
                [](RpAtomic* atomic,void* data) -> RpAtomic* {
                    auto& modes=*static_cast<std::vector<std::pair<RwTexture*,std::uint32_t>>*>(data);
                    auto* geometry=RpAtomicGetGeometry(atomic);
                    for (RwInt32 n=0;n<RpGeometryGetNumMaterials(geometry);++n) {
                        auto* texture=RpMaterialGetTexture(RpGeometryGetMaterial(geometry,n));
                        if (!texture) continue;
                        (void)RwTextureSetFilterMode(texture,rwFILTERNEAREST);
                        modes.emplace_back(texture,texture->filterAddressing & 0xffffff00u);
                    }
                    return atomic;
                }, &weapon_texture_modes);
            pal4::NativePlayerTransformRuntime weapon_transforms;
            pal4::NativePlayerNamedWeaponRuntime weapons(root, *weapon_fixture.world,
                0x7000u, packages, weapon_fixture, weapon_transforms);
            const char weapon_directory[] = "PALWeapon\\";
            const auto directory_address = reinterpret_cast<std::uint32_t>(weapon_directory);
            std::vector<std::string> original_uv_weapons;
            for (const auto& row : equipment.rows) {
                const auto& model=std::get<std::string>(row[model_index]);
                if (model.empty() || weapon_archive.Find((model+"\\"+model+".uva").c_str())<0 ||
                    std::find(original_uv_weapons.begin(),original_uv_weapons.end(),model)!=original_uv_weapons.end()) continue;
                original_uv_weapons.push_back(model);
            }
            std::cout << "Original Equip weapon models with UV dictionaries=" << original_uv_weapons.size() << '\n';
            for (const auto& model : original_uv_weapons) {
                Check(pal4::BaseObjectNamedWeapon::SetupRight(base,model,"",directory_address,0u,0u,weapons)==1u &&
                          base.Read(0x218u+148u)!=0u && base.Read(0x218u+152u)!=0u,
                      "original UV-bearing weapon creates a retained native dictionary and material list");
                pal4::RwDictSchema loaded_uv;
                loaded_uv.dict=base.Read(0x218u+148u);
                loaded_uv.list=base.Read(0x218u+152u);
                (void)loaded_uv.Update(0.25f,schema_runtime);
                Check(pal4::BaseObjectNamedWeapon::CleanupRight(base,weapons)==1u && weapons.OwnedCount()==0u &&
                          RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema())==caller_uv_dict,
                      "original UV-bearing named weapon updates and releases without replacing caller dictionary");
            }
            const std::array<std::uint32_t,3u> configured_size{0x3fa00000u,0x40200000u,0x40700000u};
            for (const auto resource : {0x218u,0x388u})
                for (unsigned field=0;field<3u;++field) base.Write(resource+(49u+field)*4u,configured_size[field]);
            const auto setup_named = [&](bool right) {
                return right ? pal4::BaseObjectNamedWeapon::SetupRight(base, weapon_model, "",
                                   directory_address, 0u, 0u, weapons)
                             : pal4::BaseObjectNamedWeapon::SetupLeft(base, weapon_model, "",
                                   directory_address, 0u, 0u, weapons);
            };
            Check(pal4::BaseObjectNamedWeapon::SetupRight(base,"MISSING_WEAPON_MODEL","",
                      directory_address,0u,0u,weapons) == 0u && weapons.OwnedCount() == 0u &&
                      base.Read(0x22cu) == 0u && base.Read(0x218u+36u)==0u &&
                      packages.AllocationCount() == package_allocations,
                  "missing original weapon closes package scope without publishing an owned resource");
            weapons.AddAndProcessScripts();
            bool invalid_path_rejected{};
            const char invalid_directory[] = "PALActor\\";
            try { (void)weapons.CreatePath(reinterpret_cast<std::uint32_t>(invalid_directory)); }
            catch (const std::invalid_argument&) { invalid_path_rejected = true; }
            Check(invalid_path_rejected && packages.AllocationCount() == package_allocations,
                  "named weapon exception closes the real package scope before retry");
            Check(setup_named(true) == 1u && weapons.OwnedCount() == 1u &&
                      packages.AllocationCount() == package_allocations,
                  "native named right weapon owns original WTA and closes its package scope");
            Check(setup_named(true) == 1u && weapons.OwnedCount() == 1u,
                  "repeated named weapon setup replaces and releases the preceding clump");
            Check(setup_named(false) == 1u && weapons.OwnedCount() == 2u &&
                      packages.AllocationCount() == package_allocations,
                  "native named left weapon shares the existing package manager without leaking scope");
            Check(!weapon_texture_modes.empty(),"material policy test has actual original textures");
            for (const auto& [texture,addressing] : weapon_texture_modes)
                Check((texture->filterAddressing & 0xffu) == 2u &&
                          (texture->filterAddressing & 0xffffff00u) == addressing,
                      "named loading applies original linear filter byte and preserves texture addressing");
            Check(RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema()) == caller_uv_dict,
                  "named loading restores the caller current UV dictionary across repeated loads and failure retry");
            for (const auto resource : {0x218u,0x388u}) {
                Check(base.Read(resource+36u)==1u && base.Read(resource+24u)==0u &&
                          base.Read(resource+32u)==0u,
                      "named weapon publishes original ready/mode/extra after complete load");
                Check((base.Read(resource+148u)!=0u) == weapon_has_uv &&
                          (base.Read(resource+152u)!=0u) == weapon_has_uv &&
                          base.Read(resource+156u) == 0x3f800000u,
                      "named resource publishes retained UV schema or honest absent-member state at unit time scale");
                const auto texture_name=base.Read(resource+16u);
                const auto effect_name=base.Read(resource+86u*4u);
                Check(base.Read(resource+78u*4u)==1u && effect_name &&
                          std::string_view(reinterpret_cast<const char*>(effect_name))==
                              "gamedata\\MatFX\\ReflectionTexture\\003.png",
                      "named WTA retains original effect texture path and enables environment map");
                Check(texture_name && std::string_view(reinterpret_cast<const char*>(texture_name))=="WTA1",
                      "original WTA uniform texture metadata resolves exact WTA1 filename stem");
                const auto size_member=weapon_archive.ReadMember((weapon_model+"\\extSize.scl").c_str());
                std::cout << "Original WTA resource texture=" << (texture_name ? reinterpret_cast<const char*>(texture_name) : "<none>")
                          << " extSize=" << (size_member ? size_member->size() : 0u) << '\n';
                std::array<std::uint32_t,3u> expected_size=configured_size;
                if (texture_name && size_member) {
                    Check(size_member->size()>=12u,"original size configuration contains three words");
                    std::memcpy(expected_size.data(),size_member->data(),12u);
                }
                for (unsigned field=0;field<3u;++field)
                    Check(base.Read(resource+(49u+field)*4u)==expected_size[field],
                          "named size configuration uses original archive bytes or preserves prior values when gated/absent");
                const auto scalar=[&](std::uint32_t field) {
                    return std::bit_cast<float>(base.Read(resource+field*4u));
                };
                Check(std::isfinite(scalar(55u)) && scalar(55u)>0.0f &&
                          std::isfinite(scalar(59u)) && scalar(59u)>0.0f,
                      "named resource contains finite positive local and world bounding spheres");
                for (const auto first : {60u,66u})
                    for (unsigned axis=0;axis<3u;++axis)
                        Check(std::isfinite(scalar(first+axis)) &&
                                  std::isfinite(scalar(first+axis+3u)) &&
                                  scalar(first+axis)>=scalar(first+axis+3u),
                              "named resource publishes ordered local and world boxes");
            }
            for (const auto offset : {0x22cu,0x39cu}) {
                auto* clump = reinterpret_cast<RpClump*>(base.Read(offset));
                auto* frame = RpClumpGetFrame(clump);
                unsigned prepared_atomics{};
                (void)RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* data) -> RpAtomic* {
                    auto* geometry=RpAtomicGetGeometry(atomic);
                    Check(atomic->pipeline==RpMatFXGetD3D9Pipeline(rpMATFXD3D9ATOMICPIPELINE),
                          "named WTA automatically selects SDK environment pipeline");
                    for (int i=0;i<RpGeometryGetNumMaterials(geometry);++i) {
                        auto* material=RpGeometryGetMaterial(geometry,i);
                        auto* texture=RpMatFXMaterialGetEnvMapTexture(material);
                        Check(RpMatFXMaterialGetEffects(material)==rpMATFXEFFECTENVMAP &&
                                  texture && std::string_view(RwTextureGetName(texture))=="003" &&
                                  std::bit_cast<std::uint32_t>(RpMatFXMaterialGetEnvMapCoefficient(material))==1028443341u &&
                                  RpMatFXMaterialGetEnvMapFrame(material)==nullptr &&
                                  !RpMatFXMaterialGetEnvMapFrameBufferAlpha(material),
                              "named WTA loads real003 reflection and original SDK environment parameters");
                    }
                    Check((RpGeometryGetFlags(geometry)&0x45u)==0x45u,
                          "named loader applies original geometry45 flags");
                    Check(geometry->mesh && (geometry->mesh->flags&0xffu)==rpMESHHEADERTRISTRIP &&
                              geometry->mesh->numMeshes>0u && geometry->mesh->totalIndicesInMesh>0u,
                          "named optimizer marks populated mesh headers as triangle strips");
                    ++*static_cast<unsigned*>(data);
                    return atomic;
                },&prepared_atomics);
                RpTriStripMeshCallBack strip_method{};
                void* strip_context=reinterpret_cast<void*>(1u);
                Check(prepared_atomics>0u && RpMeshGetTriStripMethod(&strip_method,&strip_context) &&
                          strip_method==RpBuildMeshGenerateCacheAwareTriStrip && strip_context==nullptr,
                      "named optimizer selects actual SDK cache-aware triangle strip callback");
                const auto* hand = pal4::NativeActorUserData::FindAtomic(clump,"hook_hand");
                const auto source = *RwFrameGetMatrix(RpAtomicGetFrame(hand));
                const std::array<RwV3d,3u> input{source.right,source.up,source.at};
                std::array<RwV3d,3u> output{};
                (void)RwV3dTransformVectors(output.data(),input.data(),3,RwFrameGetMatrix(frame));
                const float sign = offset == 0x22cu ? -1.0f : 1.0f;
                const std::array<RwV3d,3u> expected{{{sign,0,0},{0,0,1},{0,-sign,0}}};
                for (unsigned axis=0;axis<3u;++axis)
                    Check(std::fabs(output[axis].x-expected[axis].x)<0.001f &&
                              std::fabs(output[axis].y-expected[axis].y)<0.001f &&
                              std::fabs(output[axis].z-expected[axis].z)<0.001f,
                          "named native placement preserves original hand basis rotation");
                Check(RpClumpGetWorld(clump) == weapon_fixture.world &&
                          RwFrameGetParent(frame) == RpAtomicGetFrame(pal4::NativeActorUserData::FindAtomic(
                              reinterpret_cast<RpClump*>(base.Read(0x14u)),
                              offset == 0x22cu ? "hook_RHWeapon" : "hook_LHWeapon")),
                      "named weapon enters the correct world and actual actor hand hook");
            }
            const auto actor_name = base.Read(4u);
            const char linsha[] = "Player-LINSHA";
            base.Write(4u,reinterpret_cast<std::uint32_t>(linsha));
            Check(setup_named(false) == 1u && weapons.OwnedCount() == 2u,
                  "Linsha left-hand replacement retains bounded native ownership");
            auto* linsha_clump = reinterpret_cast<RpClump*>(base.Read(0x39cu));
            const auto* linsha_hand = pal4::NativeActorUserData::FindAtomic(linsha_clump,"hook_hand");
            const auto linsha_source = *RwFrameGetMatrix(RpAtomicGetFrame(linsha_hand));
            RwV3d linsha_up{};
            (void)RwV3dTransformVectors(&linsha_up,&linsha_source.up,1,
                RwFrameGetMatrix(RpClumpGetFrame(linsha_clump)));
            Check(std::fabs(linsha_up.x)<0.001f && std::fabs(linsha_up.y)<0.001f &&
                      std::fabs(linsha_up.z+1.0f)<0.001f,
                  "original Linsha left-hand branch rotates minus90 instead of plus90");
            base.Write(4u,actor_name);
            Check(setup_named(false) == 1u && weapons.OwnedCount() == 2u,
                  "ordinary actor left-hand orientation restores after Linsha branch");
            pal4::NativePlayerResetActivity loaded_reset(lifecycle, 0x7000u, weapons, materials);
            effects.bound = &loaded_reset;
            base.Write(0x534u, 1u); base.Write(0x538u, 1u);
            base.Write(0x53cu, 2u); base.Write(0x540u, 1u);
            base.Write(0x568u, 0x1234u); base.Write(0x56cu, 0x5678u);
            animation_memory.WriteDword(first_slot + 0xa1cu,
                reinterpret_cast<std::uint32_t>(original));
            (void)pal4::ScriptProcessAndCleanup::Process(first_slot, reload);
            Check(binding.Bound() && binding.Select("C01") &&
                      players.At(0u).ActionCount() == 56u && effects.cleanup_calls == 1u &&
                      base.Read(0x568u) == 0u && base.Read(0x56cu) == 0u &&
                      base.Read(0x538u) == 0u && base.Read(0x540u) == 0u &&
                      packages.AllocationCount() == package_allocations,
                  "same native save reload service restores the original actor");
            weapons.Cleanup(base);
            Check(base.Read(0x518u) == 0u && base.Read(0x51cu) == 0u &&
                      base.Read(0x22cu) == 0u && base.Read(0x39cu) == 0u &&
                      weapon_fixture.updated == std::vector<std::uint32_t>{0x520u,0x528u,0x534u,0x53cu} &&
                      weapon_fixture.displayed == std::vector<std::uint32_t>{0x4f8u,0x500u,0x508u,0x510u} &&
                      weapons.OwnedCount() == 0u && weapon_fixture.released.empty() &&
                      !RpClumpGetWorld(weapon_fixture.weapons[0]) &&
                      !RpClumpGetWorld(weapon_fixture.weapons[1]) &&
                      !RwFrameGetParent(RpClumpGetFrame(weapon_fixture.weapons[0])) &&
                      !RwFrameGetParent(RpClumpGetFrame(weapon_fixture.weapons[1])),
                  "model reset removes four weapons and repeat cleanup preserves detached borrowed ownership");
            Check(base.Read(0x218u+16u)==0u && base.Read(0x388u+16u)==0u &&
                      base.Read(0x218u+36u)==0u && base.Read(0x388u+36u)==0u &&
                      base.Read(0x218u+86u*4u)==0u && base.Read(0x388u+86u*4u)==0u &&
                      base.Read(0x218u+148u)==0u && base.Read(0x218u+152u)==0u &&
                      base.Read(0x388u+148u)==0u && base.Read(0x388u+152u)==0u &&
                      RtDictSchemaGetCurrentDict(RpUVAnimGetDictSchema()) == caller_uv_dict,
                  "named teardown clears borrowed UV views and preserves unrelated current dictionary");
            (void)caller_uv_schema.Cleanup(schema_runtime);
            for (const auto* target : {replacement, original}) {
                animation_memory.WriteDword(first_slot + 0xa1cu,
                    reinterpret_cast<std::uint32_t>(target));
                (void)pal4::ScriptProcessAndCleanup::Process(first_slot, reload);
                Check(binding.Bound() && binding.Select("C01") &&
                          binding.AdvanceFrame(0.01f) && weapons.OwnedCount() == 0u &&
                          std::string_view(reinterpret_cast<const char*>(base.Read(0xcu))) == target &&
                          packages.AllocationCount() == package_allocations,
                      "production reset repeats with real animation cleanup after equipment removal");
            }
            effects.weapons = nullptr;
            effects.bound = nullptr;
            std::array<pal4::NativeActorAnimationStateBinding*, 4> stable_bindings{};
            std::array<std::uint32_t, 4> saved_requested_models{};
            for (std::size_t index = 0; index < models.size(); ++index) {
                auto& role = player_slots.slots[index].base;
                if (index != 0u) {
                    auto& embedded = *reinterpret_cast<pal4::GameObjectEmbeddedAnimationState*>(
                        reinterpret_cast<std::uintptr_t>(&role) + 0x170u);
                    (void)pal4::GameObjectEmbeddedAnimation::InitializeState(embedded, 0u, lifecycle);
                }
                auto& role_binding = animations.For(role);
                stable_bindings[index] = &role_binding;
                if (!role_binding.Bound()) role_binding.Bind();
                Check(role_binding.Select("C01") &&
                          role_binding.Matches(role, players.At(index)),
                      "each original role owns the correct animation binding");
                saved_requested_models[index] = animation_memory.ReadDword(
                    reinterpret_cast<std::uint32_t>(&role) + 0xa1cu);
            }
            Check(stable_bindings[0] == &binding,
                  "four-player binding owner preserves the leader's borrowed address");
            bool foreign_rejected = false;
            try { pal4::BaseObjectState foreign; (void)animations.For(foreign); }
            catch (const std::invalid_argument&) { foreign_rejected = true; }
            Check(foreign_rejected, "player animation resolver rejects non-player objects");
            for (unsigned pass = 0; pass < 2u; ++pass) {
                for (std::size_t index = 0; index < models.size(); ++index) {
                    auto& role = player_slots.slots[index].base;
                    const auto slot = reinterpret_cast<std::uint32_t>(&role);
                    const auto target = models[pass == 0u ? (index + 1u) % models.size() : index];
                    animation_memory.WriteDword(slot + 0xa1cu,
                        reinterpret_cast<std::uint32_t>(target.data()));
                    (void)pal4::ScriptProcessAndCleanup::Process(slot, reload);
                    auto& role_binding = animations.For(role);
                    Check(&role_binding == stable_bindings[index] && role_binding.Bound() &&
                              role_binding.Select("C01") && role_binding.AdvanceFrame(0.01f) &&
                              std::string_view(reinterpret_cast<const char*>(role.Read(0xcu))) == target &&
                              role.Read(0x14u) == reinterpret_cast<std::uint32_t>(players.At(index).Main()) &&
                              animation_memory.ReadDword(role.Read(0x17cu) - 4u) ==
                                  players.At(index).ActionCount() &&
                              packages.AllocationCount() == package_allocations,
                          "four-role reload retains binding identity and installs target model clips");
                    for (std::size_t other = 0; other < models.size(); ++other)
                        Check(stable_bindings[other]->Bound() &&
                                  stable_bindings[other]->Matches(player_slots.slots[other].base,
                                                                  players.At(other)),
                              "reloading one player preserves every other role binding");
                }
            }
            for (std::size_t index = 0; index < models.size(); ++index)
                animation_memory.WriteDword(reinterpret_cast<std::uint32_t>(
                    &player_slots.slots[index]) + 0xa1cu, saved_requested_models[index]);
            animations.Release();
            animation_memory.WriteDword(first_slot + 0xa1cu, requested_model);
            binding.Release();
            Check(animation_memory.OutstandingAllocations() == 0u,
                  "composed original actor reload leaves no native animation allocations");
            packages.Shutdown();
        }
        Check(player_slots.slots[0].base.Read(8u) != 0u &&
                  std::string_view(reinterpret_cast<const char*>(
                      player_slots.slots[0].base.Read(8u))) == "PALActor\\101\\" &&
                  player_slots.slots[0].base.Read(0x544u) == saved_material_strings[0] &&
                  player_slots.slots[0].base.Read(0x548u) == saved_material_strings[1] &&
                  player_slots.slots[0].base.Read(0x54cu) == saved_material_strings[2] &&
                  player_slots.slots[0].base.Read(0x554u) == saved_material_strings[3] &&
                  player_slots.slots[0].base.Read(0x558u) == saved_material_strings[4] &&
                  player_slots.slots[0].base.Read(0x55cu) == saved_material_strings[5],
              "reload metadata restores slot strings while actor owner retains resource path");
        const auto& reload_base = player_slots.slots[0].base;
        Check(players.Load(first_slot,
                  reinterpret_cast<const char*>(reload_base.Read(4u)),
                  reinterpret_cast<const char*>(reload_base.Read(0xcu))) &&
                  players.Count() == 4u &&
                  std::string_view(reinterpret_cast<const char*>(
                      reload_base.Read(0xcu))) == "101" &&
                  player_slots.slots[0].base.Read(0x14u) ==
                      reinterpret_cast<std::uint32_t>(players.At(0).Main()),
              "reloading a player slot replaces its actor resources");
        const auto second_slot = reinterpret_cast<std::uint32_t>(
            &player_slots.slots[1]);
        Check(players.UnloadSlot(second_slot) && players.Count() == 3u &&
                  player_slots.slots[1].base.Read(4u) == 0u &&
                  player_slots.slots[1].base.Read(0xcu) == 0u &&
                  player_slots.slots[1].base.Read(0x14u) == 0u &&
                  player_slots.slots[1].base.Read(0x18u) == 0u &&
                  player_slots.slots[1].base.Read(0x24u) == 0u &&
                  player_slots.slots[1].base.Read(0x30u) == 0u &&
                  player_slots.slots[1].base.Read(0x530u) == 0u &&
                  player_slots.slots[1].base.Read(0x550u) == 0u,
              "single player teardown clears borrowed slot pointers");
        player_slots.slots[1].base.Write(0x14u, 0x1234u);
        Check(!players.Load(second_slot, "Player-LINSHA", "103") &&
                  players.Count() == 3u &&
                  player_slots.slots[1].base.Read(0x14u) == 0x1234u,
              "actor owner rejects another clump in a player slot");
        player_slots.slots[1].base.Write(0x14u, 0u);
        player_slots.slots[1].base.Write(0xcu, 0x1234u);
        Check(!players.Load(second_slot, "Player-LINSHA", "103") &&
                  players.Count() == 3u &&
                  player_slots.slots[1].base.Read(0xcu) == 0x1234u,
              "actor owner preserves a foreign model string");
        player_slots.slots[1].base.Write(0xcu, 0u);
        Check(players.Load(second_slot, "Player-LINSHA", "103") &&
                  players.Count() == 4u,
              "released player slot accepts its original Role model");
        ActorResetHost reset_host(players);
        pal4::NativePlayerActorResetRuntime reset_runtime(players, reset_host);
        auto& first_base = player_slots.slots[0].base;
        Check(pal4::BaseObjectActivity::Reset(first_base, reset_runtime) ==
                  0x1234u && players.Count() == 3u &&
                  first_base.Read(0x14u) == 0u &&
                  first_base.Read(4u) == 0u &&
                  first_base.Read(0xcu) == 0u &&
                  first_base.Read(0x30u) == 0u &&
                  first_base.Read(0x530u) == 0u &&
                  first_base.Read(0x550u) == 0u &&
                  reset_host.calls == std::vector<char>{
                      'A', 'O', 'D', 'd', 'N', 'G', 'C', 'P'},
              "rewritten player reset releases the owned actor in IDA order");
        reset_host.calls.clear();
        Check(pal4::BaseObjectActivity::Reset(first_base, reset_runtime) ==
                  0x1234u && players.Count() == 3u &&
                  reset_host.cleanup_calls == 2u &&
                  reset_host.calls == std::vector<char>{
                      'A', 'O', 'D', 'd', 'N', 'G', 'C', 'P'},
              "repeated player reset does not release the SDK actor twice");
        ActorSlotResetHost slot_reset(reset_runtime);
        pal4::PalActorControlState actor_control;
        actor_control.words[6] = 7u;
        pal4::IPalActorControlLoopRuntime actor_loop;
        pal4::PlayerManagerOperationsRuntimeStub operation_host;
        pal4::NativePlayerManagerResetRuntime manager_reset(
            player_slots, actor_control, slot_reset, actor_loop,
            operation_host);
        Check(pal4::PlayerManagerOperations::ResetAllPlayerStates(
                  reinterpret_cast<std::uint32_t>(&player_slots),
                  manager_reset) == 0u &&
                  players.Count() == 0u &&
                  reset_host.cleanup_calls == 6u &&
                  actor_control.words[6] == 0u,
              "manager reset releases all four original Role actor slots");
        for (const auto& slot : player_slots.slots)
            Check(slot.base.Read(0x14u) == 0u &&
                      slot.base.Read(0x30u) == 0u &&
                      slot.base.Read(0x550u) == 0u,
                  "manager reset clears every owned Clump pointer");
        players.Unload();
        Check(players.Count() == 0u && !players.At(0).Main() &&
                  player_slots.slots[0].base.Read(0x14u) == 0u &&
                  player_slots.slots[1].base.Read(0x14u) == 0u,
              "player owner releases actor resources before SDK shutdown");
        std::cout << "PASS: original actor animations, monster paths, motion and reset\n";
        desktop.Check("native actor and live scene save capture preserve desktop focus and cursor");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
