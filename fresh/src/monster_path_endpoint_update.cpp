#include "pal4/monster_path_endpoint_update.h"

namespace pal4 {
namespace {

BehaviorVector3 ReadVectorByComponent(
    std::uint32_t helper_address, std::uint32_t slot,
    IMonsterPathEndpointUpdateRuntime& runtime) {
    BehaviorVector3 value{};
    for (std::uint32_t component = 0u; component < 3u; ++component)
        value[component] =
            runtime.ReadHelperVector(helper_address, slot)[component];
    return value;
}

void ProcessVariant(const MonsterPathEndpointTargetState& target,
                    const BehaviorPathEndpointState& helper,
                    std::uint32_t helper_address,
                    MonsterPathEndpointUpdateGlobals& globals,
                    IMonsterPathEndpointUpdateRuntime& runtime,
                    std::uint32_t manager_offset);

}  // namespace

double MonsterPathEndpointUpdateRuntimeStub::ManagerDoubleProperty(
    std::uint32_t) { return 0.0; }
BehaviorVector3 MonsterPathEndpointUpdateRuntimeStub::ReadHelperVector(
    std::uint32_t, std::uint32_t) { return {}; }
void MonsterPathEndpointUpdateRuntimeStub::InitializeMovementAdapter() {}
void MonsterPathEndpointUpdateRuntimeStub::CheckCollision(
    BehaviorVector3&, float, float) {}
void MonsterPathEndpointUpdateRuntimeStub::ApplyHelperPosition(
    std::uint32_t, const BehaviorVector3&) {}
void MonsterPathEndpointUpdateRuntimeStub::UpdateOrientation(
    std::uint32_t, const BehaviorVector3&, const BehaviorVector3&,
    const BehaviorVector3&, const BehaviorVector3&) {}

void MonsterPathEndpointUpdate::Process(
    const MonsterPathEndpointTargetState& target,
    const BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    MonsterPathEndpointUpdateGlobals& globals,
    IMonsterPathEndpointUpdateRuntime& runtime) {
    ProcessVariant(target, helper, helper_address, globals, runtime, 0x1cu);
}

void MonsterPathEndpointC03Update::Process(
    const MonsterPathEndpointTargetState& target,
    const BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    MonsterPathEndpointUpdateGlobals& globals,
    IMonsterPathEndpointUpdateRuntime& runtime) {
    ProcessVariant(target, helper, helper_address, globals, runtime, 0x14u);
}

void MonsterRandomPointUpdate::Process(
    const MonsterPathEndpointTargetState& target,
    const BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    MonsterPathEndpointUpdateGlobals& globals,
    IMonsterPathEndpointUpdateRuntime& runtime) {
    ProcessVariant(target, helper, helper_address, globals, runtime, 0x04u);
}

namespace {

void ProcessVariant(const MonsterPathEndpointTargetState& target,
                    const BehaviorPathEndpointState& helper,
                    std::uint32_t helper_address,
                    MonsterPathEndpointUpdateGlobals& globals,
                    IMonsterPathEndpointUpdateRuntime& runtime,
                    std::uint32_t manager_offset) {
    const auto manager_address = target.words[manager_offset / 4u];
    if ((globals.guard & 1u) == 0u) {
        globals.guard = static_cast<std::uint8_t>(globals.guard | 1u);
        globals.first_radius = static_cast<float>(
            runtime.ManagerDoubleProperty(manager_address) *
            0.3499999940395355);
    }
    if ((globals.guard & 2u) == 0u) {
        globals.guard = static_cast<std::uint8_t>(globals.guard | 2u);
        globals.second_radius = static_cast<float>(
            runtime.ManagerDoubleProperty(manager_address) *
            2.5299999713897705);
    }
    if (((helper.words[0xb8u / 4u] >> 8u) & 0xffu) == 0u)
        return;

    globals.position = ReadVectorByComponent(
        helper_address, 0x18u, runtime);
    runtime.InitializeMovementAdapter();
    runtime.CheckCollision(globals.position,
                           globals.first_radius,
                           globals.second_radius);
    runtime.ApplyHelperPosition(helper_address, globals.position);
    globals.orientation_one = ReadVectorByComponent(
        helper_address, 0x10u, runtime);
    globals.orientation_two = ReadVectorByComponent(
        helper_address, 0x00u, runtime);
    globals.orientation_three = ReadVectorByComponent(
        helper_address, 0x08u, runtime);
    runtime.UpdateOrientation(manager_address,
        globals.orientation_one, globals.orientation_two,
        globals.orientation_three, globals.position);
}

}  // namespace

}  // namespace pal4
