#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::InitializePortraitTimer(
    const std::uint32_t manager, const std::uint32_t period,
    const std::uint8_t text_control, const std::uint32_t text_data,
    const std::uint32_t text_length, const std::uint32_t text_capacity,
    IUiFrameManagerTimerRuntime& runtime) {
    auto result = runtime.FindFrame(manager, "portrait");
    if (result != 0u) {
        UiFrameTimerText text{text_control, 0u, 0u, 0u};
        bool copied = true;
        if (text_length != 0u) {
            const auto source = text_data != 0u
                                    ? text_data : runtime.EmptyText();
            if (runtime.ReadReference(source) < 0xfeu) {
                runtime.SetErrorFlag(text, 1u);
                text.data = source;
                text.length = text_length;
                text.capacity = text_capacity;
                runtime.WriteReference(
                    source, static_cast<std::uint8_t>(
                                runtime.ReadReference(source) + 1u));
                copied = false;
            }
        }
        if (copied && runtime.AllocateText(text, text_length, 1u)) {
            const auto source = text_data != 0u
                                    ? text_data : runtime.EmptyText();
            runtime.CopyText(text.data, source, text_length);
            runtime.SetTextSize(text, text_length);
        }
        result = runtime.InitializeTimer(result, period, text);
    }

    if (text_data != 0u) {
        const auto reference = runtime.ReadReference(text_data);
        if (reference == 0u || reference == 0xffu)
            return runtime.Free(text_data - 1u);
        runtime.WriteReference(text_data,
                               static_cast<std::uint8_t>(reference - 1u));
        return text_data - 1u;
    }
    return result;
}

}  // namespace pal4
