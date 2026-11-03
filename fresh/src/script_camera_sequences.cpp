#include "pal4/script_camera_sequences.h"

namespace pal4 {

bool ScriptCameraSequences::Prepare(std::string_view camera_name) {
    const auto subsystem = runtime_.CameraSubsystem();
    const auto active = runtime_.ActiveCameraObject(subsystem);
    runtime_.ClearCameraCollection(active);
    const auto scene = runtime_.ArenaScene();
    const auto world = runtime_.ArenaWorld();
    const auto cloud = runtime_.CloudManager();
    return runtime_.LoadCameraFile(cloud, world, scene, camera_name) != 0u;
}

bool ScriptCameraSequences::RunSingle(
    std::string_view camera_name, std::uint8_t wait_mode) {
    const auto cloud = runtime_.CloudManager();
    const auto record = runtime_.FindCloud(cloud, camera_name);
    if (record == 0u) return false;

    auto* state = runtime_.AllocateWait(sizeof(CameraSequenceWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kSingleVtable;
        const auto subsystem = runtime_.CameraSubsystem();
        const auto camera = runtime_.ActiveCameraObject(subsystem);
        runtime_.ClearCameraCollection(camera);
        const auto container = runtime_.TaskContainer(camera);
        const auto task = runtime_.CreateCameraTask();
        if (task != 0u) {
            runtime_.AddCameraTask(container, task, "synchroCameraRun-CLD",
                                   {3u, 0u, 0u});
            runtime_.ApplyCameraRecord(
                task, runtime_.CameraInternalId(camera), record);
            runtime_.MarkProcessed(camera);
            runtime_.FlushWait(*state);
        } else {
            runtime_.CloseWait(*state);
        }
    }
    static_cast<void>(runtime_.AddWait(
        runtime_.ScriptExecutor() + kQueueOffset, state));
    return true;
}

bool ScriptCameraSequences::RunCircle(std::uint8_t wait_mode) {
    const auto cloud = runtime_.CloudManager();
    if (runtime_.FirstCloud(cloud) == 0u) return false;

    auto* state = runtime_.AllocateWait(sizeof(CameraSequenceWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kCircleVtable;
        const auto subsystem = runtime_.CameraSubsystem();
        const auto camera = runtime_.ActiveCameraObject(subsystem);
        runtime_.ClearCameraCollection(camera);
        auto manager = runtime_.CloudManager();
        auto record = runtime_.FirstCloud(manager);
        if (record != 0u) {
            const auto container = runtime_.TaskContainer(camera);
            do {
                const auto control = runtime_.ReadCloudControl(record);
                const auto options = std::array<std::uint32_t, 3>{
                    1u, 0u, control.field14 != 0u ? 0u : control.field13};
                const auto task = runtime_.CreateCameraTask();
                if (task != 0u)
                    runtime_.AddCameraTask(container, task,
                                           "synchroCameraRun-CLD", options);
                runtime_.ApplyCameraRecord(
                    task, runtime_.CameraInternalId(camera), record);
                manager = runtime_.CloudManager();
                record = runtime_.NextCloud(manager);
            } while (record != 0u);
        }
        runtime_.MarkProcessed(camera);
        runtime_.FlushWait(*state);
    }
    static_cast<void>(runtime_.AddWait(
        runtime_.ScriptExecutor() + kQueueOffset, state));
    return true;
}

std::uint32_t ScriptCameraSequences::UpdateAudioListener(
    CameraSequenceWaitState& state) {
    const auto subsystem = runtime_.CameraSubsystem();
    const auto camera = runtime_.ActiveCameraObject(subsystem);
    if (runtime_.CameraCompleted(camera)) runtime_.CloseWait(state);

    const auto position = runtime_.ReadVector(
        runtime_.CameraInternalId(camera) + 0x74u);
    const auto position_manager = runtime_.AudioManager();
    runtime_.SetListenerPosition(runtime_.AudioHandle(position_manager),
                                 {position.x, position.y, -position.z});

    const auto forward = runtime_.ReadVector(
        runtime_.CameraInternalId(camera) + 0x50u);
    const auto direction_manager = runtime_.AudioManager();
    return runtime_.SetListenerOrientation(
        runtime_.AudioHandle(direction_manager),
        {forward.x, forward.y, -forward.z}, {0.0f, 1.0f, -0.0f});
}

std::uint32_t ScriptCameraSequences::UpdateCircleAudioListener(
    CameraSequenceWaitState& state) {
    return UpdateAudioListener(state);
}

std::uint32_t ScriptCameraSequences::CreateWaitHandler() {
    auto* state = runtime_.AllocateWait(sizeof(CameraSequenceWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        runtime_.FlushWait(*state);
    }
    return runtime_.AddWait(runtime_.ScriptExecutor() + kQueueOffset,
                            state);
}

void ScriptCameraSequences::UpdateWaitHandler(
    CameraSequenceWaitState& state) {
    const auto subsystem = runtime_.CameraSubsystem();
    const auto camera = runtime_.ActiveCameraObject(subsystem);
    if (runtime_.CameraCompleted(camera)) runtime_.CloseWait(state);
}

}  // namespace pal4
