#pragma once

#include "pal4/combat_magic_select_window.h"

namespace pal4 {

class CombatPropertySelectWindow final : public CombatMagicSelectBase {
public:
    static constexpr std::uint32_t kLayoutName = 0x8b75bcu;
    static constexpr std::uint32_t kResourceGroup = 0x8b75d8u;
    static constexpr std::uint32_t kVtable = 0x845630u;
    static constexpr std::uint32_t kFacetVtable = 0x845618u;
    static constexpr std::uint32_t kEventVtable = 0x84564cu;
    static constexpr std::uint32_t kHoverCallback = 0x553e90u;
    static constexpr std::uint32_t kListClickCallback = 0x553e30u;
    static constexpr std::uint32_t kButtonClickCallback = 0x553e10u;

    // IDA 0x553430.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    // IDA 0x5534b0.
    [[nodiscard]] static std::uint32_t Setup(
        std::uint32_t window, std::uint32_t pair, std::uint32_t actor,
        std::uint32_t parent, ICombatSelectWindowRuntime& runtime);
    // IDA 0x553600.
    [[nodiscard]] static std::uint32_t InitializeButtons(
        std::uint32_t window, ICombatSelectConstructionRuntime& runtime);
    // IDA 0x553870.
    static void UpdatePropertyList(std::uint32_t window,
                                   ICombatSelectWindowRuntime& runtime);
    // IDA 0x553d50.
    [[nodiscard]] static std::uint32_t GetHoveredProperty(
        std::uint32_t window, ICombatSelectWindowRuntime& runtime);
    // IDA 0x553d80.
    static void SelectButton(std::uint32_t window, std::uint32_t selection,
                             ICombatSelectWindowRuntime& runtime);
    // IDA 0x553e10.
    [[nodiscard]] static std::uint8_t HandleButtonClick(
        std::uint32_t window, std::uint32_t event,
        ICombatSelectWindowRuntime& runtime);
    // IDA 0x553e30 and 0x553e40.
    [[nodiscard]] static std::uint8_t HandleListClick(
        std::uint32_t window, ICombatSelectWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleListClickInternal(
        std::uint32_t window, ICombatSelectWindowRuntime& runtime);
    // IDA 0x553e90.
    [[nodiscard]] static std::uint8_t UpdatePropertyDescription(
        std::uint32_t window, ICombatSelectWindowRuntime& runtime);
    // IDA 0x554040.
    [[nodiscard]] static bool CheckEnabled(
        std::uint32_t selection, ICombatSelectWindowRuntime& runtime);
    // IDA 0x554110.
    [[nodiscard]] static std::int32_t FindNextEnabledButton(
        std::uint32_t window, std::int32_t current,
        ICombatSelectWindowRuntime& runtime);
    // IDA554150 receives facet(window+48); this boundary takes the window.
    [[nodiscard]] static std::uint8_t HandleKeyDown(std::uint32_t window,std::uint32_t event,
        ICombatMagicSelectWindowRuntime&,ICombatSelectListNavigationRuntime&);
};

}  // namespace pal4
