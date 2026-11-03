#include "pal4/effect_random.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t EffectRandom::Construct(
    std::uint32_t object, std::uint32_t seed,
    IEffectRandomRuntime& runtime) {
    runtime.WriteWord(object, kBaseVtable);
    if (runtime.Singleton() != 0u)
        throw std::logic_error("effect random singleton already exists");
    runtime.SetSingleton(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 4u,
                      seed == 0u ? runtime.CurrentTime() : seed);
    return object;
}

std::uint32_t EffectRandom::Destruct(
    std::uint32_t object, IEffectRandomRuntime& runtime) {
    runtime.WriteWord(object, kBaseVtable);
    const auto current = runtime.Singleton();
    if (current == 0u)
        throw std::logic_error("effect random singleton is absent");
    runtime.SetSingleton(0u);
    return current;
}

std::uint32_t EffectRandom::Delete(
    std::uint32_t object, std::uint8_t flags,
    IEffectRandomRuntime& runtime) {
    (void)Destruct(object, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(object);
    return object;
}

std::uint32_t EffectRandom::NextBounded(
    std::uint32_t object, std::uint32_t bound,
    IEffectRandomRuntime& runtime) {
    const auto next = 1194211693u * runtime.ReadWord(object + 4u) +
                      12345u;
    runtime.WriteWord(object + 4u, next);
    return next % (bound == 0u ? 1u : bound);
}

double EffectRandom::NextUnit(
    std::uint32_t object, IEffectRandomRuntime& runtime) {
    return static_cast<double>(NextBounded(object, 0x10000u, runtime)) *
           (1.0 / 65536.0);
}

double EffectRandom::NextRange(
    std::uint32_t object, float minimum, float maximum,
    IEffectRandomRuntime& runtime) {
    return static_cast<double>(minimum) +
           NextUnit(object, runtime) *
               (static_cast<double>(maximum) -
                static_cast<double>(minimum));
}

}  // namespace pal4
