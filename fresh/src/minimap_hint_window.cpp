#include "pal4/minimap_hint_window.h"

namespace pal4 {

std::uint32_t MinimapHintWindow::Construct(
    const std::uint32_t window, IMinimapHintWindowRuntime& runtime) {
    (void)HintWindow::Construct(window, kLayout, runtime);
    runtime.WriteWord(window, kVtable);

    constexpr auto length = static_cast<std::uint32_t>(kClockName.size());
    const auto record = window + 48u;
    const auto data = runtime.ReadWord(window + 52u);
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference != 0u && reference != 0xffu) {
            runtime.Reallocate(record, length);
        } else {
            const auto capacity = runtime.ReadWord(window + 60u);
            if (capacity > 0x1fu || capacity < length) {
                runtime.SetErrorFlag(record, 1u);
                runtime.Reallocate(record, length);
            }
        }
    } else {
        const auto capacity = runtime.ReadWord(window + 60u);
        if (capacity > 0x1fu || capacity < length) {
            runtime.SetErrorFlag(record, 1u);
            runtime.Reallocate(record, length);
        }
    }
    const auto destination = runtime.ReadWord(window + 52u);
    runtime.CopyText(destination, kClockName);
    runtime.WriteWord(window + 56u, length);
    runtime.WriteByte(destination + length, 0u);
    return window;
}

}  // namespace pal4
