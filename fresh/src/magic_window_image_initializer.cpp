#include "pal4/magic_window_image_initializer.h"

namespace pal4 {

std::size_t MagicWindowImageInitializer::Initialize(
    std::uint32_t object, const MagicWindowState& state,
    const MagicWindowPanelState& panel,
    std::span<const MagicDbSkillRecord> records,
    IMagicWindowImageSetupRuntime& image_runtime,
    IMagicWindowSkillRowsRuntime& rows_runtime,
    IMagicWindowNumericRuntime& numeric_runtime) {
    const auto player = MagicWindowImageSetup::Prepare(
        object, state, panel, image_runtime);
    if (player == 0)
        return 0;

    const auto category_key = state.slot_order.at(state.category_position);
    const auto selection = MagicWindowSkillCatalog::Select(records, category_key);
    MagicWindowSkillRows::Bind(object, player, category_key,
                               panel, records, selection, rows_runtime);
    MagicWindowNumericDisplay::Update(player, numeric_runtime.RoleId(player), category_key,
                                      panel, numeric_runtime);
    return selection.VisibleCount();
}

}  // namespace pal4
