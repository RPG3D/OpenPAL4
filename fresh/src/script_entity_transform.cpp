#include "pal4/script_entity_transform.h"

namespace pal4 {

std::uint32_t ScriptEntityTransform::PlayerByIndex(
    const std::int32_t index, const float x, const float y, const float z,
    const float yaw, IScriptEntityTransformRuntime& runtime) {
    return Apply(runtime.FindPlayer(runtime.PlayerManager(), index),
                 x, y, z, yaw, runtime);
}

std::uint32_t ScriptEntityTransform::NpcByName(
    const char* name, const float x, const float y, const float z,
    const float yaw, IScriptEntityTransformRuntime& runtime) {
    return Apply(runtime.FindNpc(runtime.NpcManager(), name ? name : ""),
                 x, y, z, yaw, runtime);
}

std::uint32_t ScriptEntityTransform::Apply(
    const std::uint32_t entity, const float x, const float y, const float z,
    const float yaw, IScriptEntityTransformRuntime& runtime) {
    if (entity == 0u) return 0u;
    const auto rotation_z = runtime.ReadFloat(entity + 0xa8u);
    const auto rotation_x = runtime.ReadFloat(entity + 0xa0u);
    runtime.SetPosition(entity, {x, y, z}, 0u);
    return runtime.SetRotation(entity, {rotation_x, yaw, rotation_z}, 0u);
}

}  // namespace pal4
