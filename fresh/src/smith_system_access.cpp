#include "pal4/smith_system_access.h"

namespace pal4 {

std::uint32_t SmithSystemAccess::Register(
    std::uint8_t& guard, std::uint32_t& cached,
    ISmithSystemRegistrationRuntime& runtime) {
    if ((guard & 1u) != 0u) return cached;
    guard = static_cast<std::uint8_t>(guard | 1u);
    cached = runtime.RegisterSingleton("palSmithSystem", 1054u,
                                       kFactory, kTypeInfo);
    return cached;
}

std::uint32_t SmithSystemAccess::Create(
    ISmithSystemRegistrationRuntime& runtime) {
    const auto memory = runtime.Allocate(1u);
    if (memory == 0u) return 0u;
    try {
        return runtime.Construct(memory);
    } catch (...) {
        runtime.Free(memory);
        throw;
    }
}

std::uint8_t SmithUiEvents::SetFlag(
    std::uint32_t system, std::uint8_t value,
    ISmithUiEventRuntime& runtime) {
    runtime.WriteByte(system, value);
    return runtime.DispatchEvent(runtime.UIFrameManager(),
                                 kFlagEvent, 1u, 0u);
}

std::uint8_t SmithUiEvents::ClearFlags(
    ISmithUiEventRuntime& runtime) {
    return runtime.DispatchEvent(runtime.UIFrameManager(),
                                 kFlagEvent, 0u, 0u);
}

std::uint8_t SmithUiEvents::SetState(
    std::uint32_t, ISmithUiEventRuntime& runtime) {
    return runtime.DispatchEvent(runtime.UIFrameManager(),
                                 kStateEvent, 1u, 0u);
}

}  // namespace pal4
