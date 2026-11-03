#pragma once
#include "pal4/script_combat_config.h"

namespace pal4 {
// Persistent configuration only. The battle session borrows this owner; setting
// these values does not enter PALIV_COMBAT or produce a battle result.
class NativeScriptCombatConfig final : public ICombatConfigurationState {
public:
    NativeScriptCombatConfig();
    void SetText(CombatConfigTextSlot slot, std::string_view text) override;
    std::uint32_t CombatSystem() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    CombatConfigText ReadTextProperty(std::uint32_t address) override;
    void ApplyConfiguration(const CombatConfigSnapshot& config) override;
    std::uint32_t ReleaseText(CombatConfigText& text, std::uint32_t index) noexcept override;
    [[nodiscard]] CombatConfigSnapshot Snapshot() const;
    void SetCombatProperty110(std::uint32_t value);
    void Bind(ICombatConfigurationState&);
    void Unbind() noexcept;

private:
    // Scoped logical address, used only by shared Configure's scalar accessors.
    static constexpr std::uint32_t kOwner = 0x1000u;
    CombatConfigSnapshot config_{};
    ICombatConfigurationState* live_{};
};
}
