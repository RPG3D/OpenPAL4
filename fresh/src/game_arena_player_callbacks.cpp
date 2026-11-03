#include "pal4/game_arena_player_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaPlayerCall;

std::uint32_t Invoke(IGameArenaPlayerRuntime& runtime, const Call operation,
                     const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u,
                     const std::uint32_t third = 0u) {
    return runtime.Call(operation, first, second, third);
}

}  // namespace

std::uint32_t GameArenaPlayerCallbacks::Enter(
    const std::uint32_t player, const std::uint32_t world,
    IGameArenaPlayerRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || player == 0u) return 0u;

    if (runtime.ReadWord(player + 496u) != 0u) {
        if (runtime.ReadWord(player + 24u) == 0u)
            Invoke(runtime, Call::AddClump, world,
                   runtime.ReadWord(player + 20u));
        Invoke(runtime, Call::SetAnimationFlag, player + 368u, 1u);
        Invoke(runtime, Call::RenderCamera, player, 1u,
               runtime.ReadWord(world + 16u));
        Invoke(runtime, Call::ProcessClumpAtomics,
               runtime.ReadWord(player + 20u), world + 300u);
    }
    if (runtime.ReadWord(world + 404u) != 0u &&
        static_cast<std::int32_t>(Invoke(runtime,
                                         Call::GetCharacterProperty)) > 0) {
        Invoke(runtime, Call::InitializeProperties, player,
               runtime.ReadWord(world + 404u));
    }
    return player;
}

std::uint32_t GameArenaPlayerCallbacks::Leave(
    const std::uint32_t player, const std::uint32_t world,
    IGameArenaPlayerRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || player == 0u) return 0u;

    Invoke(runtime, Call::CleanupCamera, player);
    Invoke(runtime, Call::GetObjectProperty, player);
    Invoke(runtime, Call::SetStateFlag, player, 0u);
    Invoke(runtime, Call::CleanupObject, player);
    if (runtime.ReadWord(player + 24u) == 0u)
        Invoke(runtime, Call::RemoveClump, world,
               runtime.ReadWord(player + 20u));
    if (runtime.ReadWord(player + 0xb10u) != 0u) {
        const auto clump = runtime.ReadWord(player + 20u);
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        Invoke(runtime, Call::SetClumpRenderCallback,
               runtime.ReadWord(adapter), clump);
        runtime.WriteWord(player + 0xb10u, 0u);
    }
    Invoke(runtime, Call::ProcessClumpAtomicsWrapper,
           runtime.ReadWord(player + 20u));
    return player;
}

}  // namespace pal4
