#include "pal4/scenario_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint32_t ScenarioDialog::Layout(
    const std::uint32_t object, const std::uint8_t side,
    IScenarioLayoutRuntime& runtime) {
    const auto editbox = runtime.ReadWord(object + 56u);
    const auto font_extent = runtime.FontExtentAA(editbox);
    const auto portrait = runtime.ReadWord(object + 44u);
    const auto image = runtime.ReadWord(portrait + 0x6b0u);
    const auto scenario = runtime.ReadWord(object + 52u);
    const auto click = runtime.ReadWord(object + 48u);
    if (image != 0u) {
        runtime.SetVisible(portrait, true);
        if (side != 0u) {
            (void)runtime.SetRect(editbox,
                {206.0f, 12.0f, font_extent + 717.0f, 115.0f});
            runtime.WriteByte(scenario + 0x6b4u, 0u);
            runtime.RequestRedraw(scenario);
            const auto height = runtime.ReadFloat(image + 32u);
            (void)runtime.SetRect(portrait,
                {0.0f, 600.0f - height, 199.0f, 600.0f});
            runtime.WriteByte(portrait + 0x6b4u, 0u);
            return runtime.SetRect(click,
                {746.0f, 95.0f, 772.0f, 118.0f});
        }
        (void)runtime.SetRect(editbox,
            {78.0f, 12.0f, font_extent + 587.0f, 115.0f});
        runtime.WriteByte(scenario + 0x6b4u, 1u);
        runtime.RequestRedraw(scenario);
        const auto height = runtime.ReadFloat(image + 32u);
        (void)runtime.SetRect(portrait,
            {601.0f, 600.0f - height, 800.0f, 600.0f});
        runtime.WriteByte(portrait + 0x6b4u, 1u);
        return runtime.SetRect(click,
            {575.0f, 95.0f, 601.0f, 118.0f});
    }
    runtime.SetVisible(portrait, false);
    (void)runtime.SetRect(editbox,
        {30.0f, 12.0f, font_extent + 717.0f, 115.0f});
    runtime.WriteByte(scenario + 0x6b4u, 0u);
    runtime.RequestRedraw(scenario);
    return runtime.SetRect(click,
        {746.0f, 95.0f, 772.0f, 118.0f});
}

}  // namespace pal4
