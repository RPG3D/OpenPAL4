#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct LogoEffectHandle {
    std::uint32_t first;
    std::uint32_t second;
};

class ILogoStateLifecycleRuntime {
public:
    virtual ~ILogoStateLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void SaveDialogSettings(std::uint32_t global) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t MusicPlayer() = 0;
    virtual void FadeInMusic(std::uint32_t player, std::string_view track,
                             double seconds, std::uint32_t flags) = 0;
    virtual void AddUIElement(std::uint32_t manager,
                              std::string_view name) = 0;
    virtual void SetupUICamera(std::uint32_t camera,
                               std::uint32_t enabled) = 0;
    virtual void SendUIEvent(std::uint32_t manager, std::uint32_t event,
                             std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t UICameraName() = 0;
    virtual void ActivateCamera(std::uint32_t manager,
                                std::uint32_t name) = 0;
    virtual void ToggleWindows(std::uint32_t manager, bool enabled) = 0;
    [[nodiscard]] virtual bool EffectNeedsUpdate(std::uint32_t handle) = 0;
    virtual void HandleEffect(std::uint32_t handle,
                              std::uint32_t mode) = 0;
    [[nodiscard]] virtual LogoEffectHandle CreateEffect(
        std::uint32_t manager, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t selector, std::uint32_t flags) = 0;
    virtual void ResetTemporaryEffectHandle() = 0;
    virtual void SetEffectProperties(
        std::uint32_t handle, const std::array<float, 3>& scale,
        std::uint8_t enabled) = 0;
    virtual void CleanupEffects(std::uint32_t manager) = 0;
};

// Update capability is independent of entry/leave and rendering. Script waits
// may resume the active module; this is not a logger endpoint.
class ILogoStateUpdateRuntime {
public:
    virtual ~ILogoStateUpdateRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() = 0;
    virtual void AdvanceScreenEffect(std::uint32_t effect, float delta) = 0;
    virtual void ProcessScriptWaitHandlers(double delta) = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateEffects(
        std::uint32_t manager, double delta) = 0;
};

class ILogoStateRenderRuntime {
public:
    virtual ~ILogoStateRenderRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() = 0;
    [[nodiscard]] virtual std::uint32_t BinkPlayer() = 0;
    [[nodiscard]] virtual bool IsBinkPlaying(std::uint32_t player_state) = 0;
    virtual void PreRenderUI(std::uint32_t manager,
                             std::uint32_t frame) = 0;
    virtual void DrawScreenEffect(std::uint32_t effect,
                                  std::uint32_t frame) = 0;
    virtual void RenderBink(std::uint32_t player,
                            std::uint32_t frame) = 0;
    virtual void RenderUI(std::uint32_t manager,
                          std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    virtual void RenderConsole(std::uint32_t console,
                               std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GameHeight() = 0;
    virtual void DrawVersionText(std::string_view text,
                                 std::int32_t x, std::int32_t y,
                                 std::int32_t width, std::int32_t height,
                                 std::uint32_t mode, std::uint32_t color,
                                 std::uint32_t flags) = 0;

};

// Compatibility composition for shared tests/owners needing all capabilities.
class ILogoStateRuntime : public ILogoStateLifecycleRuntime,
                          public ILogoStateUpdateRuntime,
                          public ILogoStateRenderRuntime {
public:
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() override = 0;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() override = 0;
};

class LogoState final {
public:
    static constexpr std::uint32_t kOpenUIEvent = 0x8a7880u;
    static constexpr std::uint32_t kCloseUIEvent = 0x8a7884u;

    // IDA 0x58f930/0x58fb20/0x58fc00/0x58fc80/0x58fca0.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t object, ILogoStateLifecycleRuntime& runtime);
    static void Render(std::uint32_t object, std::uint32_t frame,
                       ILogoStateRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float delta, std::uint32_t unused,
        std::uint32_t flag, ILogoStateUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveWrapper(
        std::uint32_t object, std::uint32_t unused,
        ILogoStateLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t object, ILogoStateLifecycleRuntime& runtime);
};

}  // namespace pal4
