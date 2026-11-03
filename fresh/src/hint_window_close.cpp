#include "pal4/hint_window.h"

namespace pal4 {

int HintWindow::Close(const std::uint32_t window,
                      IHintWindowCloseRuntime& runtime) {
    const auto root = runtime.ReadWord(window + 36u);
    const auto parent = runtime.ReadWord(root + 68u);
    if (parent != 0u) runtime.RemoveChild(parent, root);
    return runtime.SetVisibility(window, false, 0u);
}

}  // namespace pal4
