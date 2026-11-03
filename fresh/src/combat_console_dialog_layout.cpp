#include "pal4/combat_console.h"

namespace pal4 {

void CombatConsoleWindow::UpdateDialogLayout(
    const std::uint32_t console, ICombatConsoleDialogRuntime& runtime) {
    const auto edit = runtime.ReadWord(console + 264u);
    const auto height = runtime.GetTextHeight(edit) + 5.0f;
    const auto height_delta = height - runtime.ReadFloat(edit + 272u);
    runtime.SetHeight(edit, 1u, height);
    const auto panel = runtime.ReadWord(console + 260u);
    runtime.SetHeight(panel, 1u,
                      runtime.ReadFloat(panel + 272u) + height_delta);

    const auto width = runtime.GetMaxLineWidth(edit) + 5.0f;
    const auto width_delta = width - runtime.ReadFloat(edit + 268u);
    runtime.SetWidth(edit, 1u, width);
    runtime.SetWidth(panel, 1u,
                     runtime.ReadFloat(panel + 268u) + width_delta);
}

}  // namespace pal4
