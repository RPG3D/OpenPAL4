#include "pal4/ef_effect_system_lifecycle.h"

namespace pal4 {

std::uint32_t EfEffectSystemLifecycle::Register(
    std::uint8_t& guard, std::uint32_t& registered,
    IEfEffectSystemLifecycleRuntime& runtime) {
    if ((guard & 1u) != 0u) return registered;
    guard = static_cast<std::uint8_t>(guard | 1u);
    const auto manager = runtime.SingletonManager();
    registered = runtime.RegisterSingleton(
        manager, kSingletonName, kSingletonId,
        kFactoryAddress, kDestroyAddress);
    return registered;
}

std::uint32_t EfEffectSystemLifecycle::Create(
    IEfEffectSystemLifecycleRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    return object != 0u ? runtime.Construct(object) : 0u;
}

std::uint32_t EfEffectSystemLifecycle::Destroy(
    std::uint32_t object, IEfEffectSystemLifecycleRuntime& runtime) {
    if (object == 0u) return 0u;
    runtime.Destruct(object);
    return runtime.Free(object);
}

std::uint32_t EfEffectSystemLifecycle::ProcessScript(
    std::uint32_t owner, IEfEffectSystemLifecycleRuntime& runtime) {
    runtime.ProcessScript();
    return owner;
}

int EfEffectSystemLifecycle::CleanupScripts(
    IEfEffectSystemLifecycleRuntime& runtime) {
    return runtime.CleanupScripts();
}

}  // namespace pal4
