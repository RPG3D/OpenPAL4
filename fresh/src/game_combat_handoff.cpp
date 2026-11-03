#include "pal4/game_combat_handoff.h"

#include "pal4/monster_behavior_manager.h"

namespace pal4 {
namespace {

struct DocumentOwner {
    IGameCombatHandoffRuntime& runtime;
    bool active{true};

    ~DocumentOwner() {
        if (active)
            (void)runtime.DestroyDocument();
    }

    std::int32_t Close() {
        active = false;
        return runtime.DestroyDocument();
    }
};

}  // namespace

std::int32_t GameCombatHandoff::Process(
    MonsterBehaviorManagerState& manager, std::int32_t requested_mode,
    IGameCombatHandoffRuntime& runtime) {
    const auto controller = runtime.StateController();
    if (controller == 0u)
        return 0;
    const auto ready = runtime.ControllerReady(controller);
    if (ready != 1)
        return ready;

    runtime.CreateDocument("CombatParam");
    DocumentOwner document{runtime};
    const auto arena = runtime.ArenaName();
    auto path = runtime.ResolveArenaPath(
        arena, static_cast<std::uint8_t>(requested_mode));
    if (path.empty())
        path = "x";
    runtime.ClearStateText();
    const auto text_begin = manager.words[0x724u / 4u];
    for (auto record = text_begin; record != manager.words[0x728u / 4u];
         record += 16u) {
        const auto text = runtime.ReadDword(record + 4u);
        runtime.AddStateText(text == 0u ? runtime.EmptyTextAddress() : text);
    }
    runtime.FinalizeManagerText(manager, text_begin,
                                manager.words[0x728u / 4u]);

    if (requested_mode >= 0)
        manager.words[0x700u / 4u] =
            static_cast<std::uint32_t>(requested_mode);
    const auto mode = manager.words[0x700u / 4u];
    if (mode <= 2u) {
        std::int32_t count = 0;
        const auto begin = runtime.StateMonsterBegin();
        if (begin != 0u) {
            const auto end = runtime.StateMonsterEnd();
            count = static_cast<std::int32_t>(end - begin) / 12;
        }
        const auto world_mode = mode == 1u ? 2u :
                                mode == 2u ? 1u : 0u;
        runtime.SetCombatWorld("combatWorld", path, world_mode, count);
    }

    const auto monster_begin = manager.words[0x714u / 4u];
    for (auto record = monster_begin;
         record != manager.words[0x718u / 4u]; record += 12u) {
        const auto id = runtime.ReadDword(record);
        const auto value = runtime.ReadDword(record + 4u);
        const auto group = runtime.ReadDword(record + 8u) == 2u ? 1u : 0u;
        runtime.SetCombatMonster(id, value, group);
    }
    manager.words[0x718u / 4u] = monster_begin;
    runtime.InitializeCombatState(kCombatState);
    return document.Close();
}

}  // namespace pal4
