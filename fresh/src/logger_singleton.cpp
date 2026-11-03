#include "pal4/logger_singleton.h"

namespace pal4 {

std::uint32_t LoggerSingletonAccess::Get(
    LoggerSingletonState& state, ILoggerSingletonRuntime& runtime) {
    if ((state.guard & 1U) != 0) return state.instance;

    state.guard = static_cast<std::uint8_t>(state.guard | 1U);
    const auto manager = runtime.GetManager();
    state.instance = runtime.Register(manager, "logger", 1,
                                      kFactoryAddress, kTypeInfoAddress);
    return state.instance;
}

}  // namespace pal4
