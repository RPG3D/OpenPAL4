#include "pal4/game_combat_handoff.h"
#include "pal4/monster_behavior_manager.h"
#include "pal4/monster_unique_text.h"

namespace pal4 {

std::uint32_t GameCombatHandoffRuntimeStub::StateController() { return 0u; }
std::int32_t GameCombatHandoffRuntimeStub::ControllerReady(std::uint32_t) {
    return 0;
}
void GameCombatHandoffRuntimeStub::CreateDocument(std::string_view) {}
std::string GameCombatHandoffRuntimeStub::ArenaName() { return {}; }
std::string GameCombatHandoffRuntimeStub::ResolveArenaPath(
    std::string_view, std::uint8_t) { return {}; }
void GameCombatHandoffRuntimeStub::ClearStateText() {}
void GameCombatHandoffRuntimeStub::AddStateText(std::uint32_t) {}
std::uint32_t GameCombatHandoffRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::uint32_t GameCombatHandoffRuntimeStub::EmptyTextAddress() {
    return 0x841964u;
}
void GameCombatHandoffRuntimeStub::FinalizeManagerText(
    MonsterBehaviorManagerState& manager, std::uint32_t begin,
    std::uint32_t end) {
    CeguiTextRecordArrayState array{
        manager.words[0x720u / 4u], begin, end,
        manager.words[0x72cu / 4u]};
    MonsterUniqueTextRuntimeStub memory;
    (void)CeguiTextRecordArray::EraseRange(array, begin, end, memory);
    manager.words[0x728u / 4u] = array.end;
}
std::uint32_t GameCombatHandoffRuntimeStub::StateMonsterBegin() {
    return 0u;
}
std::uint32_t GameCombatHandoffRuntimeStub::StateMonsterEnd() {
    return 0u;
}
void GameCombatHandoffRuntimeStub::SetCombatWorld(
    std::string_view, std::string_view, std::uint32_t, std::int32_t) {}
void GameCombatHandoffRuntimeStub::SetCombatMonster(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void GameCombatHandoffRuntimeStub::InitializeCombatState(std::uint32_t) {}
std::int32_t GameCombatHandoffRuntimeStub::DestroyDocument() { return 0; }

}  // namespace pal4
