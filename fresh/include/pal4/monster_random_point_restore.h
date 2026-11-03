#pragma once

#include "pal4/monster_path_endpoint_restore.h"

namespace pal4 {

class IMonsterRandomPointRuntime {
public:
    virtual ~IMonsterRandomPointRuntime() = default;
    [[nodiscard]] virtual std::int32_t SoundRuntimeType() = 0;
    [[nodiscard]] virtual BehaviorVector3 CurrentHelperPoint(
        std::uint32_t helper_address) = 0;
    [[nodiscard]] virtual float ReadSoundActionTime(
        std::uint32_t memory, float fallback) = 0;
};

class MonsterRandomPointRuntimeStub final : public IMonsterRandomPointRuntime {
public:
    std::int32_t SoundRuntimeType() override;
    BehaviorVector3 CurrentHelperPoint(std::uint32_t helper_address) override;
    float ReadSoundActionTime(std::uint32_t memory, float fallback) override;
};

class MonsterRandomPointRestore final {
public:
    // IDA 0x436c40: restore class-ID-three wander or point target.
    static void Accept(const MonsterPathEndpointTargetState& target,
                       const MonsterBehaviorManagerState& manager,
                       BehaviorPathEndpointState& helper,
                       std::uint32_t helper_address,
                       IMonsterPathEndpointRestoreRuntime& runtime,
                       IMonsterRandomPointRuntime& point_runtime);
};

}  // namespace pal4
