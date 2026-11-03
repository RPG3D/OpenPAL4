#include "pal4/skill_system_lifecycle.h"
#include "pal4/skill_system_event_registry.h"

#include <array>

namespace pal4 {

std::uint32_t SkillSystemLifecycle::GetInstance(
    SkillSystemGlobals& globals, ISkillSystemLifecycleRuntime& runtime) {
    if ((globals.guard & 1u) != 0u) return globals.instance;
    globals.guard = static_cast<std::uint8_t>(globals.guard | 1u);
    const auto manager = runtime.SingletonManagerInstance();
    globals.instance = runtime.RegisterSingleton(
        manager, kSingletonName, kSingletonId, 0x58a050u, 0x58a0b0u);
    return globals.instance;
}

std::uint32_t SkillSystemLifecycle::Construct(
    ISkillSystemLifecycleRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    if (object == 0u) return 0u;
    return InitializeEventSystem(object, runtime);
}

std::uint32_t SkillSystemLifecycle::CleanupResource(
    std::uint32_t object, ISkillSystemLifecycleRuntime& runtime) {
    if (object == 0u) return 0u;
    CleanupEventData(object, runtime);
    return runtime.Free(object);
}

void SkillSystemLifecycle::CleanupEventData(
    std::uint32_t object, ISkillSystemLifecycleRuntime& runtime) {
    static_cast<void>(runtime.Free(runtime.ReadWord(object + 2348u)));
    for (const auto offset : std::array{2348u, 2352u, 2356u})
        runtime.WriteWord(object + offset, 0u);
    runtime.CleanupEventLogger(object);
}

std::uint32_t SkillSystemLifecycle::InitializeEventSystem(
    std::uint32_t object, ISkillSystemLifecycleRuntime& runtime) {
    runtime.ConstructEventLogger(object);
    runtime.WriteByte(object + 2344u, runtime.UninitializedHeaderByte());
    for (const auto offset : std::array{2348u, 2352u, 2356u})
        runtime.WriteWord(object + offset, 0u);
    SkillSystemEventRegistry::Initialize(object, runtime);
    return object;
}

}  // namespace pal4
