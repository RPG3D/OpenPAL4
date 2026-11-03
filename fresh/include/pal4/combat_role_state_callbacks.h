#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ICombatRoleStateCallbackRuntime {
public:
    virtual ~ICombatRoleStateCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void DispatchUiEvent(
        std::uint32_t tree, std::uint32_t key_address,
        const std::array<std::uint32_t, 2>& payload) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    virtual void DispatchScriptEvent(std::uint32_t router,
                                     std::uint32_t key,
                                     std::uint32_t facet,
                                     std::uint32_t mode) = 0;
};

class CombatRoleStateCallbacks final {
public:
    static constexpr std::uint32_t kUiEventKeyAddress = 0x8b68f8u;
    static constexpr std::uint32_t kSubstituteKeyAddress = 0x8b62dcu;

    // IDA 0x551c30.
    [[nodiscard]] static std::uint8_t OnMouseClick(
        std::uint32_t event, ICombatRoleStateCallbackRuntime& runtime);
    // IDA 0x551c70.
    [[nodiscard]] static std::uint8_t OnSubstitutionClick(
        std::uint32_t event, ICombatRoleStateCallbackRuntime& runtime);
};

}  // namespace pal4
