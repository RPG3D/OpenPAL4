#include "pal4/strip_system_effect.h"

namespace pal4 {

std::uint32_t StripSystemEffect::Create(
    std::uint32_t config, std::uint32_t clump,
    std::uint32_t context, IStripSystemFactoryRuntime& runtime) {
    if (clump == 0u) return 0u;
    const auto effect = runtime.Allocate(kObjectSize);
    if (effect == 0u) return 0u;
    runtime.WriteWord(effect, kBaseVtable);
    runtime.WriteByte(effect + 4u, 0u);
    runtime.WriteByte(effect + 5u, 0u);
    runtime.WriteByte(effect + 6u, 0u);
    for (std::uint32_t index = 0u; index < 2u; ++index) {
        if (StripResourceBlock::Construct(
                effect + 8u + index * StripResourceBlock::kSize,
                runtime) == 0u) {
            runtime.Destroy(effect, true);
            return 0u;
        }
    }
    runtime.WriteWord(effect, kVtable);
    if (runtime.Initialize(effect, config, clump, context))
        return effect;
    runtime.Destroy(effect, true);
    return 0u;
}

double StripSystemEffect::GetTime(
    std::uint32_t effect, IStripSystemTimeRuntime& runtime) {
    const auto time = runtime.ReadFloat(effect + 68u);
    runtime.WriteByte(effect + 5u, 1u);
    return time;
}

}  // namespace pal4
