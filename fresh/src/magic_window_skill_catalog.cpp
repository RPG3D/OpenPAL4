#include "pal4/magic_window_skill_catalog.h"

#include <algorithm>
#include <bit>

namespace pal4 {

std::size_t MagicSkillSelection::VisibleCount() const noexcept {
    return std::min(matching_indices.size(), MagicWindowSkillCatalog::kVisibleLimit);
}

MagicSkillSelection MagicWindowSkillCatalog::Select(
    std::span<const MagicDbSkillRecord> records, std::uint32_t category) {
    MagicSkillSelection selection;
    for (std::size_t index = 0; index < records.size(); ++index) {
        if (records[index].category == category)
            selection.matching_indices.push_back(index);
    }
    return selection;
}

std::string_view MagicWindowSkillCatalog::IconForKind(
    std::uint32_t kind) noexcept {
    if (kind == 0u) return "skillattack";
    if (kind == 2u) return "skillcure";
    return "skillbuff";
}

bool MagicWindowSkillCatalog::MeetsThreshold(
    std::int32_t current_value, const MagicDbSkillRecord& record) noexcept {
    return current_value >= std::bit_cast<std::int32_t>(record.threshold_3c);
}

MagicSkillButtonImages MagicWindowSkillCatalog::ButtonImages(
    bool threshold_met) noexcept {
    if (threshold_met)
        return {"bottonskill", "bottonskill1", "bottonskill3", "bottonskill2"};
    return {"bottonskilllearn", "bottonskilllearn1",
            "bottonskilllearn3", "bottonskilllearn2"};
}

MagicSkillActionPlan MagicWindowSkillCatalog::ChooseAction(
    const MagicDbSkillRecord& record, std::int32_t current_value,
    std::int32_t total_value, std::int32_t remaining_value,
    bool skill_state_is_three) noexcept {
    if (MeetsThreshold(current_value, record))
        return {skill_state_is_three ? 0x16u : 0x107u,
                skill_state_is_three, std::nullopt};

    const auto required = std::bit_cast<std::int32_t>(record.threshold_3c);
    const auto minimum_total =
        std::bit_cast<std::int32_t>(record.minimum_total_40);
    return {0xbcu, remaining_value >= required && total_value >= minimum_total,
            required};
}

}  // namespace pal4
