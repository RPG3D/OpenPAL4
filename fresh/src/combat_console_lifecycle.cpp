#include "pal4/combat_console_lifecycle.h"

namespace pal4 {

std::uint32_t CombatConsoleLifecycle::Initialize(
    ICombatConsoleRuntime& runtime) {
    const auto guard = runtime.ReadByte(kSingletonGuard);
    if ((guard & 1u) != 0u) return runtime.ReadWord(kSingletonSlot);
    runtime.WriteByte(kSingletonGuard,
                      static_cast<std::uint8_t>(guard | 1u));
    const auto result = runtime.RegisterSingleton(
        runtime.SingletonManager(), "cbCombatConsole", kSingletonId,
        kFactory, kDestroyCallback);
    runtime.WriteWord(kSingletonSlot, result);
    return result;
}

std::uint32_t CombatConsoleLifecycle::CreateInstance(
    ICombatConsoleRuntime& runtime) {
    const auto console = runtime.Allocate(kObjectSize);
    if (console == 0u) return 0u;
    try {
        return Construct(console, runtime);
    } catch (...) {
        (void)runtime.Free(console);
        throw;
    }
}

std::uint32_t CombatConsoleLifecycle::DestroyInstance(
    const std::uint32_t console, ICombatConsoleRuntime& runtime) {
    if (console == 0u) return 0u;
    (void)DestroyQueueThunk(console, runtime);
    return runtime.Free(console);
}

std::uint32_t CombatConsoleLifecycle::Construct(
    const std::uint32_t console, ICombatConsoleRuntime& runtime) {
    return Construct(console, static_cast<IAnimationEventQueueRuntime&>(runtime));
}

std::uint32_t CombatConsoleLifecycle::Construct(
    const std::uint32_t console, IAnimationEventQueueRuntime& runtime) {
    (void)AnimationEventQueue::Construct(console, 0u, runtime);
    for (std::uint32_t index = 0u;
         index < CombatConsoleEventSubscriptions::kBindingCount; ++index) {
        runtime.WriteWord(console + 68u + 8u * index, 0u);
        runtime.WriteWord(console + 72u + 8u * index, 0u);
    }
    return console;
}

std::uint32_t CombatConsoleLifecycle::DestroyQueueThunk(
    const std::uint32_t console, ICombatConsoleRuntime& runtime) {
    return AnimationEventQueue::Destroy(console, runtime);
}

std::uint32_t CombatConsoleLifecycle::SubscribeThunk(
    const std::uint32_t console, ICombatConsoleRuntime& runtime) {
    return CombatConsoleEventSubscriptions::Subscribe(console, runtime);
}

std::uint32_t CombatConsoleLifecycle::Destroy(
    const std::uint32_t console, ICombatConsoleRuntime& runtime) {
    CombatConsoleEventSubscriptions::Unsubscribe(console, runtime);
    return AnimationEventQueue::Clear(console, runtime);
}

}  // namespace pal4
