#pragma once

#include "pal4/monster_animation_flag_save.h"
#include "pal4/npc_path_endpoint_restore.h"

namespace pal4 {

class NpcAnimationFlagSave final {
public:
    // IDA 0x43b030: saves animation-completion state and releases its helper.
    [[nodiscard]] static std::uint32_t Release(
        const NpcPathEndpointTargetState& target,
        const GameObjectState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterAnimationFlagSaveRuntime& runtime);
};

}  // namespace pal4
