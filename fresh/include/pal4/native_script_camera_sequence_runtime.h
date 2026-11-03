#pragma once

#include "pal4/native_camera_file_catalog.h"
#include "pal4/native_camera_task_container.h"
#include "pal4/native_camera_task_playback.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_camera_sequences.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeCameraManagerOwner;
class NativeScriptEndActionQueue;
class NativeGameArenaManagerOwner;
class INativeCameraObjectResources;

// Binds scene camera callbacks to original .cam records and native task state.
class NativeScriptCameraSequenceRuntime final
    : public IScriptCameraSequenceRuntime,
      public INativeScriptFrameWaitRuntime {
public:
    NativeScriptCameraSequenceRuntime(
        std::filesystem::path game_root, NativeCameraManagerOwner& cameras,
        NativeGameArenaManagerOwner& arena,
        INativeCameraObjectResources& resources,
        std::function<bool()> suspend_script,
        NativeScriptCameraSequenceRuntime* shared_playback = nullptr);
    ~NativeScriptCameraSequenceRuntime();
    void BindWaitQueue(std::shared_ptr<NativeScriptEndActionQueue> queue);
    NativeScriptCameraSequenceRuntime(const NativeScriptCameraSequenceRuntime&) = delete;
    NativeScriptCameraSequenceRuntime& operator=(const NativeScriptCameraSequenceRuntime&) = delete;
    NativeScriptCameraSequenceRuntime(NativeScriptCameraSequenceRuntime&&) = delete;
    NativeScriptCameraSequenceRuntime& operator=(NativeScriptCameraSequenceRuntime&&) = delete;
    void ClearHandlers();
    // Scene teardown: retire record borrowers before clearing the cloud catalog.
    void ResetSceneSequences();
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return tasks_.OutstandingAllocations() +
               playback_.OutstandingAllocations();
    }
    // Production scheduling: poll waits before scripts, advance playback once
    // after scripts have created tasks, at the active-camera phase.
    void AdvancePlayback(float elapsed);
    [[nodiscard]] bool PollWaits();
    [[nodiscard]] INativeScriptFrameWaitRuntime& WaitHandlers() noexcept {
        return wait_handlers_;
    }
    // Combined entry retained for unbound isolated fixtures owning both phases.
    // Bound context queues use WaitHandlers after the action/wait dispatch.
    [[nodiscard]] bool Tick(float elapsed) override;
    [[nodiscard]] bool Waiting() const noexcept { return !active_waits_.empty(); }
    [[nodiscard]] bool BlocksScript() const override {
        if (wait_queue_) return SharedWaitBlocksScript();
        for (const auto& wait : active_waits_)
            if (wait.mode != 0u) return true;
        return false;
    }
    [[nodiscard]] std::uint32_t ActiveTask() const noexcept {
        return playback_.Address();
    }

    [[nodiscard]] std::size_t RecordCount() const noexcept {
        return records_.Count();
    }
    [[nodiscard]] std::uint32_t CameraSubsystem() override;
    [[nodiscard]] std::uint32_t ActiveCameraObject(
        std::uint32_t subsystem) override;
    void ClearCameraCollection(std::uint32_t camera) override;
    [[nodiscard]] std::string_view ArenaScene() override;
    [[nodiscard]] std::string_view ArenaWorld() override;
    [[nodiscard]] std::uint32_t CloudManager() override;
    [[nodiscard]] std::uint32_t LoadCameraFile(
        std::uint32_t manager, std::string_view world,
        std::string_view scene, std::string_view camera_name) override;
    [[nodiscard]] std::uint32_t FindCloud(
        std::uint32_t manager, std::string_view name) override;
    [[nodiscard]] std::uint32_t FirstCloud(std::uint32_t manager) override;
    [[nodiscard]] std::uint32_t NextCloud(std::uint32_t manager) override;
    [[nodiscard]] CameraSequenceControl ReadCloudControl(
        std::uint32_t record) override;
    [[nodiscard]] CameraSequenceWaitState* AllocateWait(
        std::uint32_t size) override;
    [[nodiscard]] std::uint32_t ScriptExecutor() override;
    void ConstructWait(CameraSequenceWaitState& state,
                       std::uint8_t wait_mode,
                       std::uint32_t queue) override;
    [[nodiscard]] std::uint32_t CreateCameraTask() override;
    [[nodiscard]] std::uint32_t TaskContainer(
        std::uint32_t camera) override;
    void AddCameraTask(std::uint32_t container, std::uint32_t task,
                       std::string_view name,
                       const std::array<std::uint32_t, 3>& options) override;
    [[nodiscard]] std::uint32_t CameraInternalId(
        std::uint32_t camera) override;
    void ApplyCameraRecord(std::uint32_t task, std::uint32_t camera_id,
                           std::uint32_t record) override;
    void MarkProcessed(std::uint32_t camera) override;
    void FlushWait(CameraSequenceWaitState& state) override;
    void CloseWait(CameraSequenceWaitState& state) override;
    [[nodiscard]] std::uint32_t AddWait(
        std::uint32_t queue, CameraSequenceWaitState* state) override;
    [[nodiscard]] bool CameraCompleted(std::uint32_t camera) override;
    [[nodiscard]] CameraSequenceVector ReadVector(
        std::uint32_t address) override;
    [[nodiscard]] std::uint32_t AudioManager() override;
    [[nodiscard]] std::uint32_t AudioHandle(
        std::uint32_t manager) override;
    void SetListenerPosition(std::uint32_t handle,
                             CameraSequenceVector position) override;
    [[nodiscard]] std::uint32_t SetListenerOrientation(
        std::uint32_t handle, CameraSequenceVector forward,
        CameraSequenceVector up) override;

private:
    class WaitHandlerAdapter final : public INativeScriptFrameWaitRuntime {
    public:
        explicit WaitHandlerAdapter(NativeScriptCameraSequenceRuntime& owner)
            : owner_(owner) {}
        [[nodiscard]] bool Tick(float elapsed) override;
        [[nodiscard]] bool BlocksScript() const override {
            return owner_.BlocksScript();
        }
    private:
        NativeScriptCameraSequenceRuntime& owner_;
    };
    struct ActiveWait {
        std::unique_ptr<CameraSequenceWaitState> state;
        std::uint8_t mode{};
        bool closed{};
    };

    [[nodiscard]] bool SharedWaitBlocksScript() const;
    std::shared_ptr<NativeScriptEndActionQueue> wait_queue_;
    void RequireCloudManager(std::uint32_t manager) const;
    [[nodiscard]] const NativeCameraFileRecord& RequireRecord(
        std::uint32_t address) const;

    std::filesystem::path game_root_;
    NativeCameraManagerOwner& cameras_;
    NativeGameArenaManagerOwner& arena_;
    struct PlaybackState {
        PlaybackState(NativeCameraManagerOwner& cameras, INativeCameraObjectResources& resources)
            : playback(cameras, resources, records), tasks(playback) {}
        NativeCameraFileCatalog records;
        NativeCameraTaskPlayback playback;
        NativeCameraTaskContainer tasks;
        std::size_t cursor{};
    };
    // One embedded camera list has one allocator/playback owner across contexts.
    std::shared_ptr<PlaybackState> playback_state_;
    NativeCameraFileCatalog& records_;
    NativeCameraTaskPlayback& playback_;
    NativeCameraTaskContainer& tasks_;
    std::function<bool()> suspend_script_;
    std::unique_ptr<CameraSequenceWaitState> pending_wait_;
    std::vector<ActiveWait> active_waits_;
    std::uint8_t wait_mode_{};
    bool pending_closed_{};
    std::size_t& cursor_;
    WaitHandlerAdapter wait_handlers_{*this};
};

}  // namespace pal4
