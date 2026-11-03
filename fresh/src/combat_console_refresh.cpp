#include "pal4/combat_console.h"

namespace pal4 {
namespace {

void RefreshActors(const std::uint32_t console,
                   ICombatConsoleActorIterator& actors,
                   const bool monsters,
                   ICombatConsoleSceneRuntime& runtime) {
    while (!actors.AtEnd()) {
        const auto actor = actors.Current();
        if (runtime.ActorState(actor)) {
            const auto index = runtime.ActorIndex(actor);
            if (monsters)
                (void)CombatConsoleWindow::HideElementSecond(
                    console, index, runtime);
            else
                (void)CombatConsoleWindow::HideElement(
                    console, index, runtime);
        } else {
            const auto image_name = runtime.ActorImageName(actor);
            const auto index = runtime.ActorIndex(actor);
            if (monsters)
                (void)CombatConsoleWindow::SetImageAndPositionSecond(
                    console, index, image_name, runtime);
            else
                (void)CombatConsoleWindow::SetImageAndPosition(
                    console, index, image_name, runtime);
            const auto progress = runtime.ActorPositionProgress(actor);
            const auto position_index = runtime.ActorIndex(actor);
            if (monsters)
                (void)CombatConsoleWindow::AnimatePositionSecond(
                    console, position_index, progress, runtime);
            else
                (void)CombatConsoleWindow::AnimatePosition(
                    console, position_index, progress, runtime);
        }
        actors.Advance();
    }
}

}  // namespace

void CombatConsoleWindow::RefreshFromEvent(
    const std::uint32_t console, const std::uint32_t,
    ICombatConsoleSceneRuntime& runtime) {
    Refresh(console, runtime);
}

std::uint32_t CombatConsoleWindow::AnimateRoleEvent(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleSceneRuntime& runtime) {
    const auto actor = runtime.ActorFromEvent(event);
    const auto progress = runtime.ActorPositionProgress(actor);
    const auto index = runtime.ActorIndex(actor);
    return AnimatePosition(console, index, progress, runtime);
}

std::uint32_t CombatConsoleWindow::AnimateMonsterEvent(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleSceneRuntime& runtime) {
    const auto actor = runtime.ActorFromEvent(event);
    const auto progress = runtime.ActorPositionProgress(actor);
    const auto index = runtime.ActorIndex(actor);
    return AnimatePositionSecond(console, index, progress, runtime);
}

void CombatConsoleWindow::HideAllFromEvent(
    const std::uint32_t console, const std::uint32_t,
    ICombatConsoleImageRuntime& runtime) {
    HideAll(console, runtime);
}

void CombatConsoleWindow::HideAll(
    const std::uint32_t console, ICombatConsoleImageRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.SetChildVisible(
            runtime.ReadWord(console + 224u + index * 4u), false);
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.SetChildVisible(
            runtime.ReadWord(console + 236u + index * 4u), false);
}

void CombatConsoleWindow::Refresh(
    const std::uint32_t console, ICombatConsoleSceneRuntime& runtime) {
    HideAll(console, runtime);
    auto roles = runtime.RoleActors();
    RefreshActors(console, *roles, false, runtime);
    auto monsters = runtime.MonsterActors();
    RefreshActors(console, *monsters, true, runtime);
    UpdateCombatCount(console, runtime);
}

void CombatConsoleWindow::UpdateCombatCount(
    const std::uint32_t console, ICombatConsoleSceneRuntime& runtime) {
    auto actors = runtime.CountMonsterActors();
    std::uint32_t count = 0u;
    while (!actors->AtEnd()) {
        const auto actor = actors->Current();
        if (runtime.ActorStateZero(actor) || runtime.ActorReady(actor))
            ++count;
        actors->Advance();
    }
    runtime.SetCombatCountText(runtime.ReadWord(console + 220u),
                               std::to_string(count));
}

}  // namespace pal4
