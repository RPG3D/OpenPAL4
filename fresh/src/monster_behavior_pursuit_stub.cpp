#include "pal4/monster_behavior_manager.h"
namespace pal4 {
std::string_view MonsterPursuitRuntimeStub::AnimationName(
    std::uint32_t) { return {}; }
std::uint32_t MonsterPursuitRuntimeStub::SelectAnimation(
    std::uint32_t, const char*) { return 0u; }
bool MonsterPursuitRuntimeStub::CheckProximity(
    MonsterBehaviorManagerState& state, std::uint32_t, float) {
    return MonsterProximityChecker::Check(state, proximity_);
}
bool MonsterPursuitRuntimeStub::PlayerControlBusy() { return false; }
bool MonsterPursuitRuntimeStub::CheckDistanceAndFlag(
    const MonsterBehaviorManagerState& state, std::uint32_t, float) {
    return MonsterProximityChecker::BeyondReturnThreshold(state, proximity_);
}
bool MonsterPursuitRuntimeStub::CheckTargetReached(
    const MonsterBehaviorManagerState& state, std::uint32_t address, float) {
    return MonsterProximityChecker::CheckReturnTargetReached(
        state, address, proximity_);
}
void MonsterPursuitRuntimeStub::ProcessCollisionAndConfig(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed) {
    (void)MonsterPursuitReturnState::Enter(state, address, elapsed, speed_);
}
bool MonsterPursuitRuntimeStub::ProcessPursuitCollisionAndSpeed(
    const MonsterBehaviorManagerState& state, std::uint32_t address,
    float elapsed, std::uint32_t target_word) {
    return MonsterPursuitCollisionSpeed::Process(
        state, address, elapsed, target_word, speed_);
}
bool MonsterPursuitRuntimeStub::ProcessObjectInteraction(
    MonsterBehaviorManagerState& state, std::uint32_t address,
    std::uint32_t target_word, float) {
    return MonsterInteractionProcessor::Process(
        state, address, target_word, interaction_);
}
void MonsterPursuitRuntimeStub::ProcessConfigAndEvents(
    std::uint32_t, float) {}
void MonsterPursuitRuntimeStub::InitializeCombatSystem(
    std::uint32_t, std::int32_t) {}
std::uint32_t MonsterPursuitRuntimeStub::ResetPursuit(
    MonsterBehaviorManagerState& state, std::uint32_t address) {
    return MonsterPursuitReset::Reset(state, address, *this);
}
IScriptEventProcessorRuntime& MonsterPursuitRuntimeStub::ProcessorRuntime() {
    return processor_;
}
void MonsterPursuitRuntimeStub::CleanupScriptObjectCollection(
    std::uint32_t) {}
void MonsterPursuitRuntimeStub::LoadFuzzyBehaviorConfig(
    std::uint32_t address, std::uint32_t child) {
    MonsterFuzzyBehaviorLoader::Load(address, child, fuzzy_);
}
void MonsterPursuitRuntimeStub::DispatchComponentField(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterPursuitRuntimeStub::SetRecordErrorFlag(
    std::uint32_t, std::uint8_t) {}
void MonsterPursuitRuntimeStub::ProcessActionWithScript(
    std::uint32_t address, float) {
    (void)MonsterActionDispatcher::Process(address, action_);
}
void MonsterPursuitRuntimeStub::VisitArenaObjects(std::uint32_t) {}

} // namespace pal4
