#include "pal4/system_setting_navigation.h"

namespace pal4 {
namespace {

void Highlight(const std::uint32_t window, const bool active,
               ISystemSettingNavigationRuntime& runtime) {
    runtime.WriteByte(window + 1201u, active ? 1u : 0u);
    runtime.RequestRedraw(window);
}

void HighlightSlider(const std::uint32_t slider, const bool active,
                     ISystemSettingNavigationRuntime& runtime) {
    Highlight(runtime.ReadWord(slider + 1212u), active, runtime);
}

std::uint32_t NextEnabled(std::uint32_t record,
                          const std::uint32_t direction_offset,
                          ISystemSettingNavigationRuntime& runtime) {
    record = runtime.ReadWord(record + direction_offset);
    while (record != 0u &&
           runtime.ReadByte(runtime.ReadWord(record) + 1202u) != 0u) {
        record = runtime.ReadWord(record + direction_offset);
    }
    return record;
}

void SelectRecord(const std::uint32_t member,
                  const std::uint32_t record,
                  ISystemSettingNavigationRuntime& runtime) {
    const auto previous = runtime.ReadWord(member + 28u);
    Highlight(runtime.ReadWord(previous), false, runtime);
    runtime.WriteWord(member + 28u, record);
    Highlight(runtime.ReadWord(record), true, runtime);
}

void SwitchSlider(const std::uint32_t member,
                  const std::uint32_t next,
                  ISystemSettingNavigationRuntime& runtime) {
    HighlightSlider(runtime.ReadWord(member + 24u), false, runtime);
    runtime.WriteWord(member + 24u, next);
    HighlightSlider(next, true, runtime);
}

}  // namespace

std::uint8_t SystemSettingNavigation::HandleKey(
    const std::uint32_t member, const std::uint32_t event,
    ISystemSettingNavigationRuntime& runtime) {
    const auto code = runtime.ReadWord(event + 16u);
    if (code == 1u) {
        runtime.RemoveActive(runtime.FrameManager() + 308u, member);
        runtime.ShowSystemFrame(runtime.ReadWord(member + 4u));
        return 1u;
    }

    if (code == 200u || code == 208u) {
        const auto frame = runtime.ReadWord(member + 4u);
        const auto current_slider = runtime.ReadWord(member + 24u);
        const auto first_slider = runtime.ReadWord(frame + 496u);
        if (code == 200u) {
            if (current_slider == first_slider) {
                return 1u;
            }
            const auto second_slider = runtime.ReadWord(frame + 500u);
            if (current_slider == second_slider) {
                SwitchSlider(member, first_slider, runtime);
                return 1u;
            }
            const auto current_record = runtime.ReadWord(member + 28u);
            if (current_record == 0u) {
                return 1u;
            }
            const auto next = NextEnabled(current_record, 12u, runtime);
            if (next != 0u) {
                SelectRecord(member, next, runtime);
            } else {
                Highlight(runtime.ReadWord(current_record), false, runtime);
                runtime.WriteWord(member + 28u, 0u);
                runtime.WriteWord(member + 24u, second_slider);
                HighlightSlider(second_slider, true, runtime);
            }
            return 1u;
        }
        if (current_slider == first_slider) {
            SwitchSlider(member, runtime.ReadWord(frame + 500u), runtime);
            return 1u;
        }
        const auto second_slider = runtime.ReadWord(frame + 500u);
        if (current_slider == second_slider) {
            HighlightSlider(second_slider, false, runtime);
            runtime.WriteWord(member + 24u, 0u);
            const auto begin = runtime.ReadWord(member + 12u);
            const auto first_record = runtime.ReadWord(begin);
            runtime.WriteWord(member + 28u, first_record);
            if (first_record == 0u) {
                return 1u;
            }
            const auto next = runtime.ReadByte(
                runtime.ReadWord(first_record) + 1202u) == 0u ?
                first_record : NextEnabled(first_record, 8u, runtime);
            if (next != 0u) {
                runtime.WriteWord(member + 28u, next);
                Highlight(runtime.ReadWord(next), true, runtime);
            }
            return 1u;
        }
        const auto current_record = runtime.ReadWord(member + 28u);
        if (current_record == 0u) {
            return 1u;
        }
        const auto next = NextEnabled(current_record, 16u, runtime);
        if (next != 0u) {
            SelectRecord(member, next, runtime);
        }
        return 1u;
    }

    if (code == 203u || code == 205u) {
        const auto slider = runtime.ReadWord(member + 24u);
        if (slider != 0u) {
            const auto value = runtime.ReadFloat(slider + 1200u);
            runtime.SetSliderValue(slider,
                                   value + (code == 203u ? 0.1f : -0.1f));
            return 1u;
        }
        const auto current_record = runtime.ReadWord(member + 28u);
        const auto next = NextEnabled(current_record,
                                      code == 203u ? 4u : 8u, runtime);
        if (next != 0u) {
            SelectRecord(member, next, runtime);
        }
        return 1u;
    }

    if (code == 28u) {
        const auto record = runtime.ReadWord(member + 28u);
        if (record != 0u) {
            runtime.HandleMouseEvent(runtime.ReadWord(record));
        }
        return 1u;
    }
    return 0u;
}

std::uint8_t SystemSettingNavigation::ResetHighlights(
    const std::uint32_t member,
    ISystemSettingNavigationRuntime& runtime) {
    const auto begin = runtime.ReadWord(member + 12u);
    const auto end = runtime.ReadWord(member + 16u);
    for (auto current = begin; current != end; current += 4u) {
        const auto record = runtime.ReadWord(current);
        Highlight(runtime.ReadWord(record), false, runtime);
    }
    const auto slider = runtime.ReadWord(member + 24u);
    if (slider != 0u) {
        HighlightSlider(slider, true, runtime);
    } else {
        const auto record = runtime.ReadWord(member + 28u);
        if (record != 0u) {
            Highlight(runtime.ReadWord(record), true, runtime);
        }
    }
    return 1u;
}

}  // namespace pal4
