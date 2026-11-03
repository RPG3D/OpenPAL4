#include "pal4/hint_window.h"

namespace pal4 {

std::uint32_t HintWindow::Construct(
    const std::uint32_t window, const std::string_view layout_name,
    IHintWindowRuntime& runtime) {
    runtime.LoadLayout(window, layout_name);
    try {
        const auto clock = window + 48u;
        runtime.SetErrorFlag(clock, 0u);
        runtime.WriteWord(window, kVtable);
        if (runtime.AllocateText(clock,
                                 static_cast<std::uint32_t>(kClockName.size()),
                                 1u)) {
            runtime.CopyText(runtime.ReadWord(window + 52u), kClockName);
            runtime.SetTextSize(clock,
                                static_cast<std::uint32_t>(kClockName.size()));
        }
        runtime.WriteByte(window + 64u, 1u);

        const auto frame_name = runtime.GetNamePrefix(window + 24u) +
                                "ImgFrame";
        runtime.WriteWord(window + 44u, runtime.FindWindow(frame_name));
        const auto edit_name = runtime.GetNamePrefix(window + 24u) +
                               "EditMessage";
        const auto edit = runtime.FindWindow(edit_name);
        runtime.WriteWord(window + 40u, edit);
        runtime.SetAutoBreak(edit, false);

        const auto memory = runtime.Allocate(12u);
        if (memory != 0u) {
            runtime.WriteWord(memory + 4u, kOnZOrderChanged);
            runtime.WriteWord(memory + 8u, window);
            runtime.WriteWord(memory, kZOrderHandlerVtable);
        }
        const auto connection = runtime.Subscribe(
            runtime.ReadWord(window + 36u), kZOrderChangedEvent, memory);
        if (connection != 0u) runtime.ReleaseConnection(connection);
    } catch (...) {
        (void)Destruct(window, runtime);
        throw;
    }
    return window;
}

std::uint32_t HintWindow::Destruct(
    const std::uint32_t window, IHintWindowRuntime& runtime) {
    runtime.WriteWord(window, kVtable);
    const auto data = runtime.ReadWord(window + 52u);
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window + 56u, 0u);
    runtime.WriteWord(window + 60u, 0u);
    return runtime.DestroyBase(window);
}

}  // namespace pal4
