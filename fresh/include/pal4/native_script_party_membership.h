#pragma once

#include "pal4/script_party_membership.h"
#include "pal4/native_player_inventory_state_runtime.h"
#include <array>
#include <string>

namespace pal4 {
class IScriptInventoryChangeRuntime;
// Borrows persistent player/inventory owners and the existing UI/audio boundary.
class NativeScriptPartyMembership final : public IScriptPartyMembershipRuntime {
public:
    NativeScriptPartyMembership(PlayerManagerObjectState& players,
        NativeBaseObjectMemory& memory, NativeInventorySession& inventory,
        IScriptInventoryChangeRuntime& hints);
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t manager, std::uint32_t index) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t InventoryManager() override;
    std::uint32_t FindItem(std::uint32_t collection, std::uint32_t key, std::uint32_t category) override;
    std::uint8_t CheckPlayerCondition(std::uint32_t item) override;
    void SortPlayers(std::vector<std::uint32_t>& players, std::uint32_t comparator) override;
    void SetPartyOrder(std::uint32_t script, std::uint32_t position) override;
    std::uint32_t FreePartyList(const std::vector<std::uint32_t>&) noexcept override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateHintWindow(std::uint32_t frames) override;
    void InitializeHintText(std::uint32_t buffer) override;
    std::string_view PlayerName(std::uint32_t player) override;
    std::uint32_t GameDatabase() override;
    std::string_view GameString(std::uint32_t table, std::uint32_t id) override;
    void FormatHintText(std::string_view format, std::string_view name) override;
    void AppendHintText() override;
    std::string_view HintText() override;
    void ShowHint(std::uint32_t window, std::string_view text, float x, float y,
        float scale, std::uint32_t mode, std::uint32_t enabled) override;
    std::uint32_t AudioManager() override;
    void PlaySound(std::uint32_t audio, std::uint8_t flag,
        std::string_view path, std::uint32_t mode) override;
    void CleanupHintText(std::uint32_t buffer) noexcept override;
private:
    void ValidatePlayer(std::uint32_t player);
    PlayerManagerObjectState& players_;
    NativeBaseObjectMemory& memory_;
    NativePlayerInventoryStateRuntime equipment_;
    IScriptInventoryChangeRuntime& hints_;
    std::array<std::string, 2> text_;
};
}
