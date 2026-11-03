#include "pal4/game_arena_script_clump_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaScriptClumpCall;
using Runtime = IGameArenaScriptClumpRuntime;

std::uint32_t Invoke(Runtime& runtime, Call operation,
                     std::initializer_list<std::uint32_t> arguments = {}) {
    return runtime.Call(operation, arguments);
}

bool HasWorldAndObject(std::uint32_t world, std::uint32_t object,
                       Runtime& runtime) {
    return runtime.ReadWord(world + 48u) != 0u && object != 0u;
}

}  // namespace

std::uint32_t GameArenaScriptClumpCallbacks::ActivateClump(
    std::uint32_t world, std::uint32_t object, Runtime& runtime) {
    if (!HasWorldAndObject(world, object, runtime)) return 0u;
    const auto clump = runtime.ReadWord(object + 68u);
    if (clump == 0u) return 0u;

    const auto type = runtime.ReadWord(object + 4u);
    runtime.WriteWord(object + 76u, 1u);
    if (type == 4u) {
        Invoke(runtime, Call::AddCameraClump,
               {Invoke(runtime, Call::GetCameraSubsystem), clump});
    } else if (type != 5u) {
        const auto resource = runtime.ReadWord(object + 64u);
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        const auto adapterSlot = runtime.ReadWord(adapter);
        const auto property = Invoke(runtime, Call::GetResourceProperty,
                                     {resource});
        Invoke(runtime, Call::SetClumpProperties,
               {adapterSlot, clump, property, 1u, 1u, resource});
        if (runtime.ReadWord(object + 4u) == 0u) {
            const auto nextResource = runtime.ReadWord(object + 64u);
            const auto nextClump = runtime.ReadWord(object + 68u);
            const auto nextAdapter = Invoke(runtime, Call::GetMovementAdapter);
            const auto nextSlot = runtime.ReadWord(nextAdapter + 4u);
            const auto nextProperty = Invoke(
                runtime, Call::GetResourceProperty, {nextResource});
            Invoke(runtime, Call::SetClumpProperties,
                   {nextSlot, nextClump, nextProperty, 1u, 1u,
                    nextResource});
        }
    }
    return 1u;
}

std::uint32_t GameArenaScriptClumpCallbacks::DeactivateClump(
    std::uint32_t world, std::uint32_t object, Runtime& runtime) {
    if (!HasWorldAndObject(world, object, runtime)) return 0u;
    const auto clump = runtime.ReadWord(object + 68u);
    if (clump == 0u) return 1u;

    const auto type = runtime.ReadWord(object + 4u);
    runtime.WriteWord(object + 76u, 0u);
    if (type == 4u) {
        Invoke(runtime, Call::RemoveCameraClump,
               {Invoke(runtime, Call::GetCameraSubsystem), clump});
    } else if (type != 5u) {
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        Invoke(runtime, Call::RestoreClumpRenderCallback,
               {runtime.ReadWord(adapter), clump});
        if (runtime.ReadWord(object + 4u) == 0u) {
            const auto nextClump = runtime.ReadWord(object + 68u);
            const auto nextAdapter = Invoke(runtime, Call::GetMovementAdapter);
            Invoke(runtime, Call::RestoreClumpRenderCallback,
                   {runtime.ReadWord(nextAdapter + 4u), nextClump});
        }
    }
    return 1u;
}

std::uint32_t GameArenaScriptClumpCallbacks::AddAnimatedObject(
    std::uint32_t world, std::uint32_t object, Runtime& runtime) {
    if (!HasWorldAndObject(world, object, runtime)) return 0u;
    Invoke(runtime, Call::CleanupObject, {object});
    if (runtime.ReadWord(object + 0x184u) != 0u &&
        runtime.ReadWord(object + 0x170u) != 0u) {
        if (runtime.ReadWord(object + 24u) == 0u)
            Invoke(runtime, Call::AddClump,
                   {world, runtime.ReadWord(object + 20u)});
        Invoke(runtime, Call::Virtual72, {object});
        Invoke(runtime, Call::ProcessClumpAtomics,
               {runtime.ReadWord(object + 20u), world + 300u});
        if (runtime.ReadWord(object + 0x1a8u) != 0u)
            Invoke(runtime, Call::ProcessAtomics,
                   {runtime.ReadWord(object + 20u)});
        Invoke(runtime, Call::ProcessAtomicsFinal,
               {runtime.ReadWord(object + 20u)});
        if (Invoke(runtime, Call::Virtual140, {object}) != 0u) {
            const auto clump = runtime.ReadWord(object + 20u);
            Invoke(runtime, Call::AddCameraClump,
                   {Invoke(runtime, Call::GetCameraSubsystem), clump});
        }
    }
    return 1u;
}

std::uint32_t GameArenaScriptClumpCallbacks::RemoveAnimatedObject(
    std::uint32_t world, std::uint32_t object, Runtime& runtime) {
    if (!HasWorldAndObject(world, object, runtime)) return 0u;
    Invoke(runtime, Call::Virtual156, {object, 0u});
    Invoke(runtime, Call::CleanupObject, {object});
    Invoke(runtime, Call::Virtual76, {object});
    if (runtime.ReadWord(object + 24u) == 0u)
        Invoke(runtime, Call::RemoveClump,
               {world, runtime.ReadWord(object + 20u)});
    Invoke(runtime, Call::ResetAtomicCallbacks,
           {runtime.ReadWord(object + 20u)});
    if (runtime.ReadWord(object + 0x1a8u) != 0u)
        Invoke(runtime, Call::ResetMaterialCallbacks,
               {runtime.ReadWord(object + 20u)});
    Invoke(runtime, Call::ProcessClumpAtomicsWrapper,
           {runtime.ReadWord(object + 20u)});
    if (Invoke(runtime, Call::Virtual140, {object}) != 0u) {
        const auto clump = runtime.ReadWord(object + 20u);
        Invoke(runtime, Call::RemoveCameraClump,
               {Invoke(runtime, Call::GetCameraSubsystem), clump});
    }
    return 1u;
}

}  // namespace pal4
