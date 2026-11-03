#include "pal4/game_arena_script_scene_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaScriptSceneCall;
using Runtime = IGameArenaScriptSceneRuntime;

std::uint32_t Invoke(Runtime& runtime, Call operation,
                     std::uint32_t first = 0u,
                     std::uint32_t second = 0u,
                     std::uint32_t third = 0u) {
    return runtime.Call(operation, first, second, third);
}

std::uint32_t Pet(Runtime& runtime) {
    return Invoke(runtime, Call::GetPetInstance);
}

}  // namespace

std::uint32_t GameArenaScriptSceneCallbacks::AddObject(
    std::uint32_t world, std::uint32_t object, Runtime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || object == 0u) return 0u;
    if (runtime.ReadWord(object + 0x1f0u) != 0u) {
        if (runtime.ReadWord(object + 24u) == 0u)
            Invoke(runtime, Call::AddClump, world,
                   runtime.ReadWord(object + 20u));
        Invoke(runtime, Call::SetAnimationFlag, object + 0x170u, 1u);
        Invoke(runtime, Call::RenderCamera, object, 1u,
               runtime.ReadWord(world + 16u));
        Invoke(runtime, Call::ProcessClumpAtomics,
               runtime.ReadWord(object + 20u), world + 300u);
    }
    if (runtime.ReadWord(world + 404u) != 0u &&
        static_cast<std::int32_t>(
            Invoke(runtime, Call::GetCharacterProperty)) > 0) {
        Invoke(runtime, Call::InitializeProperties, object,
               runtime.ReadWord(world + 404u));
    }
    return 1u;
}

std::uint32_t GameArenaScriptSceneCallbacks::RemoveObject(
    std::uint32_t world, std::uint32_t object, Runtime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || object == 0u) return 0u;
    Invoke(runtime, Call::CleanupCamera, object);
    Invoke(runtime, Call::SetStateFlag, object, 0u);
    Invoke(runtime, Call::CleanupObject, object);
    if (runtime.ReadWord(object + 24u) == 0u)
        Invoke(runtime, Call::RemoveClump, world,
               runtime.ReadWord(object + 20u));
    Invoke(runtime, Call::GetObjectProperty, object);
    if (runtime.ReadWord(object + 0xb10u) != 0u) {
        const auto clump = runtime.ReadWord(object + 20u);
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        Invoke(runtime, Call::SetClumpRenderCallback,
               runtime.ReadWord(adapter), clump);
        runtime.WriteWord(object + 0xb10u, 0u);
    }
    Invoke(runtime, Call::ProcessClumpAtomicsWrapper,
           runtime.ReadWord(object + 20u));
    return 1u;
}

std::uint32_t GameArenaScriptSceneCallbacks::AddPet(
    std::uint32_t world, Runtime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u) return 0u;
    if (runtime.ReadWord(Pet(runtime) + 0x1f0u) != 0u) {
        if (runtime.ReadWord(Pet(runtime) + 24u) == 0u)
            Invoke(runtime, Call::AddClump, world,
                   runtime.ReadWord(Pet(runtime) + 20u));
        Invoke(runtime, Call::SetAnimationFlag, Pet(runtime) + 0x170u, 1u);
        const auto camera = runtime.ReadWord(world + 16u);
        Invoke(runtime, Call::RenderCamera, Pet(runtime), 1u, camera);
        Invoke(runtime, Call::ProcessClumpAtomics,
               runtime.ReadWord(Pet(runtime) + 20u), world + 300u);
    }
    if (runtime.ReadWord(world + 404u) != 0u &&
        static_cast<std::int32_t>(
            Invoke(runtime, Call::GetCharacterProperty)) > 0) {
        const auto pet = Pet(runtime);
        Invoke(runtime, Call::InitializeProperties, pet,
               runtime.ReadWord(world + 404u));
    }
    return 1u;
}

std::uint32_t GameArenaScriptSceneCallbacks::RemovePet(
    std::uint32_t world, Runtime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u) return 0u;
    if (runtime.ReadWord(Pet(runtime) + 20u) == 0u) return 0u;
    Invoke(runtime, Call::CleanupCamera, Pet(runtime));
    Invoke(runtime, Call::SetStateFlag, Pet(runtime), 0u);
    Invoke(runtime, Call::CleanupObject, Pet(runtime));
    if (runtime.ReadWord(Pet(runtime) + 24u) == 0u)
        Invoke(runtime, Call::RemoveClump, world,
               runtime.ReadWord(Pet(runtime) + 20u));
    Invoke(runtime, Call::GetObjectProperty, Pet(runtime));
    Invoke(runtime, Call::ProcessClumpAtomicsWrapper,
           runtime.ReadWord(Pet(runtime) + 20u));
    return 1u;
}

}  // namespace pal4
