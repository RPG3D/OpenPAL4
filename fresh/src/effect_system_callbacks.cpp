#include "pal4/effect_system_callbacks.h"

#include <stdexcept>

namespace pal4 {
namespace {

bool WithinSquaredRadius(EffectPosition first, EffectPosition second,
                         float squared_radius) {
    const auto dx = static_cast<long double>(first.x) - second.x;
    const auto dy = static_cast<long double>(first.y) - second.y;
    const auto dz = static_cast<long double>(first.z) - second.z;
    const auto squared = dz * dz + dy * dy + dx * dx;
    return squared <= static_cast<long double>(squared_radius);
}

std::uint32_t RequireAuxiliary(IEffectSystemCallbackRuntime& runtime) {
    const auto auxiliary = runtime.AuxiliarySingleton();
    if (auxiliary == 0u)
        throw std::logic_error("effect-system auxiliary singleton is absent");
    return auxiliary;
}

}  // namespace

bool EffectSystemCallbacks::ValidateObjectTransform(
    std::uint32_t gate, std::uint32_t object,
    EffectPosition target, IEffectProximityRuntime& runtime) {
    if (runtime.ReadByte(gate + 4u) == 0u)
        return true;
    const auto source = runtime.ObjectPosition(runtime.ReadWord(object + 4u));
    return WithinSquaredRadius(source, target,
                               runtime.ReadFloat(gate + 8u));
}

std::uint32_t EffectSystemCallbacks::ProcessEffectObject(
    std::uint32_t object, IEffectSystemCallbackRuntime& runtime) {
    const auto auxiliary = RequireAuxiliary(runtime);
    const auto candidate = runtime.FindElement7(auxiliary + 28u, object);
    const auto head = runtime.ReadWord(auxiliary + 32u);
    if (candidate == head || object < runtime.ReadWord(candidate + 12u))
        return object;
    const auto source = runtime.ObjectPosition(runtime.ReadWord(object + 4u));
    const auto camera = runtime.CurrentCamera();
    const auto target = runtime.ObjectPosition(runtime.ReadWord(camera + 4u));
    if (WithinSquaredRadius(source, target,
                            runtime.ReadFloat(auxiliary + 8u)))
        runtime.InvokeEffect(candidate, object);
    return object;
}

std::uint32_t EffectSystemCallbacks::ProcessAtomic(
    std::uint32_t atomic, std::uint8_t enabled,
    IEffectSystemCallbackRuntime& runtime) {
    const auto auxiliary = RequireAuxiliary(runtime);
    if (enabled != 0u)
        runtime.AddEffect(auxiliary, atomic);
    else
        runtime.RemoveEffect(auxiliary, atomic);
    return atomic;
}

std::uint32_t EffectSystemCallbacks::ProcessAllAtomics(
    std::uint32_t clump, std::uint8_t enabled,
    IEffectSystemCallbackRuntime& runtime) {
    return runtime.ForAllAtomics(
        clump, enabled,
        [&runtime](std::uint32_t atomic, std::uint8_t flag) {
            return ProcessAtomic(atomic, flag, runtime);
        });
}

}  // namespace pal4
