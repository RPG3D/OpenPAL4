#include "pal4/magic_window_skill_rows.h"

namespace pal4 {

void MagicWindowSkillRows::Bind(
    std::uint32_t object, std::uint32_t player,
    std::uint32_t category_key,
    const MagicWindowPanelState& panel,
    std::span<const MagicDbSkillRecord> records,
    const MagicSkillSelection& selection,
    IMagicWindowSkillRowsRuntime& runtime) {
    for (std::size_t row = 0; row < selection.VisibleCount(); ++row) {
        const auto& record = records[selection.matching_indices.at(row)];
        const auto name_window = panel.At(0xc0u + 4u * static_cast<std::uint32_t>(row));
        const auto icon_window = panel.At(0xd8u + 4u * static_cast<std::uint32_t>(row));
        const auto button = panel.At(0xf0u + 4u * static_cast<std::uint32_t>(row));

        runtime.SetText(name_window, runtime.ReadNameUtf8(record.name_address));
        runtime.SetVisible(button, true);
        const auto state_is_three = runtime.SkillStateIsThree(record);
        const auto current = runtime.CurrentValue(player, record);
        const auto threshold_met = MagicWindowSkillCatalog::MeetsThreshold(current, record);
        runtime.SetButtonImages(button, MagicWindowSkillCatalog::ButtonImages(threshold_met));
        runtime.SetIcon(icon_window, "gy0", MagicWindowSkillCatalog::IconForKind(record.kind));

        const auto total = threshold_met ? 0 : runtime.TotalValue(player, category_key);
        const auto remaining = threshold_met ? 0 : runtime.RemainingValue(player);
        const auto action = MagicWindowSkillCatalog::ChooseAction(
            record, current, total, remaining, state_is_three);
        runtime.SetText(button, runtime.ActionText(action.trigger_id, action.format_value));
        runtime.SetEnabled(button, action.enabled);
        runtime.SetButtonRecordKey(button, record.key);
    }
    if (selection.VisibleCount() != 0)
        runtime.UpdateDetails(object);
}

}  // namespace pal4
