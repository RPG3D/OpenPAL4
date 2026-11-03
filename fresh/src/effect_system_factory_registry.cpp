#include "pal4/effect_system_factory_registry.h"

namespace pal4 {

std::uint8_t EffectSystemFactoryRegistry::InitializeEvents(
    IEffectSystemFactoryRuntime& runtime) {
    const auto system = runtime.RegisterEffectSystem();
    const auto tree = system + 24u;
    if (runtime.ReadWord(system + 36u) != 0u)
        runtime.ClearRegistry(tree);

    std::uint8_t result = 0u;
    for (const auto& factory : kFactories) {
        if (factory.via_event_helper) {
            (void)ProcessEvent(tree, factory.name, factory.creator,
                               runtime);
        } else if (!runtime.HasFactory(tree, factory.name)) {
            runtime.InsertFactory(tree, factory.name, factory.creator);
        }
        result = runtime.ReleaseName(factory.name);
    }
    return result;
}

std::uint8_t EffectSystemFactoryRegistry::ProcessEvent(
    std::uint32_t tree, std::string_view name,
    std::uint32_t creator,
    IEffectSystemFactoryRuntime& runtime) {
    if (runtime.HasFactory(tree, name))
        return 0u;
    runtime.InsertFactory(tree, name, creator);
    return 1u;
}

std::uint32_t EffectSystemFactoryRegistry::Create(
    EffectFactoryKind kind,
    IEffectSystemFactoryCreationRuntime& runtime) {
    const auto bytes = kFactories.at(static_cast<std::size_t>(kind)).bytes;
    const auto object = runtime.Allocate(bytes);
    return object == 0u ? 0u : runtime.Construct(kind, object);
}

}  // namespace pal4
