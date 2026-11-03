#include "pal4/game_arena_npc_item_callbacks.h"

namespace pal4 {
namespace {

using Call = GameArenaNpcItemCall;
using Runtime = IGameArenaNpcItemRuntime;

std::uint32_t Invoke(Runtime& runtime, Call operation,
                     std::initializer_list<std::uint32_t> arguments = {}) {
    return runtime.Call(operation, arguments);
}

std::uint32_t Enter(std::uint32_t object, std::uint32_t world,
                    std::uint32_t clumpPropertiesOffset, Runtime& runtime) {
    if (runtime.ReadWord(world + 0x30u) == 0u || object == 0u) return 0u;

    if (runtime.ReadWord(object + 0x1f0u) != 0u) {
        if (runtime.ReadWord(object + 0x18u) == 0u)
            Invoke(runtime, Call::AddClump,
                   {world, runtime.ReadWord(object + 0x14u)});

        const auto clump = runtime.ReadWord(object + 0x14u);
        const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
        Invoke(runtime, Call::SetClumpProperties,
               {runtime.ReadWord(adapter), clump, 1u, 0u, 0u,
                object + clumpPropertiesOffset});
        Invoke(runtime, Call::SetAnimationFlag, {object + 0x170u, 1u});
        Invoke(runtime, Call::RenderCamera,
               {object, 1u, runtime.ReadWord(world + 0x10u)});
        Invoke(runtime, Call::Virtual72,
               {object, runtime.ReadWord(object + 0x0cu), 0xffffffffu});
        Invoke(runtime, Call::ProcessClumpAtomics,
               {runtime.ReadWord(object + 0x14u), world + 0x12cu});
        Invoke(runtime, Call::ProcessAtomics,
               {runtime.ReadWord(object + 0x14u)});
        Invoke(runtime, Call::ProcessAtomicsFinal,
               {runtime.ReadWord(object + 0x14u)});
    }
    if (runtime.ReadWord(world + 0x194u) != 0u &&
        static_cast<std::int32_t>(
            Invoke(runtime, Call::GetCharacterProperty)) > 0) {
        Invoke(runtime, Call::InitializeProperties,
               {object, runtime.ReadWord(world + 0x194u)});
    }
    return object;
}

std::uint32_t Leave(std::uint32_t object, std::uint32_t world,
                    bool isNpc, Runtime& runtime) {
    if (runtime.ReadWord(world + 0x30u) == 0u || object == 0u) return 0u;

    Invoke(runtime, Call::CleanupCamera, {object});
    Invoke(runtime, Call::GetObjectProperty, {object});
    Invoke(runtime, Call::SetStateFlag, {object, 0u});
    Invoke(runtime, Call::CleanupObject, {object});
    if (isNpc) Invoke(runtime, Call::Virtual68, {object});
    Invoke(runtime, Call::Virtual76, {object});

    const auto clump = runtime.ReadWord(object + 0x14u);
    const auto adapter = Invoke(runtime, Call::GetMovementAdapter);
    Invoke(runtime, Call::SetClumpRenderCallback,
           {runtime.ReadWord(adapter), clump});
    Invoke(runtime, Call::ResetAtomicCallbacks,
           {runtime.ReadWord(object + 0x14u)});
    Invoke(runtime, Call::ResetMaterialCallbacks,
           {runtime.ReadWord(object + 0x14u)});
    Invoke(runtime, Call::ProcessClumpAtomicsWrapper,
           {runtime.ReadWord(object + 0x14u)});
    if (runtime.ReadWord(object + 0x18u) == 0u)
        Invoke(runtime, Call::RemoveClump,
               {world, runtime.ReadWord(object + 0x14u)});
    return object;
}

}  // namespace

std::uint32_t GameArenaNpcItemCallbacks::EnterNpc(
    std::uint32_t npc, std::uint32_t world, Runtime& runtime) {
    return Enter(npc, world, 0x6dcu, runtime);
}

std::uint32_t GameArenaNpcItemCallbacks::LeaveNpc(
    std::uint32_t npc, std::uint32_t world, Runtime& runtime) {
    return Leave(npc, world, true, runtime);
}

std::uint32_t GameArenaNpcItemCallbacks::EnterItem(
    std::uint32_t item, std::uint32_t world, Runtime& runtime) {
    return Enter(item, world, 0x6d4u, runtime);
}

std::uint32_t GameArenaNpcItemCallbacks::LeaveItem(
    std::uint32_t item, std::uint32_t world, Runtime& runtime) {
    return Leave(item, world, false, runtime);
}

}  // namespace pal4
