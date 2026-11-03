#include "pal4/game_arena_animated_object_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaAnimatedObjectCall;

std::uint32_t Invoke(IGameArenaAnimatedObjectRuntime& runtime,
                     const Call operation, const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u) {
    return runtime.Call(operation, first, second);
}

}  // namespace

std::uint32_t GameArenaAnimatedObjectCallbacks::Enter(
    const std::uint32_t object, const std::uint32_t world,
    IGameArenaAnimatedObjectRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || object == 0u) return 0u;
    Invoke(runtime, Call::CleanupObject, object);
    const auto active = runtime.ReadWord(object + 0x184u);
    const auto animated = runtime.ReadWord(object + 0x170u);
    if (active != 0u && animated != 0u) {
        if (runtime.ReadWord(object + 24u) == 0u)
            Invoke(runtime, Call::AddClump, world,
                   runtime.ReadWord(object + 20u));
        Invoke(runtime, Call::Virtual72, object);
        Invoke(runtime, Call::ProcessAtomics,
               runtime.ReadWord(object + 20u), world + 300u);
        if (runtime.ReadWord(object + 0x1a8u) != 0u)
            Invoke(runtime, Call::ProcessAtomicsOptional,
                   runtime.ReadWord(object + 20u));
        Invoke(runtime, Call::ProcessAtomicsFinal,
               runtime.ReadWord(object + 20u));
        if (Invoke(runtime, Call::Virtual140, object) != 0u) {
            const auto clump = runtime.ReadWord(object + 20u);
            Invoke(runtime, Call::AddCameraClump,
                   Invoke(runtime, Call::GetCameraSubsystem), clump);
        }
    } else if (!(active == 0u && animated != 0u) &&
               runtime.ReadWord(object + 0x174u) != 0u) {
        Invoke(runtime, Call::Virtual72, object);
    }
    return object;
}

std::uint32_t GameArenaAnimatedObjectCallbacks::Leave(
    const std::uint32_t object, const std::uint32_t world,
    IGameArenaAnimatedObjectRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || object == 0u) return 0u;
    Invoke(runtime, Call::Virtual156, object, 0u);
    Invoke(runtime, Call::CleanupObject, object);
    Invoke(runtime, Call::Virtual76, object);
    Invoke(runtime, Call::ResetAtomicCallbacks,
           runtime.ReadWord(object + 20u));
    if (runtime.ReadWord(object + 0x1a8u) != 0u)
        Invoke(runtime, Call::ResetMaterialCallbacks,
               runtime.ReadWord(object + 20u));
    Invoke(runtime, Call::ProcessAtomicsWrapper,
           runtime.ReadWord(object + 20u));
    if (runtime.ReadWord(object + 24u) == 0u)
        Invoke(runtime, Call::RemoveClump, world,
               runtime.ReadWord(object + 20u));
    if (Invoke(runtime, Call::Virtual140, object) != 0u) {
        const auto clump = runtime.ReadWord(object + 20u);
        Invoke(runtime, Call::RemoveCameraClump,
               Invoke(runtime, Call::GetCameraSubsystem), clump);
    }
    return object;
}

}  // namespace pal4
