#include "pal4/combat_hint_window.h"

#include <bit>

namespace pal4 {

std::uint32_t CombatHintWindow::Cleanup(
    const std::uint32_t window, ICombatHintWindowCleanupRuntime& runtime) {
    const auto root = runtime.ReadWord(window + 36u);
    const auto parent = runtime.ReadWord(root + 68u);
    if (parent != 0u) runtime.RemoveChild(parent, root);
    runtime.SetVisibility(window, false, 0u);
    const auto manager = runtime.FrameManager();
    return runtime.RemoveTimer(runtime.ReadWord(manager + 0x1a0u),
                               window + 48u);
}

bool CombatHintWindow::OnZOrderChanged(
    const std::uint32_t window, ICombatHintWindowCleanupRuntime& runtime) {
    if (runtime.ReadByte(window + 64u) != 0u &&
        std::bit_cast<float>(runtime.ReadWord(window + 68u)) >= 0.0f)
        (void)Cleanup(window, runtime);
    return true;
}

}  // namespace pal4
