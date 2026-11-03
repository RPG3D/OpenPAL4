#pragma once
#include "pal4/pal_actor_control.h"
#include <bit>
#include <initializer_list>

namespace pal4 {
// Shared IDA 423AC0 algorithm; runtime owns platform/game endpoints.
template<class Runtime>
std::uint32_t ResetActorControlState(
    PalActorControlState& state, bool clear_monster_flags,
    Runtime& runtime) {
    auto& words = state.words;
    words[0] = clear_monster_flags ? 1u : 0u;
    for (const auto index : {20u, 21u, 22u, 14u, 15u, 16u, 17u, 18u})
        words[index] = 0u;
    runtime.SetCursor("default.cur");
    words[32] = 0u;
    runtime.ProcessInput(state);
    runtime.ProcessInputAndClearVector(state);
    runtime.InitializeInputState();

    if (!clear_monster_flags) {
        runtime.ProcessMonsters();
        if (words[1] != 0u && words[33] == 2u)
            runtime.SetActorAnimationFlag(words[1], 0x3f800000u);
        for (const auto index : {279u, 186u}) {
            if (runtime.DialogNeedsUpdate(words.data() + index))
                runtime.UpdateDialog(words.data() + index);
        }
        if (words[184] != 0u) {
            words[185] = 0u;
            words[184] = 0u;
            if (runtime.DialogNeedsUpdate(words.data() + 186u))
                runtime.UpdateDialog(words.data() + 186u);
        }
        const auto pet = runtime.GetPetManager();
        if (runtime.PetActive(pet) && words[19] == 0u) {
            runtime.ScriptPetOnRemove(runtime.GetArenaManager());
            runtime.PetVirtualRemove(runtime.GetPetManager());
            return runtime.ResetPet(runtime.GetPetManager());
        }
        return pet;
    }

    runtime.ClearMonsterFlags();
    if (words[1] != 0u && words[33] == 2u) {
        runtime.UpdateGameStateConditional(state, 2u);
        runtime.UpdateGameStateAndUi(state, words[1]);
    }
    if (words[184] != 0u &&
        std::bit_cast<float>(words[185]) > 0.0f)
        runtime.UpdateDialogStatus(state, words[185]);

    const auto pet = runtime.GetPetManager();
    if (runtime.PetActive(pet) && words[19] == 0u) {
        runtime.ScriptPetOnRemove(runtime.GetArenaManager());
        runtime.PetVirtualRemove(runtime.GetPetManager());
        static_cast<void>(runtime.ResetPet(runtime.GetPetManager()));
        runtime.PetVirtualLoad(runtime.GetPetManager(), "PAL4Pet", "339", 1u);
        runtime.InitializePetData(runtime.GetPetManager(), words[1]);
        runtime.SetPetStateFlag(runtime.GetPetManager(), 1u);
        return runtime.ScriptPetOnAdd(runtime.GetArenaManager());
    }
    return pet;
}

} // namespace pal4
