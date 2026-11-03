#include "pal4/game_arena_clump_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaClumpCall;

std::uint32_t Invoke(IGameArenaClumpRuntime& runtime, const Call operation,
                     const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u,
                     const std::uint32_t third = 0u,
                     const std::uint32_t fourth = 0u,
                     const std::uint32_t fifth = 0u,
                     const std::uint32_t sixth = 0u) {
    return runtime.Call(operation, first, second, third, fourth, fifth,
                        sixth);
}

void ApplyClump(IGameArenaClumpRuntime& runtime,
                const std::uint32_t adapter_slot,
                const std::uint32_t clump,
                const std::uint32_t resource) {
    const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
    const auto owner = runtime.ReadWord(adapter + adapter_slot);
    const auto property = Invoke(runtime, Call::GetResourceProperty, resource);
    Invoke(runtime, Call::SetClumpProperties, owner, clump, property, 1u,
           1u, resource);
}

}  // namespace

std::uint32_t GameArenaClumpCallbacks::Enter(
    const std::uint32_t object, const std::uint32_t world,
    IGameArenaClumpRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || object == 0u) return 0u;
    const auto clump = runtime.ReadWord(object + 68u);
    if (clump == 0u) return object;

    const auto type = runtime.ReadWord(object + 4u);
    runtime.WriteWord(object + 76u, 1u);
    if (type == 4u) {
        Invoke(runtime, Call::AddCameraClump,
               Invoke(runtime, Call::GetCameraSubsystem), clump);
    } else if (type != 5u) {
        const auto resource = runtime.ReadWord(object + 64u);
        ApplyClump(runtime, 0u, clump, resource);
        if (runtime.ReadWord(object + 4u) == 0u) {
            const auto second_resource = runtime.ReadWord(object + 64u);
            const auto second_clump = runtime.ReadWord(object + 68u);
            ApplyClump(runtime, 4u, second_clump, second_resource);
        }
    }
    return object;
}

std::uint32_t GameArenaClumpCallbacks::Leave(
    const std::uint32_t object, const std::uint32_t world,
    IGameArenaClumpRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || object == 0u) return 0u;
    const auto clump = runtime.ReadWord(object + 68u);
    if (clump == 0u) return object;

    const auto type = runtime.ReadWord(object + 4u);
    runtime.WriteWord(object + 76u, 0u);
    if (type == 4u) {
        Invoke(runtime, Call::RemoveCameraClump,
               Invoke(runtime, Call::GetCameraSubsystem), clump);
    } else if (type != 5u) {
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        Invoke(runtime, Call::RestoreRenderCallback,
               runtime.ReadWord(adapter), clump);
        if (runtime.ReadWord(object + 4u) == 0u) {
            const auto second_clump = runtime.ReadWord(object + 68u);
            const auto second_adapter = Invoke(runtime,
                                               Call::GetMovementAdapter);
            Invoke(runtime, Call::RestoreRenderCallback,
                   runtime.ReadWord(second_adapter + 4u), second_clump);
        }
    }
    return object;
}

}  // namespace pal4
