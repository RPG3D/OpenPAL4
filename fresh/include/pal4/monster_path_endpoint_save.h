#pragma once

#include "pal4/monster_path_endpoint_restore.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMonsterPathEndpointSaveRuntime {
public:
    virtual ~IMonsterPathEndpointSaveRuntime() = default;
    [[nodiscard]] virtual std::int32_t ScriptVersion() = 0;
    virtual void WriteMemoryInt(std::uint32_t memory,
                                 std::string_view key,
                                 std::int32_t value) = 0;
    virtual void WriteMemoryFloat(std::uint32_t memory,
                                   std::string_view key,
                                   float value) = 0;
    [[nodiscard]] virtual float HelperSpeed(
        std::uint32_t helper_address) = 0;
    [[nodiscard]] virtual float AnimationTime(
        std::uint32_t animation_address) = 0;
    virtual void RemoveGridElement(std::uint32_t grid_address,
                                    std::uint32_t helper_address) = 0;
    [[nodiscard]] virtual std::uint32_t SelectAction(
        std::uint32_t animation_address, std::string_view name,
        float blend) = 0;
    [[nodiscard]] virtual std::uint32_t SetAnimationFlag(
        std::uint32_t animation_address, std::uint32_t flag) = 0;
};

class MonsterPathEndpointSaveRuntimeStub
    : public IMonsterPathEndpointSaveRuntime {
public:
    std::int32_t ScriptVersion() override;
    void WriteMemoryInt(std::uint32_t memory, std::string_view key,
                        std::int32_t value) override;
    void WriteMemoryFloat(std::uint32_t memory, std::string_view key,
                          float value) override;
    float HelperSpeed(std::uint32_t helper_address) override;
    float AnimationTime(std::uint32_t animation_address) override;
    void RemoveGridElement(std::uint32_t grid_address,
                           std::uint32_t helper_address) override;
    std::uint32_t SelectAction(std::uint32_t animation_address,
                               std::string_view name,
                               float blend) override;
    std::uint32_t SetAnimationFlag(std::uint32_t animation_address,
                                   std::uint32_t flag) override;
};

class MonsterPathEndpointSave final {
public:
    // IDA 0x435c20: save class-ID-zero monster path endpoint state.
    [[nodiscard]] static std::uint32_t Release(
        const MonsterPathEndpointTargetState& target,
        const MonsterBehaviorManagerState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime);
};

class MonsterPathEndpointC03Save final {
public:
    // IDA 0x436610: save class-ID-one state and restore the animation flag.
    [[nodiscard]] static std::uint32_t Release(
        const MonsterPathEndpointTargetState& target,
        const MonsterBehaviorManagerState& manager,
        BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        IMonsterPathEndpointSaveRuntime& runtime);
};

}  // namespace pal4
