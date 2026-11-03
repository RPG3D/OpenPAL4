#include "pal4/script_pet_visibility.h"

namespace pal4 {

std::uint32_t ScriptPetVisibility::SetEnabled(
    const std::uint8_t enabled, IScriptPetVisibilityRuntime& runtime) {
    runtime.WriteWord(runtime.PetManager() + 0x654u,
                      enabled == 0u ? 0u : 1u);
    runtime.RemovePet(runtime.ArenaManager());
    const auto cleanup_result = runtime.CleanupPet(runtime.PetManager());
    if (enabled == 0u) return cleanup_result;

    const auto first_manager = runtime.PlayerManager();
    const auto first_index = runtime.ReadWord(first_manager + 0x2c50u);
    const auto second_manager = runtime.PlayerManager();
    const auto second_index = runtime.ReadWord(second_manager + 0x2c50u);
    const auto player = second_manager + second_index * 0xb14u;
    if (first_index != 0u || player == 0u) return second_manager;

    runtime.CreatePet(runtime.PetManager(), "PAL4Pet", "339", 1u);
    runtime.InitializePet(runtime.PetManager(), player);
    runtime.SetStateFlag(runtime.PetManager(), 1u);
    return runtime.AddPet(runtime.ArenaManager());
}

}  // namespace pal4
