#include "pal4/effect_system_task_variants.h"

namespace pal4 {

std::uint32_t EffectSystemTaskVariants::CleanupMessage(
    std::uint32_t object, IEffectSystemTaskVariantRuntime& runtime) {
    runtime.WriteWord(object, kMessageVtable);
    runtime.ResetMessageString(object);
    const auto text = runtime.ReadWord(object + 76u);
    if (text != 0u) {
        const auto header = text - 1u;
        const auto count = runtime.ReadByte(header);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(header);
        else
            runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(object + 76u, 0u);
    runtime.WriteWord(object + 80u, 0u);
    runtime.WriteWord(object + 84u, 0u);
    return runtime.CleanupCommon(object);
}

std::uint32_t EffectSystemTaskVariants::CleanupSlowMotion(
    std::uint32_t object, IEffectSystemTaskVariantRuntime& runtime) {
    runtime.WriteWord(object, kSlowMotionVtable);
    runtime.ResetSlowMotion(object);
    return runtime.CleanupCommon(object);
}

}  // namespace pal4
