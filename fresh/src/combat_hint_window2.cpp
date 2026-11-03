#include "pal4/combat_hint_window2.h"

namespace pal4 {

std::uint32_t CombatHintWindow2::Construct(
    const std::uint32_t window, ICombatHintWindowRuntime& runtime) {
    runtime.LoadCombatLayout(window, kLayoutName, 0u);
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

        const auto prefix = runtime.GetNamePrefix(window + 20u);
        runtime.WriteWord(window + 44u,
                          runtime.FindWindow(prefix + "StaticPanel"));
        runtime.WriteWord(window + 40u,
                          runtime.FindWindow(prefix + "StaticText"));

        const auto callback = runtime.Allocate(12u);
        if (callback != 0u) {
            runtime.WriteWord(callback + 4u,
                              CombatHintWindow::kOnZOrderChanged);
            runtime.WriteWord(callback + 8u, window);
            runtime.WriteWord(callback, kHandlerVtable);
        }
        const auto connection = runtime.Subscribe(
            runtime.ReadWord(window + 36u),
            HintWindow::kZOrderChangedEvent, callback);
        if (connection != 0u)
            runtime.ReleaseConnection(connection);
    } catch (...) {
        (void)Destruct(window, runtime);
        throw;
    }
    return window;
}

std::uint32_t CombatHintWindow2::Delete(
    const std::uint32_t window, const std::uint8_t flags,
    ICombatHintWindowRuntime& runtime) {
    (void)Destruct(window, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(window);
    return window;
}

std::uint32_t CombatHintWindow2::Destruct(
    const std::uint32_t window, ICombatHintWindowRuntime& runtime) {
    runtime.WriteWord(window, kVtable);
    runtime.ReleaseClockText(window+48u);
    return runtime.DestroyBase(window);
}

}  // namespace pal4
