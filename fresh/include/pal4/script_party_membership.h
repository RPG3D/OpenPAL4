#pragma once

#include "pal4/player_manager_slot_inventory_state.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace pal4 {

class IScriptPartyMembershipRuntime : public IPlayerManagerSlotInventoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    virtual void SortPlayers(std::vector<std::uint32_t>& players,
                             std::uint32_t comparator) = 0;
    virtual void SetPartyOrder(std::uint32_t player_state,
                               std::uint32_t position) = 0;
    [[nodiscard]] virtual std::uint32_t FreePartyList(
        const std::vector<std::uint32_t>& players) noexcept = 0;

    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t frames) = 0;
    virtual void InitializeHintText(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::string_view PlayerName(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::string_view GameString(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void FormatHintText(std::string_view format,
                                std::string_view name) = 0;
    virtual void AppendHintText() = 0;
    [[nodiscard]] virtual std::string_view HintText() = 0;
    virtual void ShowHint(std::uint32_t window, std::string_view text,
                          float x, float y, float scale,
                          std::uint32_t mode, std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlaySound(std::uint32_t audio, std::uint8_t flag,
                           std::string_view path, std::uint32_t mode) = 0;
    virtual void CleanupHintText(std::uint32_t buffer) noexcept = 0;
};

class ScriptPartyMembership final {
public:
    // IDA 0x5d4950, giPlayerInTeam.
    [[nodiscard]] static std::uint32_t Add(
        std::uint32_t index, std::uint8_t show_hint,
        IScriptPartyMembershipRuntime& runtime);
    // IDA 0x5d4b60, giPlayerOutTeam.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t index, std::uint8_t show_hint,
        IScriptPartyMembershipRuntime& runtime);
};

}  // namespace pal4
