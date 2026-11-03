#include "pal4/native_camera_file_catalog.h"
#include "pal4/native_camera_task_playback.h"
#include "pal4/native_camera_task_container.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_script_camera_sequence_runtime.h"
#include "pal4/script_camera_sequences.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class CameraResources final : public pal4::INativeCameraObjectResources {
public:
    class Geometry final : public pal4::ICameraTaskGeometry {
    public:
        void ClampDirection(pal4::CameraTaskVector&) override {}
        void Normalize(pal4::CameraTaskVector&) override {}
        pal4::CameraTaskQuaternion ToQuaternion(
            const pal4::CameraTaskBasis&) override { return {}; }
        pal4::CameraTaskVector ToEuler(
            const pal4::CameraTaskQuaternion&) override { return {}; }
    } geometry;
    std::uint32_t matrix_updates{};
    pal4::ICameraTaskGeometry* TaskGeometry() override { return &geometry; }
    pal4::CameraRenderRect RenderRect() override {
        return {0, 0, 640, 480};
    }
    void LogInitialization(bool) override {}
    bool Initialize(pal4::CameraSystemObjectState&,
                    pal4::CameraDataState& data,
                    const pal4::CameraSetupParameters&) override {
        data.At(0u) = 0x1000u;
        return true;
    }
    void Destroy(pal4::CameraSystemObjectState&,
                 pal4::CameraDataState&) noexcept override {}
    void SetFarClip(std::uint32_t, float) override {}
    float MinimumDistance() override { return 1.0f; }
    float MaximumDistance() override { return 1000.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState&,
                               std::uint32_t) override {
        ++matrix_updates;
        return 1u;
    }
};

void CheckNativePlayback(const pal4::NativeCameraFileCatalog& records) {
    pal4::NativeCameraManagerOwner cameras;
    (void)cameras.Construct();
    CameraResources resources;
    Check(cameras.InitializeCameras(resources),
          "initialize native camera for playback");
    pal4::NativeCameraTaskPlayback playback(cameras, resources, records);
    const auto* record = records.Find("001");
    Check(record && playback.Create() != 0u &&
              playback.OutstandingAllocations() == 1u,
          "allocate original 0x98-byte camera task");
    playback.Configure(cameras.Object(0u).camera, record->Address());
    Check(playback.Enter() && playback.Completed() &&
              !playback.Advance(0.1f) && resources.matrix_updates == 5u &&
              cameras.Data(0u).At(0x74u) == record->image[3u] &&
              cameras.Data(0u).At(0x78u) == record->image[4u] &&
              cameras.Data(0u).At(0x7cu) == record->image[5u],
          "original 001 immediate camera record updates owned camera data");
    playback.Destroy();
    Check(playback.OutstandingAllocations() == 0u,
          "camera task releases its native allocation");
    const auto* interpolated = records.Find("002");
    Check(interpolated && interpolated->image[12u] == 1u,
          "original 002 camera record uses target-position anchor");
    const auto start_x = std::bit_cast<float>(cameras.Data(0u).At(0x74u));
    const auto end_x = std::bit_cast<float>(interpolated->image[3u]);
    (void)playback.Create();
    playback.Configure(cameras.Object(0u).camera, interpolated->Address());
    Check(playback.Enter() && !playback.Completed(),
          "interpolated camera task enters without completing immediately");
    Check(playback.Advance(2.25f) && !playback.Completed() &&
              std::bit_cast<float>(*reinterpret_cast<const std::uint32_t*>(
                  static_cast<std::uintptr_t>(playback.Address() + 0x40u))) ==
                  2.25f &&
              std::abs(std::bit_cast<float>(cameras.Data(0u).At(0x74u)) -
                       (start_x + end_x) * 0.5f) < 0.001f,
          "original 002 camera record reaches its midpoint");
    Check(playback.Advance(2.25f) && playback.Completed() &&
              std::bit_cast<float>(*reinterpret_cast<const std::uint32_t*>(
                  static_cast<std::uintptr_t>(playback.Address() + 0x40u))) ==
                  4.5f &&
              std::abs(std::bit_cast<float>(cameras.Data(0u).At(0x74u)) -
                       end_x) < 0.001f && !playback.Advance(0.1f),
          "original 002 camera record reaches its endpoint and stops");
    playback.Destroy();
    Check(playback.OutstandingAllocations() == 0u,
          "interpolated camera task releases its native allocation");
    (void)playback.Create();
    playback.Configure(cameras.Object(0u).camera, record->Address());
    pal4::NativeCameraTaskContainer tasks(playback);
    tasks.Attach(static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&cameras.Object(0u))));
    tasks.Clear();
    tasks.Add(playback.Address(), "synchroCameraRun-CLD", {3u, 0u, 0u});
    tasks.MarkProcessed();
    Check(!tasks.Completed() && !tasks.Advance(0.0f) &&
              playback.Completed() && !tasks.Advance(0.0f) &&
              tasks.Advance(0.0f) && tasks.Completed() &&
              playback.Address() == 0u &&
              playback.OutstandingAllocations() == 0u,
          "original task container enters, finishes and releases camera task");
    tasks.Destroy();
    Check(tasks.OutstandingAllocations() == 0u,
          "camera task container releases sentinel and task text");
    cameras.Destroy();
}

void CheckNativePrepare(const std::filesystem::path& root) {
    pal4::NativeCameraManagerOwner cameras;
    (void)cameras.Construct();
    CameraResources resources;
    Check(cameras.InitializeCameras(resources), "initialize camera owner");
    pal4::NativeGameArenaManagerOwner arena;
    (void)arena.Construct();
    static constexpr char world[] = "Q01";
    static constexpr char scene[] = "N01";
    auto* fields = reinterpret_cast<std::uint32_t*>(
        static_cast<std::uintptr_t>(arena.Address()));
    fields[2u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(world));
    fields[3u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(scene));
    {
        pal4::NativeScriptCameraSequenceRuntime runtime(
            root, cameras, arena, resources, [] { return true; });
        pal4::ScriptCameraSequences sequences(runtime);
        Check(sequences.Prepare("MC001") && runtime.RecordCount() == 2u,
              "prepare Q01 camera file through scene package");
        const auto manager = runtime.CloudManager();
        const auto first = runtime.FindCloud(manager, "001");
        Check(first != 0u && runtime.FindCloud(manager, "absent") == 0u &&
                  runtime.FirstCloud(manager) != 0u &&
                  runtime.NextCloud(manager) != 0u &&
                  runtime.NextCloud(manager) == 0u,
              "camera records support name lookup and ordered traversal");
        Check(!sequences.Prepare("MISSING") && runtime.RecordCount() == 2u &&
                  runtime.FindCloud(manager, "001") == first,
              "missing camera file preserves prepared records");
        const auto active = runtime.ActiveCameraObject(cameras.Address());
        const auto task = runtime.CreateCameraTask();
        runtime.AddCameraTask(runtime.TaskContainer(active), task,
                              "synchroCameraRun-CLD", {3u, 0u, 0u});
        runtime.ApplyCameraRecord(task, runtime.CameraInternalId(active), first);
        runtime.MarkProcessed(active);
        Check(!runtime.Waiting() && runtime.ActiveTask() == task &&
                  !runtime.Tick(0.0f) && !runtime.Tick(0.0f) &&
                  !runtime.Tick(0.0f) && runtime.ActiveTask() == 0u &&
                  runtime.CameraCompleted(active),
              "camera task advances on frames without a script wait");
        Check(sequences.Prepare("MC001") && sequences.RunCircle(1u) &&
                  runtime.Waiting() && runtime.ActiveTask() != 0u &&
                  cameras.Object(0u).tasks[2u] == 2u,
              "original two-record camera sequence creates distinct tasks");
        const auto retained_task = runtime.ActiveTask();
        runtime.ClearHandlers();
        Check(!runtime.Waiting() && !runtime.BlocksScript() &&
                  runtime.ActiveTask() == retained_task,
              "camera handler cleanup preserves the separately owned active task");
        runtime.ClearHandlers();
        Check(sequences.CreateWaitHandler() != 0u && runtime.Waiting(),
              "retained camera task accepts a new wait after cleanup");
        for (int frame = 0; frame < 80 && runtime.Waiting(); ++frame)
            (void)runtime.Tick(0.5f);
        const auto second = runtime.FindCloud(manager, "002");
        Check(!runtime.Waiting() && runtime.ActiveTask() == 0u &&
                  runtime.CameraCompleted(active) && second != 0u &&
                  std::abs(std::bit_cast<float>(cameras.Data(0u).At(0x74u)) -
                           *reinterpret_cast<const float*>(
                               static_cast<std::uintptr_t>(second + 0x0cu))) <
                      0.001f,
              "original camera sequence completes and releases both tasks");
        Check(sequences.RunCircle(1u) && runtime.Waiting() &&
                  cameras.Object(0u).tasks[2u] == 2u,
              "populate tasks and blocking wait before scene reset");
        // Include an allocated task which has not reached AddCameraTask yet.
        Check(runtime.CreateCameraTask() != 0u,
              "allocate unqueued task before scene reset");
        runtime.ResetSceneSequences();
        Check(!runtime.Waiting() && !runtime.BlocksScript() &&
                  runtime.ActiveTask() == 0u && runtime.RecordCount() == 0u &&
                  runtime.OutstandingAllocations() == 0u &&
                  cameras.Object(0u).tasks[1u] == 0u &&
                  cameras.Object(0u).tasks[2u] == 0u &&
                  runtime.FindCloud(manager, "001") == 0u &&
                  runtime.FirstCloud(manager) == 0u && !runtime.Tick(0.1f),
              "scene reset retires queued and unqueued borrowers before records");
        runtime.ResetSceneSequences();
        Check(runtime.OutstandingAllocations() == 0u &&
                  sequences.Prepare("MC001") && runtime.RecordCount() == 2u &&
                  sequences.RunCircle(1u),
              "repeated reset preserves reusable manager and reloads camera file");
        for (int frame = 0; frame < 80 && runtime.Waiting(); ++frame)
            (void)runtime.Tick(0.5f);
        Check(!runtime.Waiting() && runtime.ActiveTask() == 0u &&
                  runtime.CameraCompleted(active),
              "reloaded camera sequence completes in the same runtime");
        runtime.ResetSceneSequences();
        Check(runtime.OutstandingAllocations() == 0u,
              "reloaded camera sequence resets without retained allocations");
    }
    Check(cameras.Object(0u).tasks[1u] == 0u,
          "camera sequence cleanup releases the task sentinel");
    fields[2u] = 0u;
    fields[3u] = 0u;
    arena.Destroy();
    cameras.Destroy();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) return EXIT_FAILURE;
    pal4::NativePackageArchive archive;
    const std::string path = std::string(argv[1]) +
                             "/gamedata/scenedata.cpk";
    Check(archive.Open(path.c_str()), "open original scene package");
    const auto member = archive.ReadLoadedMember("Q01\\N01\\MC001.cam");
    Check(member && member->size() == 628u,
          "read original Q01/N01 camera file");
    pal4::NativeCameraFileCatalog cameras;
    Check(cameras.Parse(*member) && cameras.Count() == 2u,
          "parse both original camera records and embedded XML");
    const auto* first = cameras.Find("001");
    Check(first && first->Address() != 0u &&
              first->xml.Root().name == "ScriptCameraLocalDataCache" &&
              first->image[1u] == 3u,
          "first camera record retains native fields and XML");
    const auto* second = cameras.Find("002");
    Check(first->image[13u] == 0x40000000u &&
              first->image[14u] == 1u && first->image[12u] == 1u &&
              second && second->image[13u] == 0x40900000u &&
              second->image[14u] == 0u && second->image[12u] == 1u,
          "Q01 camera records retain original timing and playback modes");
    Check(!cameras.Find("missing"), "unknown camera name is absent");
    Check(archive.Close(), "close original scene package");
    CheckNativePlayback(cameras);
    CheckNativePrepare(argv[1]);
    return EXIT_SUCCESS;
}
