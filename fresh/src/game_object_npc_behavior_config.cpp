#include "pal4/game_object_npc_behavior_config.h"

namespace pal4 {

std::uint32_t GameObjectNpcBehaviorRuntimeStub::FindAttribute(
    std::uint32_t, const char*) { return 0u; }
std::int32_t GameObjectNpcBehaviorRuntimeStub::AttributeToInt(
    std::uint32_t) { return 0; }
float GameObjectNpcBehaviorRuntimeStub::AttributeToFloat(
    std::uint32_t) { return 0.0f; }
std::uint32_t GameObjectNpcBehaviorRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t GameObjectNpcBehaviorRuntimeStub::GetChildByIndex(
    std::uint32_t, std::uint32_t&) { return 0u; }
std::uint32_t GameObjectNpcBehaviorRuntimeStub::AddEvent(
    std::uint32_t, const char*) { return 0u; }
std::int32_t GameObjectNpcBehaviorRuntimeStub::ResolveClassId(
    std::int32_t object_type) {
    return GameObjectNpcBehaviorConfig::ClassIdFromObjectType(object_type);
}
std::uint32_t GameObjectNpcBehaviorRuntimeStub::AddClassProportion(
    std::uint32_t, std::int32_t, std::int32_t) { return 0u; }
void GameObjectNpcBehaviorRuntimeStub::WriteInt(
    std::uint32_t, const char*, std::int32_t) {}
void GameObjectNpcBehaviorRuntimeStub::WriteFloat(
    std::uint32_t, const char*, float) {}
void GameObjectNpcBehaviorRuntimeStub::SetGroupMode(
    std::uint32_t, std::int32_t) {}

namespace {

std::int32_t ReadInt(std::uint32_t node, const char* name,
                     std::int32_t fallback,
                     IGameObjectNpcBehaviorRuntime& runtime) {
    const auto attribute = runtime.FindAttribute(node, name);
    return attribute != 0u ? runtime.AttributeToInt(attribute) : fallback;
}

float ReadFloat(std::uint32_t node, const char* name, float fallback,
                IGameObjectNpcBehaviorRuntime& runtime) {
    const auto attribute = runtime.FindAttribute(node, name);
    return attribute != 0u ? runtime.AttributeToFloat(attribute) : fallback;
}

}  // namespace

void GameObjectNpcBehaviorConfig::Process(
    std::uint32_t owner_address, std::uint32_t config_node,
    IGameObjectNpcBehaviorRuntime& runtime) {
    if (config_node == 0u) return;

    const auto group_mode = ReadInt(
        config_node, "NPCINFO_Behavior_GroupMode", 3, runtime);
    if (group_mode == 3) return;

    const auto event = runtime.AddEvent(
        owner_address + 0x658u, "NPC-Fuzzy-Behavior");
    const auto count = static_cast<std::int32_t>(
        runtime.ReadDword(config_node + 0x38u));
    for (std::uint32_t index = 0u;
         static_cast<std::int32_t>(index) < count; ++index) {
        const auto child = runtime.GetChildByIndex(config_node, index);
        const auto object_type = ReadInt(
            child, "NPCINFO_Behavior_ObjectType", -1, runtime);
        const auto proportion = ReadInt(
            child, "NPCINFO_Behavior_ObjectProportion", -1, runtime);
        const auto wait_mode = ReadInt(
            child, "NPCINFO_Behavior_ObjectWaitMode", 0, runtime);
        const auto wait_time = ReadFloat(
            child, "NPCINFO_Behavior_ObjectWaitTime", -1.0f, runtime);
        const auto expect_time = ReadFloat(
            child, "NPCINFO_Behavior_ObjectExpectTime", -1.0f, runtime);
        if (object_type < 0 || proportion < 0 || wait_mode <= 0) return;

        const auto class_id = runtime.ResolveClassId(object_type);
        const auto selected = runtime.AddClassProportion(
            event, class_id, proportion);
        runtime.WriteInt(selected, "beh-vtm-waitMode", wait_mode);
        runtime.WriteFloat(selected, "beh-vtm-waitTime", wait_time);
        runtime.WriteFloat(selected, "beh-vtm-expectTime", expect_time);
    }
    runtime.SetGroupMode(event, group_mode);
}

std::int32_t GameObjectNpcBehaviorConfig::ClassIdFromObjectType(
    std::int32_t object_type) {
    return object_type >= 0 && object_type <= 4 ? object_type : -1;
}

}  // namespace pal4
