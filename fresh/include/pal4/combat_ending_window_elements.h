#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct CombatUiPosition {
    float x = 0.0f;
    float y = 0.0f;
};

class ICombatEndingWindowElementsRuntime {
public:
    virtual ~ICombatEndingWindowElementsRuntime() = default;
    [[nodiscard]] virtual std::string LayoutPrefix(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t dialog, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    virtual void SetFormatting(std::uint32_t window,
                               std::uint32_t horizontal,
                               std::uint32_t vertical) = 0;
    virtual void SetHorizontalFormatting(std::uint32_t window,
                                         std::uint32_t mode) = 0;
    virtual void SetClippedByParent(std::uint32_t window,
                                    bool clipped) = 0;
    [[nodiscard]] virtual float GetYPosition(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    [[nodiscard]] virtual CombatUiPosition GetPosition(
        std::uint32_t window, std::uint32_t metrics_mode) = 0;
    [[nodiscard]] virtual CombatUiPosition WindowToScreen(
        std::uint32_t parent, CombatUiPosition position) = 0;
    [[nodiscard]] virtual std::uint32_t GuiRootWindow() = 0;
    [[nodiscard]] virtual CombatUiPosition AbsoluteToRelative(
        std::uint32_t root, CombatUiPosition position) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeMouseDown(
        std::uint32_t root, std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
};

class CombatEndingWindowElements final {
public:
    static constexpr std::uint32_t kMouseDownCallback = 0x544e40u;
    static constexpr std::uint32_t kCallbackVtable = 0x845388u;

    // IDA 0x543f80.
    static void Setup(std::uint32_t dialog,
                      ICombatEndingWindowElementsRuntime& runtime);
};

}  // namespace pal4
