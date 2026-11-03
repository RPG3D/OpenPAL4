#include "pal4/system_setting_options.h"

namespace pal4 {
namespace {

std::uint32_t Select(const std::uint32_t frame,
                     const std::uint32_t event,
                     const std::uint32_t offset,
                     const std::uint32_t count,
                     ISystemSettingOptionActionsRuntime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index) {
        runtime.SetSelected(runtime.ReadWord(frame + offset + 4u * index),
                            false);
    }
    const auto source = runtime.ReadWord(event + 8u);
    runtime.SetSelected(source, true);
    return runtime.ReadWord(source + 280u);
}

}  // namespace

std::uint8_t SystemSettingOptions::SelectSequence(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingOptionActionsRuntime& runtime) {
    const auto index = Select(frame, event, 200u, 3u, runtime);
    runtime.WriteWord(runtime.GlobalData() + 4u, index);
    return 1u;
}

std::uint8_t SystemSettingOptions::SelectClipDistance(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingOptionActionsRuntime& runtime) {
    const auto index = Select(frame, event, 212u, 2u, runtime);
    runtime.ApplyClipDistance(runtime.GlobalData(), index);
    return 1u;
}

std::uint8_t SystemSettingOptions::SelectAuto(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingOptionActionsRuntime& runtime) {
    const auto index = Select(frame, event, 220u, 3u, runtime);
    runtime.WriteWord(runtime.GlobalData(), index);
    return 1u;
}

std::uint8_t SystemSettingOptions::SelectIntro(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingOptionActionsRuntime& runtime) {
    const auto index = Select(frame, event, 232u, 2u, runtime);
    runtime.WriteWord(runtime.GlobalData() + 8u, index == 0u ? 1u : 0u);
    return 1u;
}

}  // namespace pal4
