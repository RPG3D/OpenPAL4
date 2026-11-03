#include "pal4/scenario_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint8_t ScenarioDialog::HandleEditboxMouseUp(
    const std::uint32_t object, const std::uint32_t event,
    IScenarioInputEventRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 0u)
        runtime.HandleTextDisplay(object);
    return 1u;
}

std::uint8_t ScenarioDialog::HandleEditboxKey(
    const std::uint32_t context, const std::uint32_t event,
    IScenarioInputEventRuntime& runtime) {
    if (runtime.ReadWord(event + 16u) == 28u)
        runtime.HandleTextDisplay(context - 40u);
    return 1u;
}

std::uint8_t ScenarioDialog::AlwaysTrue(const std::uint32_t) {
    return 1u;
}

std::uint32_t ScenarioDialog::RegisterEditboxEvent(
    const std::uint32_t object, IScenarioEventRuntime& runtime) {
    const auto callback = runtime.Allocate(24u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, 0x4b7fb0u);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, object);
        runtime.WriteWord(callback, 0x844334u);
    }
    const auto root = runtime.ReadWord(object + 36u);
    const auto owner = runtime.SubscribeMouseUp(root, callback);
    if (owner == 0u) return 0u;
    const auto count = runtime.ReadWord(owner + 4u) - 1u;
    runtime.WriteWord(owner + 4u, count);
    if (count == 0u) return runtime.DestroyOwner(owner, 1u);
    return owner + 4u;
}

}  // namespace pal4
