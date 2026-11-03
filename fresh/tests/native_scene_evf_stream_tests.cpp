#include "pal4/native_package_archive.h"
#include "pal4/native_scene_evf_assets.h"
#include "pal4/native_arena_evf_scene_entry.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_scene_evf_event_callbacks.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_scene_player_movement.h"
#include "pal4/native_new_game_script_session.h"
#include "pal4/native_renderware_dialog_mode.h"
#include "pal4/rw_plugin_8449_native.h"
#include "pal4/rw_plugin_8450_native.h"
#include "pal4/rw_plugin_8451_native.h"
#include "pal4/rw_plugin_8452_native.h"
#include "support/event_arena_runtime.h"

#include <windows.h>
#include <rpcollis.h>
#include <rwcore.h>
#include <rpworld.h>

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "support/native_test_display.h"

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t ObjectWord(const std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

std::string StateString(const pal4::SceneEvfRecordState& state,
                        const std::uint32_t offset) {
    const auto data = state.words[offset / 4u];
    const auto length = state.words[offset / 4u + 1u];
    const auto capacity = state.words[offset / 4u + 2u];
    Check(data != 0u && length < capacity &&
              *reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(data + length)) == '\0',
          "initialized EVF string buffer");
    return {reinterpret_cast<const char*>(
                static_cast<std::uintptr_t>(data)), length};
}

RpAtomic* VerifyIdentityRender(RpAtomic* atomic, void* context) {
    auto& count = *static_cast<std::uint32_t*>(context);
    const auto callback = RpAtomicGetRenderCallBack(atomic);
    Check(callback && callback(atomic) == atomic,
          "EVF Atomic uses original identity render callback");
    ++count;
    return atomic;
}

std::uint32_t ReadWord(RwStream* stream) {
    std::uint32_t value = 0u;
    Check(RwStreamRead(stream, &value, sizeof(value)) == sizeof(value),
          "EVF word is truncated");
    return value;
}

std::string ReadString(RwStream* stream) {
    const auto length = ReadWord(stream);
    Check(length != 0u && length < 256u, "EVF string length is invalid");
    std::string value(length, '\0');
    Check(RwStreamRead(stream, value.data(), length) == length,
          "EVF string is truncated");
    Check(value.back() == '\0', "EVF string is not terminated");
    value.pop_back();
    return value;
}

void InspectRecord(RwStream* stream, const char* name,
                   const char* function, const std::uint32_t kind,
                   const std::uint32_t field, const std::uint32_t properties,
                   const std::uint32_t expected_end) {
    Check(ReadString(stream) == name, "EVF event name");
    Check(ReadWord(stream) == kind, "EVF event kind");
    Check(ReadWord(stream) == field, "EVF effect field");
    Check(ReadString(stream).empty(), "EVF effect string");
    Check(ReadWord(stream) == 0u && ReadWord(stream) == 0u,
          "EVF effect words");
    Check(ReadString(stream) == "Q01", "EVF world");
    Check(ReadString(stream) == "N01", "EVF scene");
    Check(ReadString(stream) == function, "EVF script function");
    Check(ReadWord(stream) == properties, "EVF property count");
    std::array<std::uint8_t, 80u> property{};
    for (std::uint32_t i = 0u; i < properties; ++i)
        Check(RwStreamRead(stream, property.data(), property.size()) ==
                  property.size(), "EVF property body");
    Check(ReadWord(stream) == 1u && ReadWord(stream) == 0u,
          "EVF geometry flags");
    Check(RwStreamFindChunk(stream, rwID_CLUMP, nullptr, nullptr),
          "EVF Clump chunk");
    auto* clump = RpClumpStreamRead(stream);
    Check(clump != nullptr, "EVF Clump read");
    Check(RpClumpDestroy(clump), "EVF Clump release");
    std::cout << name << " ends at " << stream->Type.memory.position << '\n';
    Check(stream->Type.memory.position == expected_end,
          "RenderWare EVF stream position");
}

}  // namespace

int main(int argc, char** argv) {
    bool initialized = false;
    bool opened = false;
    bool started = false;
    HWND window = nullptr;
    pal4::NativeRenderWareDialogMode dialog_mode;
    try {
        Check(argc == 2, "original game root argument");
        pal4::NativePackageArchive archive;
        const auto package = (std::filesystem::path(argv[1]) / "gamedata" /
                              "scenedata.cpk").string();
        Check(archive.Open(package.c_str()), "open original scene archive");
        auto bytes = archive.ReadMember("q01\\N01\\N01.evf");
        Check(bytes && bytes->size() == 2810u, "read original N01 EVF");
        const auto display = pal4::test::TestDisplay();
        const auto& bounds = display.bounds;
        window = CreateWindowA("STATIC", "PAL4 EVF stream test",
                               WS_POPUP, bounds.left, bounds.top, 1, 1,
                               nullptr, nullptr,
                               GetModuleHandleA(nullptr), nullptr);
        Check(window != nullptr, "create hidden RenderWare window");
        initialized = RwEngineInit(nullptr, rwENGINEINITFREELISTS,
                                   4u * 1024u * 1024u) != 0;
        Check(initialized, "initialize RenderWare");
        Check(RpWorldPluginAttach(), "attach world plugin");
        Check(RpCollisionPluginAttach(), "attach collision plugin");
        Check(pal4::NativeRwPlugin8449Lifecycle::Attach(), "attach 8449");
        Check(pal4::NativeRwPlugin8450Lifecycle::Attach(), "attach 8450");
        Check(pal4::NativeRwPlugin8451Lifecycle::Attach(), "attach 8451");
        Check(pal4::NativeRwPlugin8452Lifecycle::Attach(), "attach 8452");
        Check(dialog_mode.PrepareBeforeOpen(), "prepare D3D9 dialog mode");
        RwEngineOpenParams parameters{window};
        opened = RwEngineOpen(&parameters) != 0;
        Check(opened, "open RenderWare engine");
        Check(dialog_mode.EnableBeforeStart(true), "enable D3D9 dialog mode");
        started = RwEngineStart() != 0;
        Check(started && dialog_mode.FinishAfterStart(),
              "start RenderWare engine");
        RwMemory memory{bytes->data(), static_cast<RwUInt32>(bytes->size())};
        auto* stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &memory);
        Check(stream != nullptr, "open EVF memory stream");
        try {
            Check(ReadWord(stream) == 2u, "EVF event count");
            InspectRecord(stream, "ev_Q01_N01_1", "func1002", 0u, 1u, 4u,
                          1083u);
            InspectRecord(stream, "ev_Q01_N01_3", "func2099", 2u, 5u, 8u,
                          2810u);
        } catch (...) {
            (void)RwStreamClose(stream, nullptr);
            throw;
        }
        Check(RwStreamClose(stream, nullptr), "close EVF stream");
        pal4::NativeSceneEvfAssets assets;
        Check(assets.Load(argv[1], "q01", "N01"),
              "load complete original EVF records");
        const auto* first = assets.Find("ev_Q01_N01_1");
        const auto* second = assets.Find("ev_Q01_N01_3");
        Check(assets.Size() == 2u && first && second &&
                  first->kind == 0u && first->effect_type == 1u &&
                  first->world == "Q01" && first->scene == "N01" &&
                  first->script_function == "func1002" &&
                  first->properties.size() == 4u && first->clump &&
                  first->atomic_count != 0u &&
                  first->geometry_flag1 == 1u &&
                  first->geometry_flag2 == 0u &&
                  second->kind == 2u && second->effect_type == 5u &&
                  second->world == "Q01" && second->scene == "N01" &&
                  second->script_function == "func2099" &&
                  second->properties.size() == 8u && second->clump &&
                  second->atomic_count != 0u &&
                  second->geometry_flag1 == 1u &&
                  second->geometry_flag2 == 0u,
              "retain original EVF properties and Clumps");
        const auto* manager = assets.Manager();
        Check(manager && manager->Address() != 0u &&
                  manager->FactoryCount() == 6u &&
                  manager->RecordCount() == 2u &&
                  manager->OutstandingAllocations() == 10u &&
                  manager->FindFactory(1u) == 2u &&
                  manager->FindFactory(5u) == 6u &&
                  manager->FindFactory(6u) == 0u &&
                  first->GeometryState().words[18u] == manager->Address() &&
                  second->GeometryState().words[18u] == manager->Address(),
              "EVF type objects resolve through the original manager tree");
        const auto record_array = ObjectWord(manager->Address() + 4u);
        Check(record_array != 0u &&
                  ObjectWord(manager->Address() + 8u) == record_array + 8u &&
                  ObjectWord(manager->Address() + 12u) == record_array + 8u &&
                  ObjectWord(manager->Address() + 20u) == 2u &&
                  ObjectWord(record_array) == first->RecordAddress() &&
                  ObjectWord(record_array + 4u) == second->RecordAddress(),
              "manager owns the original two-slot EVF record array");
        std::vector<std::uint32_t> visited;
        assets.VisitRecords([&](const std::uint32_t record) {
            visited.push_back(record);
            return true;
        });
        Check(visited == std::vector<std::uint32_t>{first->RecordAddress(),
                                                     second->RecordAddress()},
              "EVF manager visits records in original array order");
        visited.clear();
        assets.VisitRecords([&](const std::uint32_t record) {
            visited.push_back(record);
            return false;
        });
        Check(visited == std::vector<std::uint32_t>{first->RecordAddress()},
              "EVF manager stops when callback returns zero");
        pal4::NativeSceneMovementAsset movement;
        Check(movement.Load(argv[1], "Q01", "N01") &&
                  movement.RegisterEventClump(
                      first->clump, first->GeometryState().words[16u], false) &&
                  movement.RegisteredObjectAtomics() == first->atomic_count &&
                  movement.RegisteredSecondaryObjectAtomics() == 0u,
              "type-1 EVF registers in the primary movement container");
        const auto event_resource = first->GeometryState().words[16u];
        pal4::NativeNewGameScriptSession event_scripts(argv[1], {});
        pal4::test::EventArenaRuntime event_arena;
        pal4::NativeGameScriptServices event_services{};
        event_services.arena = &event_arena;
        event_scripts.BindEventServices(event_services);
        pal4::NativeSceneEvfEventCallbacks event_callbacks(
            assets, movement, event_scripts);
        pal4::NativeConfigCompositeCollisionRuntime collision(&event_callbacks);
        Check(assets.FindByResource(event_resource) == first &&
                  movement.IsEventResourceRegistered(first->clump,
                                                     event_resource),
              "EVF resource resolves to its registered owner");
        pal4::NativeScenePlayerMovement player_movement(movement);
        player_movement.BindEventCallbacks(&event_callbacks);
        auto& adapter = *reinterpret_cast<pal4::PalMovementAdapterState*>(
            static_cast<std::uintptr_t>(player_movement.Address()));
        const auto triangle = first->Triangles().front();
        pal4::CollisionPoint3 event_point{};
        for (const auto index : triangle.indices) {
            Check(index < first->Vertices().size(),
                  "EVF collision triangle indexes original vertices");
            for (std::size_t axis = 0u; axis < 3u; ++axis)
                event_point[axis] += std::bit_cast<float>(
                    first->Vertices()[index].words[axis]) / 3.0f;
        }
        pal4::NativeNewGameScriptSession primary_scripts(argv[1], {});
        pal4::test::EventArenaRuntime primary_arena;
        pal4::NativeGameScriptServices primary_services{};
        primary_services.arena = &primary_arena;
        primary_scripts.BindEventServices(primary_services);
        pal4::NativeSceneEvfEventCallbacks primary_callbacks(
            assets, movement, primary_scripts);
        pal4::NativeScenePlayerMovement primary_movement(movement);
        primary_movement.BindEventCallbacks(&primary_callbacks);
        pal4::ConfigCollisionPosition primary_lower{
            event_point[0], event_point[1], event_point[2], 2.0f};
        auto primary_upper = primary_lower;
        pal4::CollisionPoint3 primary_displacement{};
        primary_movement.ResolveCollision(primary_lower, primary_upper,
                                          primary_displacement, 0xffffffffu);
        Check(primary_scripts.EventStatus() ==
                  pal4::NativeScriptFrameStatus::queued,
              "type-1 EVF collision queues its script on the primary child");
        Check(primary_scripts.TickEventUntilBoundary(
                  0.0f, nullptr, [&primary_arena] {
                      return primary_arena.loading;
                  }) == pal4::NativeScriptFrameStatus::suspended &&
                  primary_scripts.EventError().empty() &&
                  primary_arena.requests == 1 &&
                  primary_arena.world == "Q01" &&
                  primary_arena.area == "Q01",
              "primary EVF collision requests the original Q01/Q01 Loading");
        primary_scripts.ReleaseServices();
        Check(movement.RegisterEventClump(first->clump, event_resource, true) &&
                  movement.RegisteredSecondaryObjectAtomics() ==
                      first->atomic_count,
              "type-0 branch can register the same Clump in both containers");
        player_movement.SelectAndUpdate(
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(
                event_point.data())), 10.0f, 10.0f, 0u);
        Check(adapter.words[2u] == adapter.words[1u],
              "original EVF floor selects the secondary movement child");
        pal4::ConfigCollisionPosition lower{
            event_point[0], event_point[1], event_point[2], 2.0f};
        auto upper = lower;
        pal4::CollisionPoint3 displacement{};
        player_movement.ResolveCollision(lower, upper, displacement,
                                         0xffffffffu);
        Check(event_scripts.EventStatus() ==
                  pal4::NativeScriptFrameStatus::queued,
              "secondary movement collision queues the original EVF script");
        Check(event_scripts.TickEventUntilBoundary(
                  0.0f, nullptr, [&event_arena] {
                      return event_arena.loading;
                  }) == pal4::NativeScriptFrameStatus::suspended &&
                  event_scripts.EventError().empty() &&
                  event_arena.requests == 1 &&
                  event_arena.world == "Q01" &&
                  event_arena.area == "Q01",
              "EVF collision executes func1002 through the Loading request");
        collision.NotifyEnter(event_resource, 1u, movement.MutableState());
        collision.NotifyLeave(event_resource, movement.MutableState());
        Check(event_scripts.EventStatus() ==
                  pal4::NativeScriptFrameStatus::suspended,
              "original local-floor collision queues its Q01 event script once");
        event_scripts.ReleaseServices();
        pal4::NativeNewGameScriptSession stop_scripts(argv[1], {});
        pal4::NativeSceneEvfEventCallbacks stop_callbacks(assets, movement, stop_scripts);
        movement.MutableState().words[22u] = 0u;
        stop_callbacks.Enter(event_resource, 0u);
        Check(stop_scripts.EventStatus() == pal4::NativeScriptFrameStatus::queued &&
                  movement.MutableState().words[22u] == 1u,
              "successful local-floor push sets original primary collision scan-stop flag");
        movement.MutableState().words[22u] = 0u;
        stop_callbacks.Enter(event_resource, 0u);
        Check(movement.MutableState().words[22u] == 0u &&
                  stop_scripts.EventStatus() == pal4::NativeScriptFrameStatus::queued,
              "duplicate local-floor entry preserves context and does not set scan-stop again");
        stop_scripts.ReleaseServices();
        Check(movement.UnregisterEventClump(first->clump, true) &&
                  movement.UnregisterEventClump(first->clump, false) &&
                  movement.RegisteredObjectAtomics() == 0u &&
                  movement.RegisteredSecondaryObjectAtomics() == 0u,
              "event Clump releases both movement containers");
        bool rejected_stale_resource = false;
        try {
            collision.NotifyEnter(event_resource, 0u, movement.MutableState());
        } catch (const std::invalid_argument&) {
            rejected_stale_resource = true;
        }
        Check(rejected_stale_resource,
              "released EVF resource cannot dispatch a collision callback");
        movement.Unload();
        for (std::uint32_t type = 0u; type < 6u; ++type)
            Check(manager->FindFactory(type) == type + 1u,
                  "all six original EVF type keys are registered");
        for (const auto* record : {first, second}) {
            std::uint32_t callback_count = 0u;
            Check(RpClumpForAllAtomics(
                      record->clump, VerifyIdentityRender,
                      &callback_count) != nullptr &&
                      callback_count == record->atomic_count,
                  "bind identity callback on every event Atomic");
        }
        Check(first->Vertices().size() == 4u &&
                  first->Indices().size() == 12u &&
                  first->Triangles().size() == 4u &&
                  first->GeometryAllocationCount() == 11u &&
                  second->Vertices().size() == 8u &&
                  second->Indices().size() == 48u &&
                  second->Triangles().size() == 16u &&
                  second->GeometryAllocationCount() == 11u &&
                  first->Vertices()[0u].words[0u] ==
                      first->properties[0u].words[0u] &&
                  second->Vertices()[0u].words[2u] ==
                      second->properties[0u].words[2u],
              "rewritten EVF geometry owns original event triangles");
        for (const auto* record : {first, second}) {
            const auto& state = record->GeometryState();
            const auto properties = state.words[21u];
            const auto list = state.words[39u];
            Check(StateString(state, 8u) == record->effect_name &&
                      StateString(state, 28u) == record->world &&
                      StateString(state, 40u) == record->scene &&
                      StateString(state, 52u) == record->script_function &&
                      StateString(state, 104u) == record->name &&
                      state.words[5u] == record->effect_field2 &&
                      state.words[6u] == record->effect_field3 &&
                      state.words[17u] == static_cast<std::uint32_t>(
                          reinterpret_cast<std::uintptr_t>(record->clump)) &&
                      state.words[19u] == 1u &&
                      properties != 0u &&
                      properties != static_cast<std::uint32_t>(
                          reinterpret_cast<std::uintptr_t>(
                              record->properties.data())) &&
                      state.words[22u] - properties ==
                          record->properties.size() * 80u &&
                      std::memcmp(reinterpret_cast<const void*>(
                          static_cast<std::uintptr_t>(properties)),
                          record->properties.data(),
                          record->properties.size() * 80u) == 0 &&
                      list != 0u && ObjectWord(list) == list &&
                      ObjectWord(list + 4u) == list &&
                      state.words[40u] == 0u,
                  "original EVF fields occupy a complete initialized record");
        }
        const auto first_type = first->GeometryState().words[16u];
        const auto second_type = second->GeometryState().words[16u];
        Check(first_type != 0u && second_type != 0u &&
                  ObjectWord(first_type) == 0x8493c0u &&
                  ObjectWord(second_type) == 0x8493d4u &&
                  ObjectWord(first_type + 4u) ==
                      static_cast<std::uint32_t>(
                          reinterpret_cast<std::uintptr_t>(
                              &first->GeometryState().words[1u])) &&
                  ObjectWord(second_type + 4u) ==
                      static_cast<std::uint32_t>(
                          reinterpret_cast<std::uintptr_t>(
                              &second->GeometryState().words[1u])),
              "rewritten factories bind both EVF type objects");
        RwMemory truncated_memory{bytes->data() + sizeof(std::uint32_t),
                                  sizeof(std::uint32_t)};
        auto* truncated_stream = RwStreamOpen(
            rwSTREAMMEMORY, rwSTREAMREAD, &truncated_memory);
        Check(truncated_stream != nullptr, "open truncated EVF stream");
        pal4::NativeSceneEvfManagerOwner partial_manager;
        bool truncated_rejected = false;
        try {
            pal4::NativeSceneEvfRecord incomplete;
            (void)incomplete.ReadFromStream(
                truncated_stream,
                {bytes->data() + sizeof(std::uint32_t),
                 sizeof(std::uint32_t)},
                partial_manager);
        } catch (const std::runtime_error&) {
            truncated_rejected = true;
        }
        Check(RwStreamClose(truncated_stream, nullptr) &&
                  truncated_rejected &&
                  partial_manager.OutstandingAllocations() == 9u,
              "truncated EVF record releases partially initialized state");
        Check(!assets.Load(argv[1], "..", "N01") && assets.Size() == 2u,
              "invalid reload preserves owned event records");
        Check(assets.Load(argv[1], "q01", "N01") &&
                  assets.Size() == 2u &&
                  assets.Manager()->FactoryCount() == 6u &&
                  assets.Find("ev_Q01_N01_1") != nullptr &&
                  assets.Find("ev_Q01_N01_3") != nullptr,
              "successful reload replaces records before their factory tree");
        {
            pal4::NativePackageArchive scenes;
            const auto package = (std::filesystem::path(argv[1]) /
                                  "gamedata" / "scenedata.cpk").string();
            Check(scenes.Open(package.c_str()) &&
                      scenes.Find("M06\\1D\\1D.evf") < 0,
                  "original M06/1D has no section EVF");
            Check(assets.Load(argv[1], "M06", "1D") &&
                      assets.LastError().empty() && assets.Size() == 0u &&
                      assets.Manager() != nullptr &&
                      assets.Manager()->FactoryCount() == 6u &&
                      assets.Find("ev_Q01_N01_1") == nullptr,
                  "absent EVF retires previous records and retains factories");
            std::size_t visits = 0u;
            assets.VisitRecords([&](std::uint32_t) { ++visits; return true; });
            Check(visits == 0u && assets.Load(argv[1], "q01", "N01") &&
                      assets.Size() == 2u,
                  "empty EVF manager visits safely and reloads real records");
        }
        assets.Clear();
        Check(assets.Size() == 0u && assets.Manager() == nullptr &&
                  assets.Find("ev_Q01_N01_1") == nullptr,
              "release both original EVF Clumps");
        pal4::NativeSceneEvfManagerOwner lifecycle;
        Check(lifecycle.OutstandingAllocations() == 9u,
              "EVF manager owns its factory tree");
        lifecycle.Clear();
        Check(lifecycle.Address() == 0u &&
                  lifecycle.OutstandingAllocations() == 0u &&
                  lifecycle.FactoryCount() == 0u &&
                  lifecycle.FindFactory(1u) == 0u,
              "EVF manager releases nodes, sentinels and instance");
        {
            // Original Clumps and real movement/camera owners, with only the
            // arena's world-ready gate supplied by an explicit fixture.
            std::array<std::uint32_t, 13> arena{};
            arena[12] = 1u;
            const auto arena_address = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(arena.data()));
            pal4::NativeCameraManagerOwner cameras;
            (void)cameras.Construct();
            pal4::NativeSceneMovementAsset scene_movement;
            pal4::NativeSceneEvfAssets scene_events;
            unsigned observed_types{};
            for (const auto* scene : {"1", "1Y", "1"}) {
                Check(scene_movement.Load(argv[1], "M03", scene) &&
                          scene_events.Load(argv[1], "M03", scene),
                      "load original M03 daytime/night EVF and movement");
                scene_events.VisitRecords([&](std::uint32_t record) {
                    const auto type = ObjectWord(record + 4u);
                    Check(type < 6u, "original M03 EVF type range");
                    observed_types |= 1u << type;
                    return true;
                });
                Check(pal4::NativeArenaEvfSceneEntry::EnterAll(
                          scene_events, arena_address, scene_movement, &cameras) ==
                          scene_events.Size(),
                      "register original M03 event Clumps");
                pal4::NativeArenaEvfSceneEntry::LeaveAll(
                    scene_events, arena_address, scene_movement, &cameras);
                scene_events.VisitRecords([&](std::uint32_t record) {
                    Check(ObjectWord(record + 76u) == 0u,
                          "departed M03 event is inactive");
                    return true;
                });
                Check(scene_movement.SceneBorrowerCount() == 0u &&
                          cameras.SceneClumpCount() == 0u,
                      "M03 leave retires all movement/camera event borrowers");
                scene_events.Clear();
                scene_movement.Unload();
            }
            Check((observed_types & ((1u << 2u) | (1u << 3u))) != 0u,
                  "M03 resources exercise the previously rejected EVF types");
            cameras.Destroy();
        }
        (void)RwEngineStop();
        (void)RwEngineClose();
        (void)RwEngineTerm();
        (void)DestroyWindow(window);
        std::cout << "original EVF stream passed\n";
        return 0;
    } catch (const std::exception& error) {
        if (started) (void)RwEngineStop();
        if (opened) (void)RwEngineClose();
        if (initialized) (void)RwEngineTerm();
        if (window) (void)DestroyWindow(window);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
