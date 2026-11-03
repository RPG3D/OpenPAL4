#include "pal4/trade_system_lifecycle.h"

namespace pal4 {

std::uint32_t TradeSystemSingleton::GetInstance(
    ITradeSystemSingletonRuntime& runtime) {
    if ((state_.initialization_bits & 1u) != 0u) return state_.instance;
    state_.initialization_bits |= 1u;
    const auto manager = runtime.GetSingletonManager();
    state_.instance = runtime.Register(manager, "palTradeSystem", 1053u,
                                       kFactoryAddress, kDestructorAddress);
    return state_.instance;
}

std::uint32_t TradeSystemLifecycle::CreateInstance(
    ITradeSystemLifecycleRuntime& runtime) {
    const auto memory = runtime.Allocate(kObjectBytes);
    return memory == 0u ? 0u : Construct(memory, runtime);
}

std::uint32_t TradeSystemLifecycle::Construct(
    const std::uint32_t object, ITradeSystemLifecycleRuntime& runtime) {
    runtime.Clear(object + 4u, 0x50u);
    runtime.WriteWord(object + 4u, 1u);
    return object;
}

}  // namespace pal4
