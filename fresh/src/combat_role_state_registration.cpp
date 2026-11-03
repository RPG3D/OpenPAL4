#include "pal4/combat_role_state_registration.h"

namespace pal4 {
namespace {

struct Binding {
    std::uint32_t key_global;
    std::uint32_t callback;
};

constexpr std::array<Binding, CombatRoleStateSubscriptions::kCount> kBindings{{
    {0x8b62e0u, 0x5518a0u},
    {0x8b6310u, 0x551be0u},
    {0x8b62e4u, 0x5518c0u},
    {0x8b62f0u, 0x5518e0u},
    {0x8b8b78u, 0x551ab0u},
    {0x8b5958u, 0x5519d0u},
    {0x8b599cu, 0x551a70u},
    {0x8b62f0u, 0x551a70u},
    {0x8b8b84u, 0x551ae0u},
}};

}  // namespace

std::uint32_t CombatRoleStateSubscriptions::Clear(
    const std::uint32_t window, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(window, 40u, kCount, runtime);
    return 0u;
}

std::uint32_t CombatRoleStateSubscriptions::Subscribe(
    const std::uint32_t window, IEventSubscriptionRuntime& runtime) {
    (void)Clear(window, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kCount; ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.ReadWord(binding.key_global);
        result = runtime.Register(runtime.EventRouter(), 2u, key,
                                  window, binding.callback);
        runtime.WriteWord(window + 40u + 8u * index, result);
        runtime.WriteWord(window + 44u + 8u * index, key);
    }
    return result;
}

std::uint32_t CombatRoleStateFactory::GetInstance(
    ICombatRoleStateFactoryRuntime& runtime) {
    const auto type = runtime.ReadWord(kFactorySource);
    runtime.WriteWord(kFactoryCache, type);
    return type;
}

std::uint32_t CombatRoleStateFactory::GetInstanceThunk(
    ICombatRoleStateFactoryRuntime& runtime) {
    return GetInstance(runtime);
}

std::uint32_t CombatRoleStateFactory::Create(
    const std::uint32_t layout_name, ICombatRoleStateFactoryRuntime& runtime) {
    const auto storage = runtime.Allocate(0x20cu);
    return storage ? runtime.ConstructRoleState(storage, layout_name) : 0u;
}

std::uint32_t CombatRoleStateFactory::RegisterFactory(
    ICombatRoleStateFactoryRuntime& runtime) {
    (void)runtime.FrameManager();
    return runtime.InsertFactory(runtime.ReadWord(kFactoryCache),
                                 kCreateCallback);
}

std::uint32_t CombatRoleStateFactory::RegisterFactoryThunk(
    ICombatRoleStateFactoryRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t CombatRoleStateFactory::RegisterEvent(
    ICombatRoleStateFactoryRuntime& runtime) {
    const auto type = runtime.ReadWord(kFactorySource);
    auto callback = runtime.Allocate(8u);
    if (callback) {
        runtime.WriteWord(callback + 4u, type);
        runtime.WriteWord(callback, kEventCallbackVtable);
    }
    const auto manager = runtime.FrameManager();
    runtime.InsertEvent(manager + 260u, kEventKey, callback);
    if (callback) runtime.DeleteCallback(callback, 1u);
    return kEventSentinel;
}

std::uint32_t CombatRoleStateFactory::RegisterEventThunk(
    ICombatRoleStateFactoryRuntime& runtime) {
    return RegisterEvent(runtime);
}

}  // namespace pal4
