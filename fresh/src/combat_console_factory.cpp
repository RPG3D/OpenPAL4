#include "pal4/combat_console_factory.h"

namespace pal4 {

std::uint32_t CombatConsoleFactory::GetInstanceThunk(
    ICombatConsoleFactoryRuntime& runtime) {
    return GetInstance(runtime);
}

std::uint32_t CombatConsoleFactory::GetInstance(
    ICombatConsoleFactoryRuntime& runtime) {
    const auto instance = runtime.ReadWord(kFactorySource);
    runtime.WriteWord(kFactoryCache, instance);
    return instance;
}

std::uint32_t CombatConsoleFactory::Create(
    const std::string_view name, ICombatConsoleFactoryRuntime& runtime) {
    const auto storage = runtime.Allocate(0x13cu);
    return storage != 0u ? runtime.ConstructConsole(storage, name) : 0u;
}

std::uint32_t CombatConsoleFactory::RegisterThunk(
    ICombatConsoleFactoryRuntime& runtime) {
    return Register(runtime);
}

std::uint32_t CombatConsoleFactory::Register(
    ICombatConsoleFactoryRuntime& runtime) {
    (void)runtime.FrameManager();
    return runtime.InsertFactory(runtime.ReadWord(kFactoryCache),
                                 kCreateCallback);
}

std::uint32_t CombatConsoleFactory::RegisterEventThunk(
    ICombatConsoleFactoryRuntime& runtime) {
    return RegisterEvent(runtime);
}

std::uint32_t CombatConsoleFactory::RegisterEvent(
    ICombatConsoleFactoryRuntime& runtime) {
    const auto factory = runtime.ReadWord(kFactorySource);
    auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, factory);
        runtime.WriteWord(callback, kEventCallbackVtable);
    }
    const auto manager = runtime.FrameManager();
    runtime.InsertEvent(manager + 260u, kEventKey, callback);
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
    return kEventSentinel;
}

}  // namespace pal4
