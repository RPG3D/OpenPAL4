#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ScreenEffectCameraRecord {
    std::uint8_t active{};
    std::uint8_t reserved[3]{};
    std::uint32_t texture{};
    std::uint32_t camera{};
    std::int32_t width{};
    std::int32_t height{};
    std::uint32_t clear_color{};
};
static_assert(sizeof(ScreenEffectCameraRecord) == 24u);
static_assert(offsetof(ScreenEffectCameraRecord, texture) == 4u);
static_assert(offsetof(ScreenEffectCameraRecord, camera) == 8u);
static_assert(offsetof(ScreenEffectCameraRecord, clear_color) == 20u);

class IScreenEffectCameraRuntime {
public:
    virtual ~IScreenEffectCameraRuntime() = default;
    [[nodiscard]] virtual std::int32_t ScreenWidth() = 0;
    [[nodiscard]] virtual std::int32_t ScreenHeight() = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::int32_t width, std::int32_t height, std::uint32_t depth,
        std::uint32_t flags) = 0;
    virtual void DestroyRaster(std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCamera() = 0;
    virtual void SetCameraRaster(std::uint32_t camera,
                                 std::uint32_t offset,
                                 std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t CameraRaster(
        std::uint32_t camera, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void AttachFrame(std::uint32_t camera, std::uint32_t frame) = 0;
    [[nodiscard]] virtual float GameNearClip() = 0;
    [[nodiscard]] virtual float GameFarClip() = 0;
    virtual void SetNearClip(std::uint32_t camera, float value) = 0;
    virtual void SetFarClip(std::uint32_t camera, float value) = 0;
    [[nodiscard]] virtual std::array<float, 2> ActiveViewWindow() = 0;
    virtual void SetViewWindow(
        std::uint32_t camera, const std::array<float, 2>& window) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t raster) = 0;
    virtual void ClearCamera(std::uint32_t camera,
                             const std::uint32_t& color,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t EndUpdate(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t ActiveGameCamera() = 0;
    [[nodiscard]] virtual std::uint32_t WorldForCamera(
        std::uint32_t camera) = 0;
    virtual void RemoveCamera(std::uint32_t world,
                              std::uint32_t camera) = 0;
    virtual void AddCamera(std::uint32_t world,
                           std::uint32_t camera) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void DestroyCamera(std::uint32_t camera) = 0;
    [[nodiscard]] virtual ScreenEffectCameraRecord* AllocateRecord(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FreeRecord(
        ScreenEffectCameraRecord* record) = 0;
};

class ScreenEffectCameraRuntimeStub final : public IScreenEffectCameraRuntime {
public:
    [[nodiscard]] std::int32_t ScreenWidth() override;
    [[nodiscard]] std::int32_t ScreenHeight() override;
    [[nodiscard]] std::uint32_t CreateRaster(
        std::int32_t width, std::int32_t height, std::uint32_t depth,
        std::uint32_t flags) override;
    void DestroyRaster(std::uint32_t raster) override;
    [[nodiscard]] std::uint32_t CreateCamera() override;
    void SetCameraRaster(std::uint32_t camera,
                         std::uint32_t offset,
                         std::uint32_t raster) override;
    [[nodiscard]] std::uint32_t CameraRaster(
        std::uint32_t camera, std::uint32_t offset) override;
    [[nodiscard]] std::uint32_t CreateFrame() override;
    void AttachFrame(std::uint32_t camera, std::uint32_t frame) override;
    [[nodiscard]] float GameNearClip() override;
    [[nodiscard]] float GameFarClip() override;
    void SetNearClip(std::uint32_t camera, float value) override;
    void SetFarClip(std::uint32_t camera, float value) override;
    [[nodiscard]] std::array<float, 2> ActiveViewWindow() override;
    void SetViewWindow(
        std::uint32_t camera, const std::array<float, 2>& window) override;
    [[nodiscard]] std::uint32_t CreateTexture(std::uint32_t raster) override;
    void ClearCamera(std::uint32_t camera, const std::uint32_t& color,
                     std::uint32_t flags) override;
    [[nodiscard]] std::uint32_t EndUpdate(std::uint32_t camera) override;
    [[nodiscard]] std::uint32_t ActiveGameCamera() override;
    [[nodiscard]] std::uint32_t WorldForCamera(
        std::uint32_t camera) override;
    void RemoveCamera(std::uint32_t world,
                      std::uint32_t camera) override;
    void AddCamera(std::uint32_t world,
                   std::uint32_t camera) override;
    void DestroyTexture(std::uint32_t texture) override;
    void DestroyCamera(std::uint32_t camera) override;
    [[nodiscard]] ScreenEffectCameraRecord* AllocateRecord(
        std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t FreeRecord(
        ScreenEffectCameraRecord* record) override;
};

class ScreenEffectCamera final {
public:
    [[nodiscard]] static std::uint32_t EndUpdate(
        const ScreenEffectCameraRecord& state, std::uint32_t incoming_eax,
        IScreenEffectCameraRuntime& runtime);
    [[nodiscard]] static std::uint8_t Initialize(
        ScreenEffectCameraRecord& state,
        IScreenEffectCameraRuntime& runtime);
    static void Cleanup(ScreenEffectCameraRecord& state,
                        IScreenEffectCameraRuntime& runtime);
    static void SyncWorld(const ScreenEffectCameraRecord& state,
                          IScreenEffectCameraRuntime& runtime);
    [[nodiscard]] static ScreenEffectCameraRecord* Allocate(
        IScreenEffectCameraRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        ScreenEffectCameraRecord* state, std::uint32_t incoming_eax,
        IScreenEffectCameraRuntime& runtime);
};

}  // namespace pal4
