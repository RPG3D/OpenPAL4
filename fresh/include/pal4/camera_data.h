#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace pal4 {

class CameraEnvironmentProbe;
class CameraObstacleCheck;

// The 0x2ec-byte camera object initialized at 0x5e8fc0.
struct CameraDataState {
    std::array<std::uint32_t, 0x2ecu / 4u> words{};

    [[nodiscard]] std::uint32_t& At(std::size_t offset) {
        return words[offset / 4u];
    }
    [[nodiscard]] const std::uint32_t& At(std::size_t offset) const {
        return words[offset / 4u];
    }
};
static_assert(sizeof(CameraDataState) == 0x2ecu);

class ICameraDataRuntime {
public:
    virtual ~ICameraDataRuntime() = default;
    virtual void InitializeNested(CameraDataState& state,
                                  std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t NullFunctionSentinel() = 0;
    virtual void DestroyCamera(CameraDataState& state) = 0;
    virtual void DestroyNested(CameraDataState& state,
                               std::uint32_t offset) = 0;
};

class CameraData final {
public:
    explicit CameraData(ICameraDataRuntime& runtime) : runtime_(runtime) {}

    // 0x5e8fc0: initialize only the fields explicitly written by the binary.
    CameraDataState& Construct(CameraDataState& state);
    // 0x5e90a0: release camera, then the nested object at +0xc.
    void Destroy(CameraDataState& state);

private:
    ICameraDataRuntime& runtime_;
};

class ICameraDeviceRuntime {
public:
    virtual ~ICameraDeviceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateCamera() = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void AttachFrame(std::uint32_t camera,
                             std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t flags) = 0;
    virtual void StoreRaster(std::uint32_t camera,
                             std::uint32_t offset,
                             std::uint32_t raster) = 0;
    virtual void SetNearClip(std::uint32_t camera, float distance) = 0;
    virtual void SetFarClip(std::uint32_t camera, float distance) = 0;
    [[nodiscard]] virtual std::uint32_t CameraFrame(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t CameraRaster(
        std::uint32_t camera, std::uint32_t offset) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<float, 3>& translation,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CameraWorld(
        std::uint32_t camera) = 0;
    virtual void RemoveCameraFromWorld(std::uint32_t world,
                                       std::uint32_t camera) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void DestroyRaster(std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyCamera(
        std::uint32_t camera) = 0;
};

class CameraDevice final {
public:
    explicit CameraDevice(ICameraDeviceRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e90f0: rebuild native camera and optional depth raster.
    [[nodiscard]] bool Initialize(CameraDataState& state,
                                  std::uint32_t width,
                                  std::uint32_t height,
                                  bool depth_raster,
                                  float near_clip,
                                  float far_clip);
    // 0x5e91f0: detach and release camera resources in native order.
    [[nodiscard]] std::uint32_t Destroy(CameraDataState& state);

private:
    ICameraDeviceRuntime& runtime_;
};

using CameraRect = std::array<std::int32_t, 4>;

struct CameraViewportGlobals {
    CameraRect default_rect{};  // 0x90f104..0x90f110
    std::int32_t original_x{};  // 0x90f114
    std::int32_t original_y{};  // 0x90f118
    std::int32_t original_width{};  // 0x90f11c
    std::int32_t original_height{};  // 0x90f120
};

struct CameraVideoMode {
    std::int32_t width{};
    std::int32_t height{};
    std::uint32_t flags{};
};

class ICameraGeometryRuntime {
public:
    virtual ~ICameraGeometryRuntime() = default;
    [[nodiscard]] virtual std::pair<std::int32_t, std::int32_t>
    RasterSize(std::uint32_t camera, std::uint32_t offset) = 0;
    virtual void SetRasterSize(std::uint32_t camera,
                               std::uint32_t offset,
                               std::int32_t width,
                               std::int32_t height) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentVideoMode() = 0;
    [[nodiscard]] virtual CameraVideoMode VideoModeInfo(
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool ExpandedAspect() = 0;
    virtual void SetViewWindow(std::uint32_t camera,
                               float x, float y) = 0;
    [[nodiscard]] virtual float MinimumDistance() = 0;
    [[nodiscard]] virtual float MaximumDistance() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMatrix(
        CameraDataState& state, std::uint32_t mode) = 0;
};

class CameraGeometry final {
public:
    explicit CameraGeometry(ICameraGeometryRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e9270: apply viewport and set aspect-aware view window.
    void AdjustViewWindow(CameraDataState& state,
                          CameraViewportGlobals& globals,
                          CameraRect* rectangle,
                          float scale,
                          std::int32_t unused);
    // 0x5e93d0: update target, angles and clamped camera distance.
    [[nodiscard]] std::optional<std::uint32_t> SetParameters(
        CameraDataState& state,
        const std::array<float, 3>& target,
        float angle_y, float angle_x, float angle_z,
        float distance);

private:
    ICameraGeometryRuntime& runtime_;
};

class ICameraAnimationRuntime {
public:
    virtual ~ICameraAnimationRuntime() = default;
    [[nodiscard]] virtual std::int32_t Random() = 0;
    [[nodiscard]] virtual float MinimumDistance() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMatrix(
        CameraDataState& state, std::uint32_t mode) = 0;
};

class CameraAnimation final {
public:
    CameraAnimation(ICameraAnimationRuntime& runtime,
                    CameraEnvironmentProbe& environment,
                    CameraObstacleCheck& obstacle)
        : runtime_(runtime), environment_(environment), obstacle_(obstacle) {}

    // 0x5e9460: advance distance, constrain it and update camera matrix.
    [[nodiscard]] std::optional<std::uint32_t> Update(
        CameraDataState& state, float delta);
    // 0x5ea350: animate the target Y using a fading sine/random offset.
    [[nodiscard]] std::optional<std::uint32_t> AnimatePosition(
        CameraDataState& state, float delta);

private:
    ICameraAnimationRuntime& runtime_;
    CameraEnvironmentProbe& environment_;
    CameraObstacleCheck& obstacle_;
};

}  // namespace pal4
