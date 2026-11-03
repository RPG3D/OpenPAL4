#include "pal4/id_generator_lifecycle.h"

namespace pal4 {

std::uint32_t IdGeneratorLifecycle::Initialize(
    IdGeneratorGlobalState& state, std::uint32_t incoming_ecx) {
    if ((state.guard & 1u) == 0u) {
        state.seed = static_cast<std::uint8_t>(incoming_ecx >> 24u);
        state.guard = static_cast<std::uint8_t>(state.guard | 1u);
        state.begin = 0u;
        state.end = 0u;
        state.capacity = 0u;
        runtime_.RegisterAtExit(kCleanupCallback);
    }
    return kGlobalAddress;
}

void IdGeneratorLifecycle::Cleanup(IdGeneratorGlobalState& state) {
    runtime_.FreeMemory(state.begin);
    state.begin = 0u;
    state.end = 0u;
    state.capacity = 0u;
}

}  // namespace pal4
