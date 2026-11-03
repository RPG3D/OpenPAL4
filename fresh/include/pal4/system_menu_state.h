#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemMenuRuntime {
public:
    virtual ~ISystemMenuRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    virtual void AddUIElement(std::uint32_t manager,
                              std::string_view name) = 0;
    virtual void SetupUICamera(std::uint32_t camera,
                               std::uint32_t enabled) = 0;
    virtual void CopyActiveCameraRaster(std::uint32_t manager) = 0;
    virtual void GetActiveCamera(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t UICameraName() = 0;
    virtual void ActivateCamera(std::uint32_t manager,
                                std::uint32_t name) = 0;
    virtual void DispatchEvent(std::uint32_t manager,
                               std::uint32_t descriptor,
                               std::uint32_t first,
                               std::uint32_t second) = 0;
    virtual void ToggleWindows(std::uint32_t manager, bool enabled) = 0;
    virtual void PreRenderUI(std::uint32_t manager,
                             std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t RenderUI(
        std::uint32_t manager, std::uint32_t frame) = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t RenderEffectTransform(
        std::uint32_t manager, double delta) = 0;
    virtual void CleanupEffects(std::uint32_t manager) = 0;
};

class SystemMenuState final {
public:
    // IDA Game_InitializeComponents writes this after allocating four bytes.
    static constexpr std::uint32_t kVtable = 0x849438u;
    static constexpr std::uint32_t kFirstCloseEvent = 0x8a80a4u;
    static constexpr std::uint32_t kSecondCloseEvent = 0x8a80b0u;

    // IDA 0x5902a0..0x590410, virtual slots 0x84943c..0x849448.
    [[nodiscard]] static std::uint32_t EnterWrapper(
        std::uint32_t unused1, std::uint32_t unused2,
        ISystemMenuRuntime& runtime);
    [[nodiscard]] static std::uint32_t Enter(
        ISystemMenuRuntime& runtime);
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t frame, ISystemMenuRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        float delta, std::uint32_t unused1,
        std::uint32_t unused2, ISystemMenuRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveWrapper(
        std::uint32_t unused, ISystemMenuRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        ISystemMenuRuntime& runtime);
};

}  // namespace pal4
