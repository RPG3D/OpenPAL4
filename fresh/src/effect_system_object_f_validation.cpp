#include "pal4/effect_system_object_f_validation.h"

namespace pal4 {

std::uint8_t EffectSystemObjectFValidation::Check(
    const EffectSystemObjectFState& state,
    const std::uint32_t register_ebp, const std::uint32_t register_edi,
    const std::uint32_t source,
    IEffectSystemObjectFValidationRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto enabled = static_cast<std::uint8_t>(state.words[1u] >> 24u);
    if (enabled == 0u)
        return enabled;

    const auto context = state.words[3u];
    if (context != 0u && runtime.ReadByte(context + 83u) != 0u) {
        const auto handle = runtime.ReadWord(state.words[29u] + 20u);
        const auto transform = runtime.GetObjectTransform(handle) + 48u;
        const auto validator = runtime.EffectSystemValidator();
        if (validator == 0u)
            runtime.ThrowMissingValidator();
        if (runtime.ValidateTransform(validator, source, transform) == 0u)
            return 0u;
    }
    return runtime.CheckObject(state.words[29u], register_ebp,
                               register_edi);
}

}  // namespace pal4
