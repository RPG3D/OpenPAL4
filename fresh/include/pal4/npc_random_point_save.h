#pragma once

#include "pal4/monster_path_endpoint_save.h"
#include "pal4/npc_random_point_restore.h"

namespace pal4 {

class NpcRandomPointSave final {
public:
    // IDA 0x43b670: saves and releases the class-ID-three NPC wander target.
    [[nodiscard]] static std::uint32_t Release(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime,
        IMonsterRandomPointRuntime& point_runtime);
};

}  // namespace pal4
