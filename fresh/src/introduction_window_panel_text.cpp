#include "pal4/introduction_window_panel_text.h"

#include <stdexcept>

namespace pal4 {

void IntroductionWindowPanelText::ShowMainGroup(
    IntroductionWindowState& state, std::uint32_t object_address,
    std::uint32_t clicked_window,
    IIntroductionWindowPanelRuntime& runtime) {
    IntroductionWindowVisibility::SetPanelVisibility(state, true, runtime);
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        runtime.SetVisible(state.Read(0x98u + i * 4u), false);
    }
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        runtime.SetEnabled(state.Read(0x88u + i * 4u), true);
    }
    runtime.SetEnabled(clicked_window, false);

    const auto group_id = runtime.WindowUserId(clicked_window);
    if (group_id >= 4) return;

    const auto collection = object_address + 0x40u +
                            static_cast<std::uint32_t>(group_id) * 0x10u;
    const auto begin = runtime.ReadWord(collection + 4u);
    const auto count = begin == 0u ? 0 :
        static_cast<std::int32_t>(runtime.ReadWord(collection + 8u) - begin)
            >> 2;
    std::uint32_t first_window = 0u;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto trigger_id = runtime.ReadWord(
            begin + static_cast<std::uint32_t>(index) * 4u);
        const auto database = runtime.GameDatabase();
        if (!runtime.HasTrigger(database + 0x34u, trigger_id)) continue;

        const auto record_database = runtime.GameDatabase();
        const auto record = runtime.TriggerRecord(
            record_database + 0x34u, trigger_id);
        const auto slot = object_address + 0x98u +
                          static_cast<std::uint32_t>(index) * 4u;
        const auto subwindow = runtime.SubWindowAt(state, slot);
        runtime.SetVisible(subwindow, true);
        const auto source = runtime.TriggerText(record);
        const auto utf8 = runtime.ConvertToUtf8(source);
        const auto end = utf8.find('\0');
        const auto length = end == std::string::npos ? utf8.size() : end;
        if (length == runtime.CeguiNpos()) {
            throw std::length_error(
                "Length for utf8 encoded string can not be 'npos'");
        }
        const auto text = runtime.BuildCeguiString(
            std::string_view(utf8.data(), length),
            static_cast<std::uint32_t>(length));
        try {
            runtime.SetText(subwindow, text);
        } catch (...) {
            runtime.DestroyCeguiString(text);
            throw;
        }
        runtime.DestroyCeguiString(text);
        runtime.SetWindowUserId(subwindow, trigger_id);
        if (first_window == 0u) first_window = subwindow;
    }
    if (first_window != 0u) {
        runtime.SelectFirstSub(state, first_window);
    }
    state.Write(0x3cu, static_cast<std::uint32_t>(group_id));
    const auto manager = runtime.FrameManager();
    runtime.InsertActive(manager + 0x134u, object_address + 0xb0u);
}

void IntroductionWindowPanelText::ShowSubSelection(
    IntroductionWindowState& state, std::uint32_t clicked_window,
    IIntroductionWindowPanelRuntime& runtime) {
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        runtime.SetEnabled(state.Read(0x98u + i * 4u), true);
    }
    runtime.SetEnabled(clicked_window, false);

    const auto trigger_id = static_cast<std::uint32_t>(
        runtime.WindowUserId(clicked_window));
    const auto database = runtime.GameDatabase();
    if (!runtime.HasTrigger(database + 0x34u, trigger_id)) return;

    const auto record_database = runtime.GameDatabase();
    const auto record = runtime.TriggerRecord(
        record_database + 0x34u, trigger_id);
    const auto source = runtime.TriggerDescription(record);
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto end = utf8.find('\0');
    const auto length = end == std::string::npos ? utf8.size() : end;
    if (length == runtime.CeguiNpos()) {
        throw std::length_error(
            "Length for utf8 encoded string can not be 'npos'");
    }
    const auto text = runtime.BuildCeguiString(
        std::string_view(utf8.data(), length),
        static_cast<std::uint32_t>(length));
    try {
        runtime.SetText(state.Read(0x80u), text);
    } catch (...) {
        runtime.DestroyCeguiString(text);
        throw;
    }
    runtime.DestroyCeguiString(text);
}

std::uint8_t IntroductionWindowPanelText::OnMainButtonEvent(
    IntroductionWindowState& state, std::uint32_t object_address,
    std::uint32_t event_address,
    IIntroductionWindowPanelRuntime& runtime) {
    ShowMainGroup(state, object_address,
                  runtime.ReadWord(event_address + 8u), runtime);
    return 1u;
}

std::uint8_t IntroductionWindowPanelText::OnSubButtonEvent(
    IntroductionWindowState& state, std::uint32_t event_address,
    IIntroductionWindowPanelRuntime& runtime) {
    ShowSubSelection(state, runtime.ReadWord(event_address + 8u), runtime);
    return 1u;
}

}  // namespace pal4
