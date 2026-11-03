#include "pal4/game_manager_data_lifecycle.h"

namespace pal4 {

void GameManagerDataMemoryStub::Free(std::uint32_t) {}

void GameManagerDataLifecycle::Cleanup(
    GameManagerDataState& state, IGameManagerDataMemory& memory) {
    state.words[0] = kCleanupVtable;
    memory.Free(state.words[0x2b8u / 4u]);
    state.words[0x2b8u / 4u] = 0u;
    state.words[0x2bcu / 4u] = 0u;
    state.words[0x2c0u / 4u] = 0u;
    memory.Free(state.words[0x2a8u / 4u]);
    state.words[0x2a8u / 4u] = 0u;
    state.words[0x2acu / 4u] = 0u;
    state.words[0x2b0u / 4u] = 0u;
    state.words[0] = kBaseVtable;
}

std::uint32_t GameManagerDataLifecycle::DestroyDeleting(
    GameManagerDataState& state, std::uint32_t object,
    std::uint32_t flags, IGameManagerDataMemory& memory) {
    Cleanup(state, memory);
    if ((flags & 1u) != 0u)
        memory.Free(object);
    return object;
}

}  // namespace pal4
