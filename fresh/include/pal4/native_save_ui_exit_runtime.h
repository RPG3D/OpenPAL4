#pragma once
#include "pal4/save_ui_state.h"
#include "pal4/native_save_player_property_runtime.h"
#include "pal4/script_effect_callbacks.h"

namespace pal4 {
// Borrows persistent player properties and the existing arena effect group.
// SaveUiState::Finish owns the four-slot recovery and selected-player effect order.
class NativeSaveUiExitRuntime final : public ISaveUiExitRuntime {
public:
    NativeSaveUiExitRuntime(PlayerManagerObjectState& players, IScriptEffectRuntime& effects)
        : players_(players), properties_(players), effects_(effects) {}
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t manager, std::uint32_t index) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t GetComponentProperty(std::uint32_t component, std::uint32_t property) override;
    void SetComponentProperty(std::uint32_t component, std::uint32_t property, std::uint32_t value) override;
    std::uint32_t EffectManager() override;
    void CreateEffect(std::uint32_t manager, std::string_view name, std::uint32_t position,
                      std::uint32_t selector, std::uint32_t flags) override;
    void ResetTemporaryEffectHandle() override;
    bool EffectWasCreated() const noexcept { return effect_created_; }
private:
    PlayerManagerObjectState& players_;
    NativeSavePlayerPropertyRuntime properties_;
    IScriptEffectRuntime& effects_;
    ScriptEffectHandle temporary_{};
    bool pending_reset_{};
    bool effect_created_{};
};
} // namespace pal4
