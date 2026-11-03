#include "pal4/script_ui_timer.h"

namespace pal4 {

std::uint32_t ScriptUiTimer::Start(
    const std::uint32_t period, const UiFrameTimerText& source,
    IScriptUiTimerRuntime& runtime) {
    UiFrameTimerText text{source.control & 0xffu, 0u, 0u, 0u};
    bool copy = true;
    if (source.length != 0u) {
        const auto data = source.data != 0u ? source.data : runtime.EmptyText();
        if (runtime.ReadReference(data) < 0xfeu) {
            runtime.SetErrorFlag(text, 1u);
            text.data = source.data != 0u ? source.data : runtime.EmptyText();
            text.length = source.length;
            text.capacity = source.capacity;
            const auto reference = runtime.ReadReference(text.data);
            runtime.WriteReference(text.data,
                                   static_cast<std::uint8_t>(reference + 1u));
            copy = false;
        }
    }
    if (copy && runtime.AllocateText(text, source.length, 1u)) {
        runtime.CopyText(text.data,
                         source.data != 0u ? source.data : runtime.EmptyText(),
                         source.length);
        runtime.SetTextSize(text, source.length);
    }
    return runtime.InitializeTimerFrame(runtime.FrameManager(), period, text);
}

}  // namespace pal4
