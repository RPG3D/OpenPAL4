#include "pal4/strip_system_effect.h"

namespace pal4 {

std::uint8_t StripSystemEffect::CleanupObject(
    std::uint32_t effect, IStripSystemLifecycleRuntime& runtime) {
    if (runtime.ReadByte(effect + 4u) == 0u) return 0u;
    runtime.WriteByte(effect + 4u, 0u);
    runtime.WriteByte(effect + 6u, 0u);
    static_cast<void>(runtime.CleanupResources(effect + 8u));
    return runtime.CleanupResources(effect + 144u);
}

std::uint8_t StripSystemEffect::UpdateVisuals(
    std::uint32_t effect, double seconds,
    IStripSystemLifecycleRuntime& runtime) {
    const auto active = runtime.ReadByte(effect + 4u);
    if (active == 0u) return active;
    static_cast<void>(runtime.UpdateVisual(
        effect + 8u, seconds, runtime.ReadByte(effect + 5u)));
    const auto second = runtime.ReadByte(effect + 6u);
    if (second == 0u) return second;
    return runtime.UpdateVisual(
        effect + 144u, seconds, runtime.ReadByte(effect + 5u));
}

void StripSystemEffect::ManageVisuals(
    std::uint32_t effect, IStripSystemLifecycleRuntime& runtime) {
    if (runtime.ReadByte(effect + 4u) == 0u) return;
    runtime.ManageVisual(effect + 8u);
    if (runtime.ReadByte(effect + 6u) != 0u)
        runtime.ManageVisual(effect + 144u);
}

void StripSystemEffect::Destruct(
    std::uint32_t effect, IStripSystemLifecycleRuntime& runtime) {
    runtime.WriteWord(effect, kVtable);
    static_cast<void>(CleanupObject(effect, runtime));
    runtime.DestructResourceBlocks(effect + 8u, 0x88u, 2u);
    runtime.WriteWord(effect, kBaseVtable);
}

}  // namespace pal4
