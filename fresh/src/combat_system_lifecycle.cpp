#include "pal4/combat_system_lifecycle.h"

namespace pal4 {

std::uint32_t CombatSystemSingleton::GetInstance(
    ICombatSystemSingletonRuntime& runtime) {
    if ((state_.initialization_bits & 1u) != 0u) return state_.instance;
    state_.initialization_bits |= 1u;
    const auto manager = runtime.GetSingletonManager();
    state_.instance = runtime.Register(manager, "cbCombatSystem", 1028u,
                                       kFactoryAddress, kDestructorAddress);
    return state_.instance;
}

std::uint32_t CombatSystemLifecycle::Create(
    ICombatSystemLifecycleRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    return object == 0u ? 0u : runtime.Construct(object);
}

std::uint32_t CombatSystemLifecycle::Destroy(
    const std::uint32_t object, ICombatSystemLifecycleRuntime& runtime) {
    if (object == 0u) return 0u;
    runtime.Destruct(object);
    return runtime.Free(object);
}

}  // namespace pal4
