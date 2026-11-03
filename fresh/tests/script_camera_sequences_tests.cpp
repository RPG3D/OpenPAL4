#include "pal4/script_camera_sequences.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptCameraSequenceRuntime {
public:
    std::vector<std::string> calls;
    pal4::CameraSequenceWaitState state{};
    std::string_view expected_camera = "opening";
    std::uint32_t load_result = 1u;
    bool cloud_exists = true;
    bool allocation_fails = false;
    bool task_exists = true;
    bool completed = false;
    bool circle_exists = true;
    bool second_current_exists = true;
    std::uint32_t next_calls = 0u;
    std::uint32_t task_adds = 0u;

    std::uint32_t CameraSubsystem() override {
        calls.emplace_back("camera subsystem");
        return 0x4000u;
    }
    std::uint32_t ActiveCameraObject(std::uint32_t subsystem) override {
        Check(subsystem == 0x4000u, "active camera subsystem");
        calls.emplace_back("active camera");
        return 0x5000u;
    }
    void ClearCameraCollection(std::uint32_t camera) override {
        Check(camera == 0x5000u, "clear active camera collection");
        calls.emplace_back("clear collection");
    }
    std::string_view ArenaScene() override {
        calls.emplace_back("arena scene");
        return "scene01";
    }
    std::string_view ArenaWorld() override {
        calls.emplace_back("arena world");
        return "world01";
    }
    std::uint32_t CloudManager() override {
        calls.emplace_back("cloud manager");
        return 0x6000u;
    }
    std::uint32_t LoadCameraFile(
        std::uint32_t manager, std::string_view world,
        std::string_view scene, std::string_view camera_name) override {
        Check(manager == 0x6000u && world == "world01" &&
                  scene == "scene01" && camera_name == expected_camera,
              "camera file operands");
        calls.emplace_back("load camera");
        return load_result;
    }
    std::uint32_t FindCloud(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x6000u && name == expected_camera,
              "camera record lookup");
        calls.emplace_back("find cloud");
        return cloud_exists ? 0x7000u : 0u;
    }
    std::uint32_t FirstCloud(std::uint32_t manager) override {
        Check(manager == 0x6000u, "first camera manager");
        calls.emplace_back("first cloud");
        const auto count = static_cast<std::uint32_t>(
            std::count(calls.begin(), calls.end(), "first cloud"));
        return circle_exists && (count == 1u || second_current_exists) ?
            0x7000u : 0u;
    }
    std::uint32_t NextCloud(std::uint32_t manager) override {
        Check(manager == 0x6000u, "next camera manager");
        calls.emplace_back("next cloud");
        return ++next_calls == 1u ? 0x7100u : 0u;
    }
    pal4::CameraSequenceControl ReadCloudControl(
        std::uint32_t record) override {
        Check(record == 0x7000u || record == 0x7100u,
              "camera control record");
        calls.emplace_back("read control");
        return record == 0x7000u ?
            pal4::CameraSequenceControl{9u, 1u} :
            pal4::CameraSequenceControl{12u, 0u};
    }
    pal4::CameraSequenceWaitState* AllocateWait(
        std::uint32_t size) override {
        Check(size == 0x20u, "camera wait allocation size");
        calls.emplace_back("allocate wait");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x8000u;
    }
    void ConstructWait(pal4::CameraSequenceWaitState& candidate,
                       std::uint8_t mode, std::uint32_t queue) override {
        Check(&candidate == &state && (mode == 7u || mode == 1u) &&
                  queue == 0x8054u,
              "camera wait construction");
        calls.emplace_back("construct wait");
    }
    std::uint32_t CreateCameraTask() override {
        calls.emplace_back("create task");
        return task_exists ? 0xa000u : 0u;
    }
    std::uint32_t TaskContainer(std::uint32_t camera) override {
        Check(camera == 0x5000u, "camera task container receiver");
        calls.emplace_back("task container");
        return 0xb000u;
    }
    void AddCameraTask(
        std::uint32_t container, std::uint32_t task,
        std::string_view name,
        const std::array<std::uint32_t, 3>& options) override {
        Check(container == 0xb000u && task == 0xa000u &&
                  name == "synchroCameraRun-CLD" &&
                  (options == std::array<std::uint32_t, 3>{3u, 0u, 0u} ||
                   options == std::array<std::uint32_t, 3>{1u, 0u,
                                                             task_adds == 0u ?
                                                                 0u : 12u}),
              "single camera task options and IDA task name");
        calls.emplace_back("add task");
        ++task_adds;
    }
    std::uint32_t CameraInternalId(std::uint32_t camera) override {
        Check(camera == 0x5000u, "camera internal ID receiver");
        calls.emplace_back("camera ID");
        return 0x9000u;
    }
    void ApplyCameraRecord(std::uint32_t task,
                           std::uint32_t camera_id,
                           std::uint32_t record) override {
        Check(task == (task_exists ? 0xa000u : 0u) &&
                  camera_id == 0x9000u &&
                  (record == 0x7000u || record == 0x7100u),
              "single camera record properties");
        calls.emplace_back("apply record");
    }
    void MarkProcessed(std::uint32_t camera) override {
        Check(camera == 0x5000u, "camera processed receiver");
        calls.emplace_back("processed");
    }
    void FlushWait(pal4::CameraSequenceWaitState& candidate) override {
        Check(&candidate == &state &&
                  (state.vtable_address == 0x8495b8u ||
                   state.vtable_address == 0x8495c4u ||
                   state.vtable_address == 0x8495d0u),
              "camera wait vtable before flush");
        calls.emplace_back("flush wait");
    }
    void CloseWait(pal4::CameraSequenceWaitState& candidate) override {
        Check(&candidate == &state, "camera close receiver");
        calls.emplace_back("close wait");
    }
    std::uint32_t AddWait(
        std::uint32_t queue,
        pal4::CameraSequenceWaitState* candidate) override {
        Check(queue == 0x8054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "camera wait queue");
        calls.emplace_back("add wait");
        return 0x4321u;
    }
    bool CameraCompleted(std::uint32_t camera) override {
        Check(camera == 0x5000u, "camera completion receiver");
        calls.emplace_back("completed?");
        return completed;
    }
    pal4::CameraSequenceVector ReadVector(
        std::uint32_t address) override {
        Check(address == 0x9074u || address == 0x9050u,
              "camera vector offsets");
        calls.emplace_back(address == 0x9074u ? "position vector" :
                                                  "forward vector");
        return address == 0x9074u ?
            pal4::CameraSequenceVector{1.0f, 2.0f, 3.0f} :
            pal4::CameraSequenceVector{4.0f, 5.0f, 6.0f};
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio manager");
        return 0xc000u;
    }
    std::uint32_t AudioHandle(std::uint32_t manager) override {
        Check(manager == 0xc000u, "audio handle manager");
        calls.emplace_back("audio handle");
        return 0xd000u;
    }
    void SetListenerPosition(
        std::uint32_t handle,
        pal4::CameraSequenceVector position) override {
        Check(handle == 0xd000u && position.x == 1.0f &&
                  position.y == 2.0f && position.z == -3.0f,
              "listener position negates camera Z");
        calls.emplace_back("listener position");
    }
    std::uint32_t SetListenerOrientation(
        std::uint32_t handle, pal4::CameraSequenceVector forward,
        pal4::CameraSequenceVector up) override {
        Check(handle == 0xd000u && forward.x == 4.0f &&
                  forward.y == 5.0f && forward.z == -6.0f &&
                  up.x == 0.0f && up.y == 1.0f && up.z == 0.0f,
              "listener direction and up vector");
        calls.emplace_back("listener orientation");
        return 0x1234u;
    }
};

void PrepareCases() {
    for (const auto result : {0u, 1u, 7u}) {
        Runtime runtime;
        runtime.load_result = result;
        pal4::ScriptCameraSequences sequences(runtime);
        Check(sequences.Prepare("opening") == (result != 0u) &&
                  runtime.calls == std::vector<std::string>{
                      "camera subsystem", "active camera",
                      "clear collection", "arena scene", "arena world",
                      "cloud manager", "load camera"},
              "prepare clears camera and forwards world, scene and result");
    }

    Runtime empty;
    empty.expected_camera = "";
    pal4::ScriptCameraSequences sequences(empty);
    Check(sequences.Prepare("") && empty.calls.size() == 7u,
          "null script string fallback reaches loader as empty name");
}

void RunSingleCases() {
    Runtime runtime;
    pal4::ScriptCameraSequences sequences(runtime);
    Check(sequences.RunSingle("opening", 7u) &&
              runtime.calls == std::vector<std::string>{
                  "cloud manager", "find cloud", "allocate wait",
                  "executor", "construct wait", "camera subsystem",
                  "active camera", "clear collection", "task container",
                  "create task", "add task", "camera ID",
                  "apply record", "processed", "flush wait",
                  "executor", "add wait"},
          "single camera creates task, applies record and queues wait");

    Runtime missing;
    missing.cloud_exists = false;
    pal4::ScriptCameraSequences missing_sequences(missing);
    Check(!missing_sequences.RunSingle("opening", 7u) &&
              missing.calls == std::vector<std::string>{
                  "cloud manager", "find cloud"},
          "missing camera record returns false before allocation");

    Runtime no_wait;
    no_wait.allocation_fails = true;
    pal4::ScriptCameraSequences no_wait_sequences(no_wait);
    Check(no_wait_sequences.RunSingle("opening", 7u) &&
              no_wait.calls == std::vector<std::string>{
                  "cloud manager", "find cloud", "allocate wait",
                  "executor", "add wait"},
          "failed wait allocation still queues null and returns true");

    Runtime no_task;
    no_task.task_exists = false;
    pal4::ScriptCameraSequences no_task_sequences(no_task);
    Check(no_task_sequences.RunSingle("opening", 7u) &&
              no_task.calls == std::vector<std::string>{
                  "cloud manager", "find cloud", "allocate wait",
                  "executor", "construct wait", "camera subsystem",
                  "active camera", "clear collection", "task container",
                  "create task", "close wait", "executor", "add wait"},
          "failed camera task closes wait before queueing it");
}

void AudioListenerCases() {
    for (const auto completed : {false, true}) {
        Runtime runtime;
        runtime.completed = completed;
        pal4::ScriptCameraSequences sequences(runtime);
        Check(sequences.UpdateAudioListener(runtime.state) == 0x1234u &&
                  runtime.calls == (completed ? std::vector<std::string>{
                      "camera subsystem", "active camera", "completed?",
                      "close wait", "camera ID", "position vector",
                      "audio manager", "audio handle", "listener position",
                      "camera ID", "forward vector", "audio manager",
                      "audio handle", "listener orientation"} :
                      std::vector<std::string>{
                      "camera subsystem", "active camera", "completed?",
                      "camera ID", "position vector", "audio manager",
                      "audio handle", "listener position", "camera ID",
                      "forward vector", "audio manager", "audio handle",
                      "listener orientation"}),
              "camera audio callback closes completed wait and updates listener");
    }

    Runtime circle;
    pal4::ScriptCameraSequences circle_sequences(circle);
    Check(circle_sequences.UpdateCircleAudioListener(circle.state) ==
              0x1234u && circle.calls.back() == "listener orientation",
          "circle listener callback shares the original audio behavior");
}

void RunCircleCases() {
    Runtime runtime;
    pal4::ScriptCameraSequences sequences(runtime);
    Check(sequences.RunCircle(7u) &&
              runtime.state.vtable_address == 0x8495c4u &&
              runtime.calls == std::vector<std::string>{
                  "cloud manager", "first cloud", "allocate wait",
                  "executor", "construct wait", "camera subsystem",
                  "active camera", "clear collection", "cloud manager",
                  "first cloud", "task container", "read control",
                  "create task", "add task", "camera ID", "apply record",
                  "cloud manager", "next cloud", "read control",
                  "create task", "add task", "camera ID", "apply record",
                  "cloud manager", "next cloud", "processed", "flush wait",
                  "executor", "add wait"},
          "circle runs each camera record with conditional control word");

    Runtime empty;
    empty.circle_exists = false;
    pal4::ScriptCameraSequences empty_sequences(empty);
    Check(!empty_sequences.RunCircle(7u) &&
              empty.calls == std::vector<std::string>{
                  "cloud manager", "first cloud"},
          "empty camera collection does not allocate handler");

    Runtime no_wait;
    no_wait.allocation_fails = true;
    pal4::ScriptCameraSequences no_wait_sequences(no_wait);
    Check(no_wait_sequences.RunCircle(7u) &&
              no_wait.calls == std::vector<std::string>{
                  "cloud manager", "first cloud", "allocate wait",
                  "executor", "add wait"},
          "circle queues null if wait allocation fails");

    Runtime changed;
    changed.second_current_exists = false;
    pal4::ScriptCameraSequences changed_sequences(changed);
    Check(changed_sequences.RunCircle(7u) &&
              changed.calls == std::vector<std::string>{
                  "cloud manager", "first cloud", "allocate wait",
                  "executor", "construct wait", "camera subsystem",
                  "active camera", "clear collection", "cloud manager",
                  "first cloud", "processed", "flush wait", "executor",
                  "add wait"},
          "circle still queues flushed handler if records vanish after clear");

    Runtime no_task;
    no_task.task_exists = false;
    pal4::ScriptCameraSequences no_task_sequences(no_task);
    Check(no_task_sequences.RunCircle(7u) &&
              no_task.calls == std::vector<std::string>{
                  "cloud manager", "first cloud", "allocate wait",
                  "executor", "construct wait", "camera subsystem",
                  "active camera", "clear collection", "cloud manager",
                  "first cloud", "task container", "read control",
                  "create task", "camera ID", "apply record",
                  "cloud manager", "next cloud", "read control",
                  "create task", "camera ID", "apply record",
                  "cloud manager", "next cloud", "processed", "flush wait",
                  "executor", "add wait"},
          "circle still applies each record after task factory returns null");
}

void WaitCases() {
    Runtime runtime;
    pal4::ScriptCameraSequences sequences(runtime);
    Check(sequences.CreateWaitHandler() == 0x4321u &&
              runtime.calls == std::vector<std::string>{
                  "allocate wait", "executor", "construct wait",
                  "flush wait", "executor", "add wait"},
          "camera wait constructs mode-one handler and forwards queue result");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptCameraSequences failed_sequences(failed);
    Check(failed_sequences.CreateWaitHandler() == 0x4321u &&
              failed.calls == std::vector<std::string>{
                  "allocate wait", "executor", "add wait"},
          "camera wait queues null on allocation failure");

    for (const auto completed : {false, true}) {
        Runtime callback;
        callback.completed = completed;
        pal4::ScriptCameraSequences callback_sequences(callback);
        callback_sequences.UpdateWaitHandler(callback.state);
        Check(callback.calls == (completed ? std::vector<std::string>{
                  "camera subsystem", "active camera", "completed?",
                  "close wait"} : std::vector<std::string>{
                  "camera subsystem", "active camera", "completed?"}),
              "camera wait closes only after active camera completion");
    }
}

}  // namespace

int main() {
    try {
        PrepareCases();
        RunSingleCases();
        AudioListenerCases();
        RunCircleCases();
        WaitCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
