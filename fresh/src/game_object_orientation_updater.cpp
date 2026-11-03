#include "pal4/game_object_orientation_updater.h"

namespace pal4 {
namespace {

void UpdateVariant(const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject,
    std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals,
    IGameObjectOrientationRuntime& runtime,
    std::uint32_t manager_offset);

}  // namespace

double GameObjectOrientationRuntimeStub::GetDoubleProperty(
    std::uint32_t) { return 0.0; }
float GameObjectOrientationRuntimeStub::ReadSubjectVectorComponent(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0.0f; }
void GameObjectOrientationRuntimeStub::InitializeMovementAdapter() {}
void GameObjectOrientationRuntimeStub::CheckObjectIntersection(
    BehaviorVector3&, float, float,
    std::uint32_t, std::uint32_t) {}
void GameObjectOrientationRuntimeStub::UpdateSubjectPosition(
    std::uint32_t, const BehaviorVector3&) {}
void GameObjectOrientationRuntimeStub::UpdateOrientation(
    std::uint32_t, const BehaviorVector3&, const BehaviorVector3&,
    const BehaviorVector3&, const BehaviorVector3&) {}

void GameObjectOrientationUpdater::UpdateTarget(const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject, std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals, IGameObjectOrientationRuntime& runtime,
    std::uint32_t manager_offset) {
    UpdateVariant(owner,subject,subject_address,globals,runtime,manager_offset);
}

void GameObjectOrientationUpdater::UpdateVtable3(
    const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject,
    std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals,
    IGameObjectOrientationRuntime& runtime) {
    UpdateVariant(owner, subject, subject_address, globals, runtime, 0x24u);
}

void GameObjectOrientationUpdater::UpdateVtable4(
    const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject,
    std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals,
    IGameObjectOrientationRuntime& runtime) {
    UpdateVariant(owner, subject, subject_address, globals, runtime, 0x1cu);
}

void GameObjectOrientationUpdater::UpdateVtable5(
    const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject,
    std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals,
    IGameObjectOrientationRuntime& runtime) {
    UpdateVariant(owner, subject, subject_address, globals, runtime, 0x0cu);
}

void GameObjectOrientationUpdater::UpdateVtable6(
    const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject,
    std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals,
    IGameObjectOrientationRuntime& runtime) {
    UpdateVariant(owner, subject, subject_address, globals, runtime, 0x04u);
}

namespace {

void UpdateVariant(const GameObjectState& owner,
    const GameObjectOrientationSubjectState& subject,
    std::uint32_t subject_address,
    GameObjectOrientationGlobals& globals,
    IGameObjectOrientationRuntime& runtime,
    std::uint32_t manager_offset) {
    const auto manager = owner.words[manager_offset / 4u];
    if ((globals.guard & 1u) == 0u) {
        globals.guard |= 1u;
        globals.first_limit = static_cast<float>(
            runtime.GetDoubleProperty(manager) * 0.3499999940395355);
    }
    if ((globals.guard & 2u) == 0u) {
        globals.guard |= 2u;
        globals.second_limit = static_cast<float>(
            runtime.GetDoubleProperty(manager) * 2.5299999713897705);
    }
    if (subject.bytes[0xb9u] == 0u) return;

    for (std::uint32_t i = 0u; i < 3u; ++i)
        globals.candidate[i] = runtime.ReadSubjectVectorComponent(
            subject_address, 0x18u, i);
    runtime.InitializeMovementAdapter();
    runtime.CheckObjectIntersection(globals.candidate,
        globals.first_limit, globals.second_limit, 0u, 0u);
    runtime.UpdateSubjectPosition(subject_address, globals.candidate);

    for (std::uint32_t i = 0u; i < 3u; ++i)
        globals.first_basis[i] = runtime.ReadSubjectVectorComponent(
            subject_address, 0x10u, i);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        globals.second_basis[i] = runtime.ReadSubjectVectorComponent(
            subject_address, 0u, i);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        globals.third_basis[i] = runtime.ReadSubjectVectorComponent(
            subject_address, 0x08u, i);
    runtime.UpdateOrientation(manager, globals.first_basis,
        globals.second_basis, globals.third_basis, globals.candidate);
}

}  // namespace

}  // namespace pal4
