#pragma once

#include "pal4/monster_path_endpoint_restore.h"

namespace pal4 {

class MonsterAnimationFlagRestore final {
public:
    // IDA 0x436920: restore class-ID-two animation-completion target.
    static void Accept(MonsterPathEndpointTargetState& target,
                       const MonsterBehaviorManagerState& manager,
                       BehaviorPathEndpointState& helper,
                       std::uint32_t helper_address,
                       IMonsterPathEndpointRestoreRuntime& runtime);
    // IDA 0x436c20: complete when the animation flag is nonzero.
    [[nodiscard]] static bool IsDone(
        const MonsterPathEndpointTargetState& target,
        IMonsterPathEndpointRestoreRuntime& runtime);
};

}  // namespace pal4
