#include "pal4/magic_window_numeric_display.h"

#include <string>

namespace pal4 {

void MagicWindowNumericDisplay::Update(
    std::uint32_t player, std::uint32_t role_id,
    std::uint32_t category_key, const MagicWindowPanelState& panel,
    IMagicWindowNumericRuntime& runtime) {
    const auto base = runtime.QueryPlayerValue(player, role_id);
    runtime.SetText(panel.At(0x90u), std::to_string(base));

    const auto remaining = runtime.RemainingValue(player, base);
    runtime.SetText(panel.At(0x94u), std::to_string(remaining));

    const auto total = runtime.TotalValue(player, category_key);
    runtime.SetText(panel.At(0x98u), std::to_string(total));
}

}  // namespace pal4
