#include "pal4/game_arena_script_character_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaScriptCharacterCall;
using Runtime = IGameArenaScriptCharacterRuntime;

std::uint32_t Invoke(Runtime& runtime, Call operation,
                     std::initializer_list<std::uint32_t> arguments = {}) {
    return runtime.Call(operation, arguments);
}

std::uint32_t LeaveCharacter(std::uint32_t world,
                             std::uint32_t character, bool callVirtual68,
                             Runtime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || character == 0u) return 0u;
    Invoke(runtime, Call::CleanupCamera, {character});
    Invoke(runtime, Call::GetObjectProperty, {character});
    Invoke(runtime, Call::SetStateFlag, {character, 0u});
    Invoke(runtime, Call::CleanupObject, {character});
    if (callVirtual68) Invoke(runtime, Call::Virtual68, {character});
    Invoke(runtime, Call::Virtual76, {character});
    const auto clump = runtime.ReadWord(character + 20u);
    const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
    Invoke(runtime, Call::SetClumpRenderCallback,
           {runtime.ReadWord(adapter), clump});
    if (runtime.ReadWord(character + 24u) == 0u)
        Invoke(runtime, Call::RemoveClump,
               {world, runtime.ReadWord(character + 20u)});
    Invoke(runtime, Call::ResetAtomicCallbacks,
           {runtime.ReadWord(character + 20u)});
    Invoke(runtime, Call::ResetMaterialCallbacks,
           {runtime.ReadWord(character + 20u)});
    Invoke(runtime, Call::ProcessClumpAtomicsWrapper,
           {runtime.ReadWord(character + 20u)});
    return 1u;
}

}  // namespace

std::uint32_t GameArenaScriptCharacterCallbacks::Enter(
    std::uint32_t world, std::uint32_t character, Runtime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u || character == 0u) return 0u;
    if (runtime.ReadWord(character + 0x1f0u) != 0u) {
        if (runtime.ReadWord(character + 24u) == 0u)
            Invoke(runtime, Call::AddClump,
                   {world, runtime.ReadWord(character + 20u)});
        const auto clump = runtime.ReadWord(character + 20u);
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        Invoke(runtime, Call::SetClumpProperties,
               {runtime.ReadWord(adapter), clump, 1u, 0u, 0u, 0u});
        Invoke(runtime, Call::SetAnimationFlag,
               {character + 0x170u, 1u});
        Invoke(runtime, Call::RenderCamera,
               {character, 1u, runtime.ReadWord(world + 16u)});
        Invoke(runtime, Call::Virtual72,
               {character, runtime.ReadWord(character + 12u), 0xffffffffu});
        Invoke(runtime, Call::ProcessClumpAtomics,
               {runtime.ReadWord(character + 20u), world + 300u});
        Invoke(runtime, Call::ProcessAtomics,
               {runtime.ReadWord(character + 20u)});
        Invoke(runtime, Call::ProcessAtomicsFinal,
               {runtime.ReadWord(character + 20u)});
    }
    if (runtime.ReadWord(world + 404u) != 0u &&
        static_cast<std::int32_t>(
            Invoke(runtime, Call::GetCharacterProperty)) > 0) {
        Invoke(runtime, Call::InitializeProperties,
               {character, runtime.ReadWord(world + 404u)});
    }
    return 1u;
}

std::uint32_t GameArenaScriptCharacterCallbacks::LeaveWithVirtual68(
    std::uint32_t world, std::uint32_t character, Runtime& runtime) {
    return LeaveCharacter(world, character, true, runtime);
}

std::uint32_t GameArenaScriptCharacterCallbacks::Leave(
    std::uint32_t world, std::uint32_t character, Runtime& runtime) {
    return LeaveCharacter(world, character, false, runtime);
}

}  // namespace pal4
