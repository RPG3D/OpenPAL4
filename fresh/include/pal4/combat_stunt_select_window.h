#pragma once

#include "pal4/combat_magic_select_window.h"

namespace pal4 {

class CombatStuntSelectWindow final {
public:
    static constexpr std::uint32_t kLayoutName = 0x8b7618u;
    static constexpr std::uint32_t kResourceGroup = 0x8b7630u;
    static constexpr std::uint32_t kVtable = 0x84566cu;
    static constexpr std::uint32_t kFacetVtable = 0x845654u;
    static constexpr std::uint32_t kEventVtable = 0x845684u;
    static constexpr std::uint32_t kHoverCallback = 0x554d60u;
    static constexpr std::uint32_t kListClickCallback = 0x554d00u;
    static constexpr std::uint32_t kRootClickCallback = 0x555010u;
    static constexpr std::uint32_t kKeyCallback = 0x555030u;

    // IDA 0x554220-0x555010.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Setup(
        std::uint32_t window, std::uint32_t pair, std::uint32_t actor,
        std::uint32_t parent, ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeButtons(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    static void UpdateStuntList(std::uint32_t window,
                                ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetHoveredStunt(
        std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleListClick(
        std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleListClickInternal(
        std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateStuntDescription(
        std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t window, ICombatStuntSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleRootClick(
        std::uint32_t window, std::uint32_t event,
        ICombatStuntSelectWindowRuntime& runtime);
    // IDA555030: input facet is window+40; caller supplies the window owner.
    [[nodiscard]] static std::uint8_t HandleKeyDown(
        std::uint32_t window, std::uint32_t event,
        ICombatStuntSelectWindowRuntime&, ICombatSelectListNavigationRuntime&);
};

}  // namespace pal4
