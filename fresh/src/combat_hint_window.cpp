#include "pal4/combat_hint_window.h"
#include <initializer_list>

namespace pal4 {

void ICombatHintWindowRuntime::ReleaseClockText(std::uint32_t record) {
    const auto data=ReadWord(record+4u);
    if(data) {
        const auto reference=ReadByte(data-1u);
        if(reference==0u || reference==0xffu)Free(data-1u);
        else WriteByte(data-1u,static_cast<std::uint8_t>(reference-1u));
    }
    for(auto offset:{4u,8u,12u})WriteWord(record+offset,0u);
}

std::uint32_t CombatHintWindow::Construct(
    const std::uint32_t window, const std::uint32_t layout_argument,
    ICombatHintWindowRuntime& runtime) {
    runtime.LoadCombatLayout(window, kLayoutName, layout_argument);
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
        runtime.AppendPrefixToClock(clock, window + 20u);
        runtime.WriteByte(window + 64u, 1u);

        const auto prefix = runtime.GetNamePrefix(window + 20u);
        runtime.WriteWord(window + 44u,
                          runtime.FindWindow(prefix + "ImgFrame"));
        const auto edit = runtime.FindWindow(prefix + "EditMessage");
        runtime.WriteWord(window + 40u, edit);
        runtime.SetAutoBreak(edit, false);

        const auto callback = runtime.Allocate(12u);
        if (callback != 0u) {
            runtime.WriteWord(callback + 4u, kOnZOrderChanged);
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

std::uint32_t CombatHintWindow::Destruct(
    const std::uint32_t window, ICombatHintWindowRuntime& runtime) {
    runtime.WriteWord(window, kVtable);
    runtime.ReleaseClockText(window+48u);
    return runtime.DestroyBase(window);
}

}  // namespace pal4
