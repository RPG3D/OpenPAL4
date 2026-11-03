#pragma once

#include "pal4/npc_path_endpoint_restore.h"

namespace pal4 {

class NpcAnimationFlagRestore final {
public:
    // IDA 0x43ade0: restores class-ID-two NPC animation target.
    [[nodiscard]] static std::uint32_t Accept(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointRestoreRuntime& runtime);

    // IDA 0x43b0e0: true when the target animation reports completion.
    [[nodiscard]] static bool IsDone(
        const NpcPathEndpointTargetState& target,
        IMonsterPathEndpointRestoreRuntime& runtime);
};

}  // namespace pal4
