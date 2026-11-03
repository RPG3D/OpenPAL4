#pragma once

#include "pal4/hint_window.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatHintWindowRuntime : public IHintWindowRuntime {
public:
    // Native owners release the existing host string instead of VC6 storage.
    virtual void ReleaseClockText(std::uint32_t record);
    virtual void LoadCombatLayout(std::uint32_t window,
                                  std::string_view layout_name,
                                  std::uint32_t layout_argument) = 0;
    virtual void AppendPrefixToClock(std::uint32_t clock_record,
                                     std::uint32_t prefix_record) = 0;
};

class ICombatHintWindowUiRuntime : public IHintWindowResizeRuntime {
public:
    virtual ~ICombatHintWindowUiRuntime() = default;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view source) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetText(std::uint32_t edit,
                         std::u32string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t DefaultSheet() = 0;
    virtual void SetVisibility(std::uint32_t window,
                               bool visible, std::uint32_t parent) = 0;
    virtual void SetPosition(std::uint32_t root,
                             std::uint32_t mode,
                             std::uint32_t position) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t root, bool enabled) = 0;
    [[nodiscard]] virtual float TextHeight(std::uint32_t edit) = 0;
    [[nodiscard]] virtual float MaxLineWidth(std::uint32_t edit) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetHeight(std::uint32_t window,
                           std::uint32_t mode, float value) = 0;
    virtual void SetWidth(std::uint32_t window,
                          std::uint32_t mode, float value) = 0;
    [[nodiscard]] virtual float GetX(std::uint32_t root) = 0;
    [[nodiscard]] virtual float GetY(std::uint32_t root) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t root) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t root) = 0;
    virtual void SetX(std::uint32_t root, float value) = 0;
    virtual void SetY(std::uint32_t root, float value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveQueued(std::uint32_t timers,
                              std::uint32_t clock_record) = 0;
    virtual void QueueClose(std::uint32_t timers,
                            std::uint32_t clock_record,
                            std::uint32_t duration,
                            std::uint32_t window,
                            std::uint32_t callback) = 0;
};

class ICombatHintWindowCleanupRuntime {
public:
    virtual ~ICombatHintWindowCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void SetVisibility(std::uint32_t window, bool visible,
                               std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RemoveTimer(
        std::uint32_t timers, std::uint32_t clock_record) = 0;
};

class CombatHintWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x8454b8u;
    static constexpr std::uint32_t kHandlerVtable = 0x8454b0u;
    static constexpr std::uint32_t kOnZOrderChanged = 0x54acf0u;
    static constexpr std::string_view kLayoutName = "CombathintWindow";
    static constexpr std::string_view kClockName = "CombatHintClock";
    static constexpr std::uint32_t kCleanupCallback = 0x54acb0u;

    // IDA CombatHintWindow_Construct at 0x549e60.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::uint32_t layout_argument,
        ICombatHintWindowRuntime& runtime);
    // IDA CombatHintWindow_Destruct at 0x54a1a0.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, ICombatHintWindowRuntime& runtime);
    // IDA CombatHintWindow_ShowText at 0x54a1f0.
    static void ShowText(std::uint32_t window, std::string_view source,
                         std::uint32_t position,
                         std::uint32_t duration,
                         std::uint32_t parent, std::uint8_t flag,
                         ICombatHintWindowUiRuntime& runtime);
    // IDA CombatHintWindow_Show at 0x54a450.
    static void Show(std::uint32_t window, std::u32string_view text,
                     std::uint32_t position, std::uint32_t duration,
                     std::uint32_t parent, std::uint8_t flag,
                     ICombatHintWindowUiRuntime& runtime);
    // IDA CombatHintWindow_Resize at 0x54a550.
    static void Resize(std::uint32_t window,
                       IHintWindowResizeRuntime& runtime);
    // IDA CombatHintWindow_SetFlag at 0x54a5f0.
    [[nodiscard]] static std::uint8_t SetFlag(
        std::uint32_t window, std::uint8_t value,
        ICombatHintWindowUiRuntime& runtime);
    // IDA CombatHintWindow_Cleanup at 0x54acb0.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t window, ICombatHintWindowCleanupRuntime& runtime);
    // IDA CombatHintWindow_OnZOrderChanged at 0x54acf0.
    [[nodiscard]] static bool OnZOrderChanged(
        std::uint32_t window, ICombatHintWindowCleanupRuntime& runtime);
};

}  // namespace pal4
