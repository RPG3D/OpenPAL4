#include "pal4/menu_window_actions.h"

#include <cstdint>

namespace pal4 {

std::uint8_t MenuWindowActions::HandleClick(
    const std::uint32_t menu, IMenuWindowActionRuntime& runtime) {
    static_cast<void>(Close(menu, runtime));
    return 1u;
}

std::uint8_t MenuWindowActions::HandleButtonClick(
    const std::uint32_t menu, const std::uint32_t event,
    IMenuWindowActionRuntime& runtime) {
    runtime.CopyWindowEventArgs(event);
    const auto window = runtime.ReadWord(event + 8u);
    const auto index = runtime.ReadWord(window + 280u);
    const auto begin = runtime.ReadWord(menu + 68u);
    if (begin != 0u) {
        const auto end = runtime.ReadWord(menu + 72u);
        if (index < (end - begin) / 164u) {
            runtime.RemoveCurrentHandler(menu + 40u);
            static_cast<void>(Close(menu, runtime));
            runtime.ProcessButtonData(begin + index * 164u);
        }
    }
    runtime.DestroyWindowEventArgs();
    return 1u;
}

std::uint8_t MenuWindowActions::HandleClickEvent(
    const std::uint32_t menu, const std::uint32_t event,
    IMenuWindowActionRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 0u)
        static_cast<void>(Close(menu, runtime));
    return 1u;
}

std::uint32_t MenuWindowActions::Close(
    const std::uint32_t menu, IMenuWindowActionRuntime& runtime) {
    runtime.HideAndCleanup(menu);
    return runtime.CallVirtualSlot4(menu, 0u, 0u);
}

void MenuWindowActions::AdjustHeight(
    const std::uint32_t menu, IMenuWindowActionRuntime& runtime) {
    const auto model = runtime.ReadWord(menu + 84u);
    const auto y = runtime.GetYPosition(model, 1u);
    const auto begin = runtime.ReadWord(menu + 68u);
    const auto count = begin == 0u
        ? 0 : static_cast<std::int32_t>(
                  runtime.ReadWord(menu + 72u) - begin) / 164;
    const auto height = runtime.GetHeight(model, 1u);
    runtime.SetHeight(runtime.ReadWord(menu + 80u), 1u,
                      y + y + height * static_cast<float>(count));
}

}  // namespace pal4
