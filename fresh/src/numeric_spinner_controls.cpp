#include "pal4/numeric_spinner.h"

#include <string>
#include <utility>

namespace pal4 {

void NumericSpinner::CreateControls(
    const std::uint32_t item, INumericSpinnerControlsRuntime& runtime) {
    const std::string name(runtime.ReadName(item + 444u));
    const auto root = runtime.CreateWindow("DefaultWindow", name + "Frame");
    runtime.WriteWord(item + 436u, root);

    const auto left = runtime.CreateWindow(
        "OiramLook/Button", name + "ButtonLeft");
    runtime.WriteWord(item + 424u, left);
    runtime.SetStandardImagery(left, false);
    for (const auto [state, image] : {
             std::pair{SpinnerImageState::Normal, "sanjiaozuo3"},
             std::pair{SpinnerImageState::Pushed, "sanjiaozuo2"},
             std::pair{SpinnerImageState::Disabled, "sanjiaozuo2"},
             std::pair{SpinnerImageState::Hover, "sanjiaozuo1"}}) {
        const auto found = runtime.FindImage("gy0", image);
        runtime.SetButtonImage(left, state, found.handle);
        if (state == SpinnerImageState::Hover) {
            runtime.SetHeight(left, found.height);
            runtime.SetWidth(left, found.width);
        }
    }

    const auto right = runtime.CreateWindow(
        "OiramLook/Button", name + "ButtonRight");
    runtime.WriteWord(item + 428u, right);
    runtime.SetStandardImagery(right, false);
    SpinnerImage right_hover{};
    for (const auto [state, image] : {
             std::pair{SpinnerImageState::Normal, "sanjiaoyou3"},
             std::pair{SpinnerImageState::Pushed, "sanjiaoyou2"},
             std::pair{SpinnerImageState::Disabled, "sanjiaoyou2"},
             std::pair{SpinnerImageState::Hover, "sanjiaoyou1"}}) {
        const auto found = runtime.FindImage("gy0", image);
        runtime.SetButtonImage(right, state, found.handle);
        if (state == SpinnerImageState::Hover) {
            right_hover = found;
            runtime.SetHeight(right, found.height);
            runtime.SetWidth(right, found.width);
        }
    }

    const auto text = runtime.CreateWindow(
        "OiramLook/StaticText", name + "StaticText");
    runtime.WriteWord(item + 432u, text);
    runtime.SetBlackText(text);
    runtime.SetHeight(text, right_hover.height);
    runtime.SetWidth(text, 15.0f);
    runtime.SetFormatting(text, 2u, 1u);
    runtime.WriteFloat(item + 464u, right_hover.height + 2.0f);
    runtime.WriteFloat(item + 460u, right_hover.width * 2.0f + 15.0f);

    runtime.Subscribe(left, SpinnerMouseEvent::Clicked,
                      SpinnerCallback::LeftClick, item);
    runtime.Subscribe(right, SpinnerMouseEvent::Clicked,
                      SpinnerCallback::RightClick, item);
    runtime.Subscribe(left, SpinnerMouseEvent::ButtonDown,
                      SpinnerCallback::LeftDown, item);
    runtime.Subscribe(right, SpinnerMouseEvent::ButtonDown,
                      SpinnerCallback::RightDown, item);
    runtime.Subscribe(left, SpinnerMouseEvent::ButtonUp,
                      SpinnerCallback::ButtonUp, item);
    runtime.Subscribe(right, SpinnerMouseEvent::ButtonUp,
                      SpinnerCallback::ButtonUp, item);

    runtime.AddChild(root, right);
    runtime.AddChild(root, left);
    runtime.AddChild(root, text);
    runtime.UpdateValue(item,
                        static_cast<std::int32_t>(
                            runtime.ReadWord(item + 440u)));
}

}  // namespace pal4
