#include "pal4/ol_info_visibility.h"

namespace pal4 {
namespace {

void SetText(std::uint32_t window, std::string_view source,
             IOlInfoVisibilityRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto nul = utf8.find('\0');
    const auto length = nul == std::string::npos ? utf8.size() : nul;
    const auto text = runtime.CreateCeguiString(
        std::string_view(utf8.data(), length));
    try {
        runtime.SetText(window, text);
    } catch (...) {
        runtime.DestroyCeguiString(text);
        throw;
    }
    runtime.DestroyCeguiString(text);
}

}  // namespace

std::uint32_t OlInfoVisibility::SetVisible(
    OlInfoState& state, std::uint32_t object_address,
    std::uint8_t showing, std::uint32_t parent,
    IOlInfoVisibilityRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        const auto window = state.Read(0x2cu + index * 4u);
        SetText(window, runtime.TriggerText(219u + index), runtime);
        runtime.SetWindowVisible(window, false);
    }
    if (runtime.ProgressCount() == 1) {
        for (std::uint32_t index = 0u; index < 4u; ++index)
            runtime.SetWindowVisible(state.Read(0x2cu + index * 4u), true);
    }
    if (runtime.ProgressCount() > 1) {
        for (std::uint32_t index = 0u; index < 8u; ++index)
            runtime.SetWindowVisible(state.Read(0x2cu + index * 4u), true);
    }
    SetText(state.Read(0x4cu), runtime.TriggerText(0xe3u), runtime);

    const auto manager = runtime.FrameManager();
    const auto active = manager + 0x134u;
    const auto member = object_address + 0x28u;
    return showing != 0u ? runtime.InsertActive(active, member) :
                           runtime.RemoveActive(active, member);
}

}  // namespace pal4
