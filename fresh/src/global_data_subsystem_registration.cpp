#include "pal4/global_data_subsystem_registration.h"

namespace pal4 {

std::uint32_t GlobalDataSubsystemRegistration::Initialize(
    std::uint8_t& guard, std::uint32_t& cached_instance,
    IGlobalDataSubsystemRuntime& runtime) {
    if ((guard & 1u) != 0u) return cached_instance;
    guard = static_cast<std::uint8_t>(guard | 1u);
    cached_instance = runtime.RegisterSingleton(
        kNameAddress, kSingletonId, kFactoryAddress, kCleanupAddress);
    return cached_instance;
}

std::uint32_t GlobalDataSubsystemRegistration::Create(
    IGlobalDataSubsystemRuntime& runtime) {
    const auto object = runtime.AllocateMemory(kObjectSize);
    if (object == 0u) return 0u;
    try {
        return runtime.Construct(object);
    } catch (...) {
        runtime.FreeMemory(object);
        throw;
    }
}

void GlobalDataSubsystemRegistration::Destroy(
    const std::uint32_t object, IGlobalDataSubsystemRuntime& runtime) {
    if (object == 0u) return;
    runtime.Cleanup(object);
    runtime.FreeMemory(object);
}

}  // namespace pal4
