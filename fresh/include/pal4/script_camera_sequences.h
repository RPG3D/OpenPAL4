#pragma once

#include <cstdint>
#include <array>
#include <cstddef>
#include <string_view>

namespace pal4 {

struct CameraSequenceWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
};
static_assert(sizeof(CameraSequenceWaitState) == 0x20u);

struct CameraSequenceVector {
    float x{};
    float y{};
    float z{};
};

struct CameraSequenceControl {
    std::uint32_t field13{};
    std::uint32_t field14{};
};

class IScriptCameraSequenceRuntime {
public:
    virtual ~IScriptCameraSequenceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t subsystem) = 0;
    virtual void ClearCameraCollection(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::string_view ArenaScene() = 0;
    [[nodiscard]] virtual std::string_view ArenaWorld() = 0;
    [[nodiscard]] virtual std::uint32_t CloudManager() = 0;
    [[nodiscard]] virtual std::uint32_t LoadCameraFile(
        std::uint32_t manager, std::string_view world,
        std::string_view scene, std::string_view camera_name) = 0;
    [[nodiscard]] virtual std::uint32_t FindCloud(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FirstCloud(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NextCloud(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual CameraSequenceControl ReadCloudControl(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual CameraSequenceWaitState* AllocateWait(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(CameraSequenceWaitState& state,
                               std::uint8_t wait_mode,
                               std::uint32_t queue) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCameraTask() = 0;
    [[nodiscard]] virtual std::uint32_t TaskContainer(
        std::uint32_t camera) = 0;
    virtual void AddCameraTask(std::uint32_t container,
                               std::uint32_t task,
                               std::string_view name,
                               const std::array<std::uint32_t, 3>& options) = 0;
    [[nodiscard]] virtual std::uint32_t CameraInternalId(
        std::uint32_t camera) = 0;
    virtual void ApplyCameraRecord(std::uint32_t task,
                                   std::uint32_t camera_id,
                                   std::uint32_t record) = 0;
    virtual void MarkProcessed(std::uint32_t camera) = 0;
    virtual void FlushWait(CameraSequenceWaitState& state) = 0;
    virtual void CloseWait(CameraSequenceWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddWait(
        std::uint32_t queue, CameraSequenceWaitState* state) = 0;
    [[nodiscard]] virtual bool CameraCompleted(std::uint32_t camera) = 0;
    [[nodiscard]] virtual CameraSequenceVector ReadVector(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t AudioHandle(
        std::uint32_t manager) = 0;
    virtual void SetListenerPosition(std::uint32_t handle,
                                     CameraSequenceVector position) = 0;
    [[nodiscard]] virtual std::uint32_t SetListenerOrientation(
        std::uint32_t handle, CameraSequenceVector forward,
        CameraSequenceVector up) = 0;
};

class ScriptCameraSequences final {
public:
    static constexpr std::uint32_t kSingleVtable = 0x8495b8u;
    static constexpr std::uint32_t kCircleVtable = 0x8495c4u;
    static constexpr std::uint32_t kWaitVtable = 0x8495d0u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    explicit ScriptCameraSequences(IScriptCameraSequenceRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5e4660..0x5e4bb0.
    [[nodiscard]] bool Prepare(std::string_view camera_name);
    [[nodiscard]] bool RunSingle(std::string_view camera_name,
                                 std::uint8_t wait_mode);
    [[nodiscard]] bool RunCircle(std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t UpdateAudioListener(
        CameraSequenceWaitState& state);
    [[nodiscard]] std::uint32_t UpdateCircleAudioListener(
        CameraSequenceWaitState& state);
    [[nodiscard]] std::uint32_t CreateWaitHandler();
    void UpdateWaitHandler(CameraSequenceWaitState& state);

private:
    IScriptCameraSequenceRuntime& runtime_;
};

}  // namespace pal4
