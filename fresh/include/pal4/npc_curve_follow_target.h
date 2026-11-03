#pragma once

#include "pal4/monster_path_endpoint_save.h"
#include "pal4/npc_path_endpoint_restore.h"

namespace pal4 {

class NpcCurveFollowTarget final {
public:
    // IDA 0x43ba00: restores the class-four NPC curve-follow target.
    [[nodiscard]] static std::uint32_t Accept(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointRestoreRuntime& runtime);

    // IDA 0x43bfd0: saves the target and releases its helper.
    [[nodiscard]] static std::uint32_t Release(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime);
};

}  // namespace pal4
