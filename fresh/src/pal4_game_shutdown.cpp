#include "pal4/pal4_game_shutdown.h"

namespace pal4 {

std::int32_t Pal4GameShutdown::ShutdownIfRequested(
    const std::int32_t requested, IPal4GameShutdownRuntime& runtime) {
    if (requested != 0) {
        const auto first_game = runtime.GameInstance();
        if (runtime.ReadWord(first_game + kGameObjectOffset) != 0u) {
            const auto second_game = runtime.GameInstance();
            const auto object = runtime.ReadWord(second_game + kGameObjectOffset);
            static_cast<void>(runtime.ShutdownGameObject(object));
        }
    }
    return 2;
}

std::int32_t Pal4GameShutdown::CheckAndShutdown(
    IPal4GameShutdownRuntime& runtime) {
    const auto first_game = runtime.GameInstance();
    if (runtime.ReadWord(first_game + kGameObjectOffset) == 0u) return 2;
    const auto second_game = runtime.GameInstance();
    const auto object = runtime.ReadWord(second_game + kGameObjectOffset);
    const auto result = runtime.ShutdownGameObject(object);
    return result == 0 || result == 1 ? 1 : 2;
}

std::uint32_t Pal4GameShutdown::CleanupResources(
    IPal4GameShutdownRuntime& runtime) {
    const auto font = runtime.FontManager();
    runtime.GetFontTextures(font);
    runtime.GetEffectScriptSlot284();
    const auto effects = runtime.ArenaEffectManager();
    static_cast<void>(runtime.CleanupCollection(effects + kCollectionOffset));
    const auto combat = runtime.CombatSystem();
    const auto result = runtime.ScriptState();
    const auto combat_object = runtime.ReadWord(combat + kCombatObjectOffset);
    if (combat_object != 0u)
        return runtime.CleanupCollection(combat_object + kCollectionOffset);
    return result;
}

std::uint32_t Pal4GameShutdown::CleanupAlternateResources(
    IPal4GameShutdownRuntime& runtime) {
    const auto font = runtime.FontManager();
    runtime.GetFontTextureStates(font);
    runtime.GetEffectScriptSlot288();
    const auto effects = runtime.ArenaEffectManager();
    static_cast<void>(runtime.CleanupCollectionAlternate(
        effects + kCollectionOffset));
    const auto combat = runtime.CombatSystem();
    const auto result = runtime.ScriptProperty288();
    const auto combat_object = runtime.ReadWord(combat + kCombatObjectOffset);
    if (combat_object != 0u)
        return runtime.CleanupCollectionAlternate(
            combat_object + kCollectionOffset);
    return result;
}

}  // namespace pal4
