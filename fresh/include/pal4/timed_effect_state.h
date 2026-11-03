#pragma once

#include <cstdint>

#include "pal4/pal_rectangle_drawer.h"

namespace pal4 {

struct TimedEffectState {
    std::uint32_t vtable{};
    std::uint8_t loaded{};
    std::uint8_t enabled{};
    std::uint8_t reserved[2]{};
    float interval{};
    float delta{};
    float elapsed{};
    std::uint32_t completed_cycles{};
    std::uint32_t camera_count{};
    std::uint32_t active_camera{};
    std::uint32_t reserved_tail{};
    std::uint32_t cameras_begin{};
    std::uint32_t cameras_end{};
    std::uint32_t cameras_capacity_end{};
};
static_assert(sizeof(TimedEffectState) == 48u);

class ITimedEffectMemory {
public:
    virtual ~ITimedEffectMemory() = default;
    [[nodiscard]] virtual std::uint32_t Read32(std::uint32_t address) = 0;
    virtual void Write32(std::uint32_t address, std::uint32_t value) = 0;
};

class TimedEffectMemoryStub final : public ITimedEffectMemory {
public:
    [[nodiscard]] std::uint32_t Read32(std::uint32_t address) override;
    void Write32(std::uint32_t address, std::uint32_t value) override;
};

class TimedEffectControl final {
public:
    [[nodiscard]] static std::uint32_t Start(TimedEffectState& state);
    [[nodiscard]] static std::uint8_t Stop(
        TimedEffectState& state, ITimedEffectMemory& memory);
    [[nodiscard]] static std::uint8_t Advance(
        TimedEffectState& state, std::uint32_t camera,
        ITimedEffectMemory& memory);
};

class ITimedEffectManagerRuntime : public ITimedEffectMemory {
public:
    [[nodiscard]] virtual std::uint32_t CreateCameraRecord() = 0;
    [[nodiscard]] virtual bool InitializeCamera(std::uint32_t record) = 0;
    virtual void CleanupCamera(std::uint32_t record) = 0;
    virtual void DestroyCameraRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateArray(std::uint32_t bytes) = 0;
    virtual void FreeArray(std::uint32_t address) = 0;
};

class TimedEffectManagerRuntimeStub final : public ITimedEffectManagerRuntime {
public:
    [[nodiscard]] std::uint32_t Read32(std::uint32_t address) override;
    void Write32(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t CreateCameraRecord() override;
    [[nodiscard]] bool InitializeCamera(std::uint32_t record) override;
    void CleanupCamera(std::uint32_t record) override;
    void DestroyCameraRecord(std::uint32_t record) override;
    [[nodiscard]] std::uint32_t AllocateArray(std::uint32_t bytes) override;
    void FreeArray(std::uint32_t address) override;
};

class TimedEffectManager final {
public:
    [[nodiscard]] static bool Initialize(
        TimedEffectState& state, ITimedEffectManagerRuntime& runtime);
    static void Cleanup(TimedEffectState& state,
                        ITimedEffectManagerRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetDelta(
        TimedEffectState& state, double delta);
};

struct TimedEffectRenderState {
    std::uint8_t dimensions_initialized{};
    std::int32_t x{};
    std::int32_t y{};
    std::int32_t width{};
    std::int32_t height{};
};

class ITimedEffectRenderRuntime : public ITimedEffectMemory,
                                  public IPalRectangleRenderRuntime {
public:
    [[nodiscard]] virtual std::int32_t ConfigWidth() = 0;
    [[nodiscard]] virtual std::int32_t ConfigHeight() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectInternalId(
        std::uint32_t camera) = 0;
};

class TimedEffectRenderRuntimeStub final : public ITimedEffectRenderRuntime {
public:
    [[nodiscard]] std::uint32_t Read32(std::uint32_t address) override;
    void Write32(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::int32_t ConfigWidth() override;
    [[nodiscard]] std::int32_t ConfigHeight() override;
    [[nodiscard]] std::uint32_t InitializeCameraSubsystem() override;
    [[nodiscard]] std::uint32_t ActiveCameraObject(
        std::uint32_t manager) override;
    [[nodiscard]] std::uint32_t ObjectInternalId(
        std::uint32_t camera) override;
    [[nodiscard]] float CameraDepth(std::uint32_t context) override;
    [[nodiscard]] float ActiveDepth() override;
    [[nodiscard]] std::uint32_t TextureRaster(
        std::uint32_t texture) override;
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override;
    void DrawPrimitive(
        std::uint32_t primitive,
        const std::array<CeguiRenderVertex, 4>& vertices,
        std::uint32_t count) override;
};

class TimedEffectRenderer final {
public:
    [[nodiscard]] static bool Render(
        const TimedEffectState& state, TimedEffectRenderState& render_state,
        std::uint32_t ignored_first, std::uint32_t ignored_second,
        ITimedEffectRenderRuntime& runtime);
};

}  // namespace pal4
