#include "pal4/combat_message_subscriptions.h"

namespace pal4 {

std::uint32_t CombatMessageSubscriptions::Clear(
    const std::uint32_t message, ICombatMessageSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(message, 40u, 1u, runtime);
    return 0u;
}

std::uint32_t CombatMessageSubscriptions::Register(
    const std::uint32_t message, ICombatMessageSubscriptionRuntime& runtime) {
    (void)Clear(message, runtime);
    const auto key = runtime.MessageEventKey();
    const auto callback = runtime.Register(runtime.EventRouter(), 3u, key,
                                           message, kCallback);
    runtime.WriteWord(message + 44u, key);
    runtime.WriteWord(message + 40u, callback);
    return callback;
}

std::uint32_t CombatMessageFactory::GetInstance(
    ICombatMessageFactoryRuntime& runtime) {
    const auto instance = runtime.ReadWord(kFactorySource);
    runtime.WriteWord(kFactoryCache, instance);
    return instance;
}

std::uint32_t CombatMessageFactory::GetInstanceThunk(
    ICombatMessageFactoryRuntime& runtime) {
    return GetInstance(runtime);
}

std::uint32_t CombatMessageFactory::Create(
    const std::uint32_t argument, ICombatMessageFactoryRuntime& runtime) {
    const auto storage = runtime.Allocate(0x44u);
    return storage ? runtime.ConstructMessage(storage, argument) : 0u;
}

std::uint32_t CombatMessageFactory::RegisterFactory(
    ICombatMessageFactoryRuntime& runtime) {
    (void)runtime.FrameManager();
    return runtime.InsertFactory(runtime.ReadWord(kFactoryCache),
                                 kCreateCallback);
}

std::uint32_t CombatMessageFactory::RegisterFactoryThunk(
    ICombatMessageFactoryRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t CombatMessageFactory::RegisterEvent(
    ICombatMessageFactoryRuntime& runtime) {
    auto callback = runtime.Allocate(8u);
    if (callback) {
        runtime.WriteWord(callback + 4u, kShowCallback);
        runtime.WriteWord(callback, kEventCallbackVtable);
    }
    const auto manager = runtime.FrameManager();
    runtime.InsertEvent(manager + 260u, kEventKey, callback);
    if (callback) runtime.DeleteCallback(callback, 1u);
    return kEventSentinel;
}

std::uint32_t CombatMessageFactory::RegisterEventThunk(
    ICombatMessageFactoryRuntime& runtime) {
    return RegisterEvent(runtime);
}

}  // namespace pal4
