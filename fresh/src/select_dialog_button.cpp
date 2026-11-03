#include "pal4/select_dialog.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t SelectDialog::CreateButton(
    const std::uint32_t object,
    ISelectDialogButtonRuntime& runtime) {
    const auto id = runtime.NextButtonId();
    const auto answer_template = runtime.ReadWord(object + 148u);
    const auto button = runtime.CreateWindow(
        "OiramLook/Button", "SelectDialogBtn_" +
                        std::to_string(static_cast<std::int32_t>(id)));
    constexpr std::array text_states{
        SelectButtonState::Normal, SelectButtonState::Hover,
        SelectButtonState::Pushed, SelectButtonState::Disabled};
    for (const auto state : text_states)
        runtime.CopyTextColour(button, answer_template, state);
    constexpr std::array image_states{
        SelectButtonState::Normal, SelectButtonState::Hover,
        SelectButtonState::Disabled, SelectButtonState::Pushed};
    for (const auto state : image_states) {
        const auto image = runtime.GetStateImage(answer_template,
                                                 state);
        runtime.SetStateImage(button, state, image);
    }

    const auto panel = runtime.CreateWindow(
        "OiramLook/StaticImage", "SelectDialogPanel_" +
                             std::to_string(
                                 static_cast<std::int32_t>(id)));
    const auto body = runtime.ReadWord(object + 140u);
    runtime.SetPanelImage(panel, runtime.ReadWord(body + 0x6b0u));
    runtime.AddChild(panel, button);
    constexpr std::uint32_t kRelativeMetrics = 1u;
    runtime.SetMetricPosition(
        button, kRelativeMetrics,
        runtime.GetMetricPosition(answer_template,
                                  kRelativeMetrics));
    runtime.SetMetricHeight(
        button, kRelativeMetrics,
        runtime.GetMetricHeight(answer_template,
                                kRelativeMetrics));
    runtime.SetMetricWidth(
        button, kRelativeMetrics,
        runtime.GetMetricWidth(answer_template,
                               kRelativeMetrics));

    const auto allocation = runtime.AllocateCallback(0x18u);
    if (allocation != 0u) {
        runtime.InitializeClickCallback(allocation, object,
                                         kClickCallback);
    }
    const auto subscription = runtime.SubscribeClicked(button, allocation);
    if (subscription != 0u) {
        const auto references = runtime.ReadWord(subscription + 4u) - 1u;
        runtime.WriteWord(subscription + 4u, references);
        if (references == 0u) runtime.DestroyWindowOwner(subscription, 1u);
    }
    return panel;
}

}  // namespace pal4
