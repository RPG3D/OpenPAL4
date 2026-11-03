#include "pal4/effect_system_task_factory_registry.h"

namespace pal4 {

std::uint8_t EffectSystemTaskFactoryRegistry::Register(
    IEffectSystemTaskFactoryRegistryRuntime& runtime) {
    const auto system = runtime.RegisterEffectSystem();
    const auto tree = system + 8u;
    if (runtime.ReadWord(system + 20u) != 0u)
        runtime.ClearRegistry(tree);

    std::uint8_t result = 0u;
    for (const auto& factory : kFactories) {
        if (factory.via_event_helper) {
            runtime.ProcessEvent(tree, factory.name, factory.creator);
        } else if (!runtime.HasFactory(tree, factory.name)) {
            runtime.InsertFactory(tree, factory.name, factory.creator);
        }
        result = runtime.ReleaseName(factory.name);
    }
    return result;
}

}  // namespace pal4
