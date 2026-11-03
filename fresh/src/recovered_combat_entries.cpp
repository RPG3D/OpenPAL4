#include "pal4/recovered_combat_entries.h"

namespace pal4 {

std::uint32_t MatFxScriptBoot::Initialize(
    const std::uint32_t owner, IMatFxScriptBootRuntime& runtime) {
    runtime.PrepareScripts();
    runtime.ExecuteScript("gamedata\\MatFX\\Shader\\ltMap.fx");
    runtime.CleanupScripts();
    const auto result = runtime.SetupCombatCamera(owner);
    runtime.WriteWord(owner + 68u, 0u);
    return result;
}

int ActorManagerFloatTask::ConvertAndProcess(
    const std::uint32_t manager, const double value,
    IActorManagerFloatTaskRuntime& runtime) {
    return runtime.Process(manager + 44u, static_cast<float>(value));
}

std::uint32_t CombatEndingScoreElement::ResultWord(
    const std::uint32_t element, ICombatTaskResultWordRuntime& runtime) {
    return runtime.ReadWord(element + 36u);
}

int CombatEndingScoreElement::Destroy(
    const std::uint32_t element, ICombatEndingScoreElementRuntime& runtime) {
    runtime.WriteWord(element, 0x845390u);
    runtime.CleanupCollection(element + 4u);
    const auto text = runtime.ReadWord(element + 24u);
    if (text) {
        const auto refs = runtime.ReadByte(text - 1u);
        if (refs == 0u || refs == 0xffu)
            runtime.Free(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                static_cast<std::uint8_t>(refs - 1u));
    }
    runtime.WriteWord(element + 24u, 0u);
    runtime.WriteWord(element + 28u, 0u);
    runtime.WriteWord(element + 32u, 0u);
    return runtime.DestroyContainer(element + 4u);
}

std::uint32_t CombatConsoleInitThunk::Call(
    ICombatConsoleInitThunkRuntime& runtime) {
    return runtime.InitializeCombatConsole();
}

}  // namespace pal4
