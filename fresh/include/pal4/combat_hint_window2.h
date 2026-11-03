#pragma once

#include "pal4/combat_hint_window.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatHintWindow2UiRuntime : public ICombatHintWindowUiRuntime {
public:
    virtual void QueueClose2(std::uint32_t timers,
                             std::uint32_t clock_record,
                             std::uint32_t duration,
                             std::uint32_t window,
                             std::uint32_t callback) = 0;
};

class CombatHintWindow2 final {
public:
    static constexpr std::uint32_t kVtable = 0x8454d8u;
    static constexpr std::uint32_t kHandlerVtable = 0x8454d0u;
    static constexpr std::string_view kLayoutName = "Combathint2Window";
    static constexpr std::string_view kClockName = "CombatHint2Clock";

    // IDA CombatHintWindow2_Construct at 0x54a600.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, ICombatHintWindowRuntime& runtime);
    // IDA CombatHintWindow2_Delete at 0x54a8f0.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t window, std::uint8_t flags,
        ICombatHintWindowRuntime& runtime);
    // IDA CombatHintWindow2_Destruct at 0x54a910.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, ICombatHintWindowRuntime& runtime);
    // IDA CombatHintWindow2_ShowText at 0x54a960.
    static void ShowText(std::uint32_t window, std::string_view source,
                         std::uint32_t position,
                         std::uint32_t duration,
                         std::uint32_t parent, std::uint8_t flag,
                         ICombatHintWindow2UiRuntime& runtime);
    // IDA CombatHintWindow2_Show at 0x54abc0.
    static void Show(std::uint32_t window, std::u32string_view text,
                     std::uint32_t position, std::uint32_t duration,
                     std::uint32_t parent, std::uint8_t flag,
                     ICombatHintWindow2UiRuntime& runtime);
};

}  // namespace pal4
