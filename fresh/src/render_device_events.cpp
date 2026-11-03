#include "pal4/render_device_events.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kDeviceLost = 0x88760868u;
constexpr std::uint32_t kDeviceNotReset = 0x88760869u;
constexpr std::uint32_t kPresentLoss = 0x88760827u;

}  // namespace

std::uint32_t RenderDeviceEvents::HandleVideoModeChange(
    IRenderDeviceEventRuntime& runtime) const {
    const auto result = runtime.TestCooperativeLevel(state_.Read(0x560u));
    if (result >= 0) {
        return 1u;
    }
    if (static_cast<std::uint32_t>(result) == kDeviceLost) {
        if (runtime.HasLostCallback()) {
            (void)runtime.InvokeLostCallback();
        }
        return 0u;
    }
    if (static_cast<std::uint32_t>(result) == kDeviceNotReset) {
        if (runtime.HasLostCallback()) {
            (void)runtime.InvokeLostCallback();
        }
        runtime.ChangeVideoMode(state_.Read(0x538u));
        if (runtime.HasRestoredCallback()) {
            runtime.InvokeRestoredCallback();
        }
    }
    return 1u;
}

std::int32_t RenderDeviceEvents::HandleDeviceLoss(
    IRenderDeviceEventRuntime& runtime) const {
    const auto result =
        runtime.Present(state_.Read(0x560u), state_.Read(0u));
    const auto code = static_cast<std::uint32_t>(result);
    if (code != kDeviceLost && code != kPresentLoss) {
        return result;
    }
    if (!runtime.HasLostCallback()) {
        return 0;
    }
    return static_cast<std::int32_t>(runtime.InvokeLostCallback());
}

}  // namespace pal4
