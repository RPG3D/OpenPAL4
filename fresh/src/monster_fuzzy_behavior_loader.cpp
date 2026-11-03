#include "pal4/monster_fuzzy_behavior_loader.h"

namespace pal4 {
namespace {

std::int32_t ReadInt(IMonsterFuzzyBehaviorRuntime& runtime,
                     std::uint32_t node, const char* name,
                     std::int32_t fallback) {
    const auto attribute = runtime.Attribute(node, name);
    return attribute != 0u
        ? runtime.ParseInt(attribute, fallback) : fallback;
}

float ReadFloat(IMonsterFuzzyBehaviorRuntime& runtime,
                std::uint32_t node, const char* name, float fallback) {
    const auto attribute = runtime.Attribute(node, name);
    return attribute != 0u
        ? runtime.ParseFloat(attribute, fallback) : fallback;
}

}  // namespace

std::uint32_t MonsterFuzzyBehaviorRuntimeStub::Attribute(
    std::uint32_t, const char*) { return 0u; }
std::int32_t MonsterFuzzyBehaviorRuntimeStub::ParseInt(
    std::uint32_t, std::int32_t fallback) { return fallback; }
float MonsterFuzzyBehaviorRuntimeStub::ParseFloat(
    std::uint32_t, float fallback) { return fallback; }
std::int32_t MonsterFuzzyBehaviorRuntimeStub::ChildCount(
    std::uint32_t) { return 0; }
std::uint32_t MonsterFuzzyBehaviorRuntimeStub::ChildByIndex(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t MonsterFuzzyBehaviorRuntimeStub::CreateEvent(
    std::uint32_t, const char*) { return 0u; }
std::uint32_t MonsterFuzzyBehaviorRuntimeStub::AddClassProportion(
    std::uint32_t, std::int32_t, std::int32_t) { return 0u; }
void MonsterFuzzyBehaviorRuntimeStub::SetIntAttribute(
    std::uint32_t, const char*, std::int32_t) {}
void MonsterFuzzyBehaviorRuntimeStub::SetFloatAttribute(
    std::uint32_t, const char*, float) {}
void MonsterFuzzyBehaviorRuntimeStub::SetGroupMode(
    std::uint32_t, std::int32_t) {}

std::int32_t MonsterFuzzyBehaviorLoader::ResolveObjectType(
    std::int32_t object_type) {
    return object_type >= 0 && object_type <= 3 ? object_type : -1;
}

void MonsterFuzzyBehaviorLoader::Load(
    std::uint32_t manager_address, std::uint32_t config_address,
    IMonsterFuzzyBehaviorRuntime& runtime) {
    if (config_address == 0u)
        return;

    const auto group_mode = ReadInt(
        runtime, config_address, "MSTINFO_Behavior_GroupMode", 3);
    if (group_mode == 3)
        return;

    const auto event = runtime.CreateEvent(
        manager_address, "MST-Fuzzy-Behavior");
    for (std::uint32_t index = 0u;
         static_cast<std::int32_t>(index) < runtime.ChildCount(config_address);
         ++index) {
        const auto child = runtime.ChildByIndex(config_address, index);
        const auto type = ReadInt(runtime, child,
            "MSTINFO_Behavior_ObjectType", -1);
        const auto proportion = ReadInt(runtime, child,
            "MSTINFO_Behavior_ObjectProportion", -1);
        const auto wait_mode = ReadInt(runtime, child,
            "MSTINFO_Behavior_ObjectWaitMode", 0);
        const auto wait_time = ReadFloat(runtime, child,
            "MSTINFO_Behavior_ObjectWaitTime", -1.0f);
        const auto expect_time = ReadFloat(runtime, child,
            "MSTINFO_Behavior_ObjectExpectTime", -1.0f);
        if (type < 0 || proportion < 0 || wait_mode <= 0)
            return;

        const auto class_id = ResolveObjectType(type);
        const auto record = runtime.AddClassProportion(
            event, class_id, proportion);
        runtime.SetIntAttribute(record, "beh-vtm-waitMode", wait_mode);
        runtime.SetFloatAttribute(record, "beh-vtm-waitTime", wait_time);
        runtime.SetFloatAttribute(record, "beh-vtm-expectTime", expect_time);
    }
    runtime.SetGroupMode(event, group_mode);
}

}  // namespace pal4
