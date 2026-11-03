#pragma once

#include "pal4/monster_path_endpoint_restore.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMonsterAnimationFlagSaveRuntime {
public:
    virtual ~IMonsterAnimationFlagSaveRuntime() = default;
    [[nodiscard]] virtual std::int32_t ScriptVersion() = 0;
    virtual void WriteMemoryInt(std::uint32_t memory,
                                std::string_view key,
                                std::int32_t value) = 0;
    [[nodiscard]] virtual float AnimationTime(
        std::uint32_t animation_address) = 0;
    virtual void WriteMemoryFloat(std::uint32_t memory,
                                  std::string_view key, float value) = 0;
    virtual void RemoveGridElement(std::uint32_t grid_address,
                                   std::uint32_t helper_address) = 0;
    [[nodiscard]] virtual std::uint32_t SelectAnimation(
        std::uint32_t animation_address, std::string_view name) = 0;
};

class MonsterAnimationFlagSaveRuntimeStub final
    : public IMonsterAnimationFlagSaveRuntime {
public:
    std::int32_t ScriptVersion() override;
    void WriteMemoryInt(std::uint32_t memory, std::string_view key,
                        std::int32_t value) override;
    float AnimationTime(std::uint32_t animation_address) override;
    void WriteMemoryFloat(std::uint32_t memory, std::string_view key,
                          float value) override;
    void RemoveGridElement(std::uint32_t grid_address,
                           std::uint32_t helper_address) override;
    std::uint32_t SelectAnimation(std::uint32_t animation_address,
                                  std::string_view name) override;
};

class MonsterAnimationFlagSave final {
public:
    // IDA 0x436b70: save class-ID-two animation-completion target.
    [[nodiscard]] static std::uint32_t Release(
        const MonsterPathEndpointTargetState& target,
        const MonsterBehaviorManagerState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterAnimationFlagSaveRuntime& runtime);
};

}  // namespace pal4
