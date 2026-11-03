#include "pal4/ui_object_text_display.h"

#include <stdexcept>

namespace pal4 {

bool UiObjectTextDisplay::HandleClick(
    UiObjectState& state, std::uint32_t object_address,
    std::uint32_t event_args_address, IUiObjectTextRuntime& runtime) {
    const auto clicked_window = runtime.EventWindow(event_args_address + 8u);
    (void)SetTextAndVisibility(state, object_address, clicked_window, runtime);
    return true;
}

int UiObjectTextDisplay::SetTextAndVisibility(
    UiObjectState& state, std::uint32_t object_address,
    std::uint32_t clicked_window, IUiObjectTextRuntime& runtime) {
    runtime.SetEnabled(state.Read(0x48u), true);
    runtime.SetEnabled(state.Read(0x4cu), true);
    runtime.SetEnabled(clicked_window, false);
    runtime.SetVisible(state.Read(0x50u), true);

    const auto id = runtime.WindowUserId(clicked_window);
    const auto database = runtime.GameDatabase();
    const auto record = runtime.TriggerRecord(database + 0x34u, id);
    const auto source = runtime.TriggerText(record);
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto end = utf8.find('\0');
    const auto length = end == std::string::npos ? utf8.size() : end;
    if (length == runtime.CeguiNpos()) {
        throw std::length_error(
            "Length for utf8 encoded string can not be 'npos'");
    }
    const auto cegui_string = runtime.BuildCeguiString(
        std::string_view(utf8.data(), length),
        static_cast<std::uint32_t>(length));
    try {
        runtime.SetText(state.Read(0x40u), cegui_string);
    } catch (...) {
        runtime.DestroyCeguiString(cegui_string);
        throw;
    }
    runtime.DestroyCeguiString(cegui_string);
    const auto manager = runtime.FrameManager();
    return runtime.InsertActive(manager + 0x134u, object_address + 0x54u);
}

}  // namespace pal4
