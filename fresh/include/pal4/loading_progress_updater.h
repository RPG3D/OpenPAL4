#pragma once

#include <cstdint>

namespace pal4 {

class ILoadingProgressRuntime {
public:
    virtual ~ILoadingProgressRuntime() = default;
    [[nodiscard]] virtual bool HandleVideoModeChange() = 0;
    [[nodiscard]] virtual std::uint32_t GetUiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateLoadingWindow(
        std::uint32_t manager) = 0;
    virtual void UpdateLoadingWindow(std::uint32_t window, float progress) = 0;
    virtual void UpdateUi(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t GetGameController() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetStateType(std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t GetActiveCameraSlot(
        std::uint32_t subsystem) = 0;
    virtual void ClearCamera(std::uint32_t camera, std::uint32_t color,
                             std::uint32_t flags) = 0;
    virtual void BeginCameraUpdate(std::uint32_t camera) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void RenderState(std::uint32_t state,
                             std::uint32_t camera_slot) = 0;
    virtual void EndCameraUpdate(std::uint32_t camera) = 0;
    virtual void ShowGameRaster(std::uint32_t camera) = 0;
    virtual void HandleDeviceLoss() = 0;
};

class LoadingProgressRuntimeStub final : public ILoadingProgressRuntime {
public:
    [[nodiscard]] bool HandleVideoModeChange() override { return false; }
    [[nodiscard]] std::uint32_t GetUiFrameManager() override { return 0; }
    [[nodiscard]] std::uint32_t CreateLoadingWindow(std::uint32_t) override {
        return 0;
    }
    void UpdateLoadingWindow(std::uint32_t, float) override {}
    void UpdateUi(std::uint32_t, double) override {}
    [[nodiscard]] std::uint32_t GetGameController() override { return 0; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t GetStateType(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t InitializeCameraSubsystem() override {
        return 0;
    }
    [[nodiscard]] std::uint32_t GetActiveCameraSlot(std::uint32_t) override {
        return 0;
    }
    void ClearCamera(std::uint32_t, std::uint32_t, std::uint32_t) override {}
    void BeginCameraUpdate(std::uint32_t) override {}
    void SetRenderState(std::uint32_t, std::uint32_t) override {}
    void RenderState(std::uint32_t, std::uint32_t) override {}
    void EndCameraUpdate(std::uint32_t) override {}
    void ShowGameRaster(std::uint32_t) override {}
    void HandleDeviceLoss() override {}
};

class LoadingProgressUpdater final {
public:
    // IDA 0x51ab10.
    static void Update(float progress, ILoadingProgressRuntime& runtime);
};

}  // namespace pal4
