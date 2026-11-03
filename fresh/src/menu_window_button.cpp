#include "pal4/menu_window_button.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

void MenuWindowButton::Create(
    const std::uint32_t menu, const std::uint32_t record,
    const bool add_to_navigation, const bool enabled,
    IMenuWindowButtonRuntime& runtime) {
    const auto begin = runtime.ReadWord(menu + 68u);
    const auto insertion = runtime.ReadWord(menu + 72u);
    const auto index = begin == 0u
        ? 0 : static_cast<std::int32_t>(insertion - begin) / 164;
    runtime.AppendButtonRecord(menu + 64u, insertion, 1u, record);

    const auto name = "popItem_" + std::to_string(index);
    const auto button = runtime.CreateWindow("OiramLook/Button", name);
    const auto model = runtime.ReadWord(menu + 84u);
    runtime.SetText(button, record + 12u);
    constexpr MenuButtonColour black{0.0f, 0.0f, 0.0f, 1.0f};
    constexpr MenuButtonColour disabled{0.30000001f, 0.30000001f,
                                        0.30000001f, 1.0f};
    runtime.SetTextColour(button, MenuButtonVisualState::Normal, black);
    runtime.SetTextColour(button, MenuButtonVisualState::Highlight, black);
    runtime.SetTextColour(button, MenuButtonVisualState::Pushed, black);
    runtime.SetTextColour(button, MenuButtonVisualState::Disabled, disabled);
    for (const auto state : std::array{
             MenuButtonVisualState::Normal,
             MenuButtonVisualState::Highlight,
             MenuButtonVisualState::Disabled,
             MenuButtonVisualState::Pushed}) {
        runtime.CopyImage(model, button, state, "Self");
    }

    runtime.AddChild(runtime.ReadWord(menu + 80u), button);
    constexpr std::uint32_t mode = 1u;
    const auto y = runtime.GetMetric(model, MenuButtonMetric::Y, mode);
    const auto height = runtime.GetMetric(
        model, MenuButtonMetric::Height, mode);
    runtime.SetMetric(button, MenuButtonMetric::Y, mode,
                      y + height * static_cast<float>(index));
    runtime.SetMetric(button, MenuButtonMetric::X, mode,
                      runtime.GetMetric(model, MenuButtonMetric::X, mode));
    runtime.SetMetric(button, MenuButtonMetric::Width, mode,
                      runtime.GetMetric(model, MenuButtonMetric::Width,
                                        mode));
    runtime.SetMetric(button, MenuButtonMetric::Height, mode,
                      runtime.GetMetric(model, MenuButtonMetric::Height,
                                        mode));
    runtime.SetClippedByParent(button, false);
    runtime.SetEnabled(button, enabled);
    runtime.WriteWord(button + 280u,
                      static_cast<std::uint32_t>(index));

    const auto storage = runtime.Allocate(24u);
    const auto callback = storage == 0u
        ? 0u : runtime.ConstructClickCallback(
                   storage, 0x4b4b40u, 0u, menu);
    runtime.SubscribeClicked(button, callback);
    if (callback != 0u) runtime.ReleaseClickCallback(callback);
    if (add_to_navigation) {
        runtime.AppendNavigationButton(
            menu + 44u, runtime.ReadWord(menu + 52u), button);
    }
}

}  // namespace pal4
