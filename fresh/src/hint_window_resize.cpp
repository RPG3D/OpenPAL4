#include "pal4/combat_hint_window.h"
namespace pal4 {
void CombatHintWindow::Resize(
    const std::uint32_t window, IHintWindowResizeRuntime& runtime) {
    auto edit = runtime.ReadWord(window + 40u);
    const auto edit_height = runtime.TextHeight(edit) + 1.0f;
    const auto height_delta = edit_height - runtime.ReadFloat(edit + 272u);
    runtime.SetHeight(edit, 1u, edit_height);
    auto frame = runtime.ReadWord(window + 44u);
    runtime.SetHeight(frame, 1u,
                      runtime.ReadFloat(frame + 272u) + height_delta);

    edit = runtime.ReadWord(window + 40u);
    const auto edit_width = runtime.MaxLineWidth(edit) + 7.0f;
    const auto width_delta = edit_width - runtime.ReadFloat(edit + 268u);
    runtime.SetWidth(edit, 1u, edit_width);
    frame = runtime.ReadWord(window + 44u);
    runtime.SetWidth(frame, 1u,
                     runtime.ReadFloat(frame + 268u) + width_delta);
}

}
