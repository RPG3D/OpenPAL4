#include "pal4/game_db_flag_static.h"

namespace pal4 {

int GameDbFlagStatic::Register(GameDbFlagStaticState& state,
                               const std::uint8_t initial_byte,
                               IGameDbFlagStaticRuntime& runtime) {
    (void)Initialize(state, initial_byte, runtime);
    return runtime.RegisterCleanup();
}

std::uint8_t GameDbFlagStatic::Initialize(
    GameDbFlagStaticState& state, const std::uint8_t initial_byte,
    IGameDbFlagStaticRuntime& runtime) {
    state.initialized_byte = initial_byte;
    state.data = 0u;
    state.length = 0u;
    state.capacity = 0u;
    const auto length = static_cast<std::uint32_t>(kText.size());
    (void)runtime.SetErrorFlag(state, 1u);
    runtime.Reallocate(state, length);
    runtime.CopyLiteral(state.data, kText);
    state.length = length;
    runtime.WriteByte(state.data + length, 0u);
    return static_cast<std::uint8_t>(state.data);
}

void GameDbFlagStatic::Deinitialize(GameDbFlagStaticState& state,
                                    IGameDbFlagStaticRuntime& runtime) {
    if ((state.cleanup_guard & 1u) != 0u) return;
    state.cleanup_guard = static_cast<std::uint8_t>(state.cleanup_guard | 1u);
    if (state.data != 0u) {
        const auto reference = state.data - 1u;
        const auto count = runtime.ReadByte(reference);
        if (count == 0u || count == 0xffu)
            runtime.Free(reference);
        else
            runtime.WriteByte(reference,
                              static_cast<std::uint8_t>(count - 1u));
    }
    state.data = state.length = state.capacity = 0u;
}

}  // namespace pal4
