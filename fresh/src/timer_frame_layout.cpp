#include "pal4/timer_frame_layout.h"

#include <bit>

namespace pal4 {
namespace {

float AsFloat(const std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

std::uint32_t AsBits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

}  // namespace

std::uint32_t TimerFrameLayout::Apply(
    const std::uint32_t frame, ITimerFrameLayoutRuntime& runtime) {
    runtime.WriteWord(frame + 0x34u,
                      runtime.ReadWord(frame + 0x30u));
    const auto expanded = runtime.ReadWord(frame + 0x74u) != 0u;
    const auto settle = runtime.ReadWord(frame + 0x58u);
    const auto delegate = runtime.ReadWord(frame + 0x5cu);
    const auto exit = runtime.ReadWord(frame + 0x60u);
    const auto bottom = runtime.ReadWord(frame + 0x64u);
    const auto panel = runtime.ReadWord(frame + 0x68u);
    const auto delegate_x = runtime.ReadWord(frame + 0x48u);
    const auto delegate_y = runtime.ReadWord(frame + 0x4cu);
    const auto exit_x = runtime.ReadWord(frame + 0x50u);
    const auto exit_y = runtime.ReadWord(frame + 0x54u);
    const auto saved_panel_height = runtime.ReadWord(frame + 0x6cu);

    runtime.SetVisible(delegate, expanded);
    if (expanded) {
        runtime.SetPosition(delegate, delegate_x, delegate_y);
        runtime.SetPosition(exit, exit_x, exit_y);
        const auto bottom_y = static_cast<float>(
            static_cast<double>(AsFloat(exit_y)) +
            static_cast<double>(AsFloat(runtime.HeightBits(exit))));
        runtime.SetPosition(bottom, exit_x, AsBits(bottom_y));
        runtime.SetHeight(panel, saved_panel_height);
        (void)runtime.SetupMouseHandler(frame + 0x28u, settle);
        (void)runtime.SetupMouseHandler(frame + 0x28u, delegate);
        return runtime.SetupMouseHandler(frame + 0x28u, exit);
    }

    runtime.SetPosition(exit, delegate_x, delegate_y);
    runtime.SetPosition(bottom, exit_x, exit_y);
    const auto reduced_height = static_cast<float>(
        static_cast<double>(AsFloat(saved_panel_height)) -
        static_cast<double>(AsFloat(runtime.HeightBits(exit))));
    runtime.SetHeight(panel, AsBits(reduced_height));
    (void)runtime.SetupMouseHandler(frame + 0x28u, settle);
    return runtime.SetupMouseHandler(frame + 0x28u, exit);
}

std::uint32_t TimerFrameLayout::SetWindowVisibility(
    const std::uint32_t frame, const std::uint8_t visible,
    const std::uint32_t parent, ITimerFrameLayoutRuntime& runtime) {
    runtime.SetParent(frame, visible, parent);
    if (visible != 0u) {
        runtime.WriteWord(frame + 0x70u, 0u);
        runtime.ResetMember(frame + 0x28u);
    }
    runtime.SetModalState(runtime.ReadWord(frame + 0x24u), visible);
    return Apply(frame, runtime);
}

}  // namespace pal4
