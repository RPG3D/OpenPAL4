#include "pal4/trade_window_bootstrap.h"

namespace pal4 {

std::uint32_t TradeWindowBootstrap::GetNameThunk(
    ITradeWindowBootstrapRuntime& runtime) {
    return GetName(runtime);
}

std::uint32_t TradeWindowBootstrap::GetName(
    ITradeWindowBootstrapRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    runtime.WriteWord(kCurrentName, name);
    return name;
}

std::uint32_t TradeWindowBootstrap::Create(
    const std::uint32_t name, ITradeWindowBootstrapRuntime& runtime) {
    const auto storage = runtime.Allocate(0x1b4u);
    if (storage == 0u) return 0u;
    try {
        return runtime.ConstructWindow(storage, name);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
}

std::uint32_t TradeWindowBootstrap::RegisterFactoryThunk(
    ITradeWindowBootstrapRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t TradeWindowBootstrap::RegisterFactory(
    ITradeWindowBootstrapRuntime& runtime) {
    const auto manager = runtime.FrameManager();
    return runtime.ProcessFrameEvent(manager,
        runtime.ReadWord(kCurrentName), kFactoryAddress);
}

std::uint32_t TradeWindowBootstrap::RegisterShowEventThunk(
    ITradeWindowBootstrapRuntime& runtime) {
    return RegisterShowEvent(runtime);
}

std::uint32_t TradeWindowBootstrap::RegisterShowEvent(
    ITradeWindowBootstrapRuntime& runtime) {
    const auto name = runtime.ReadWord(kNameSource);
    const auto callback = runtime.Allocate(8u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, name);
        runtime.WriteWord(callback, kShowCallbackVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                kShowEvent, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
    return kShowRegistration;
}

}  // namespace pal4
