#pragma once

#include "pal4/monster_random_point_restore.h"
#include "pal4/npc_path_endpoint_restore.h"

namespace pal4 {

class NpcRandomPointRestore final {
public:
    // IDA 0x43b100: restores the class-ID-three NPC wander target.
    [[nodiscard]] static std::uint32_t Accept(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointRestoreRuntime& runtime,
        IMonsterRandomPointRuntime& point_runtime);
};

}  // namespace pal4
