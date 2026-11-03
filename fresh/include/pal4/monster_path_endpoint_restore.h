#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct MonsterBehaviorManagerState;

struct MonsterPathEndpointTargetState {
    std::array<std::uint32_t, 8> words{};
};
static_assert(sizeof(MonsterPathEndpointTargetState) == 0x20u);

using MonsterTransformRows = std::array<BehaviorVector3, 4>;

class IMonsterPathEndpointRestoreRuntime {
public:
    virtual ~IMonsterPathEndpointRestoreRuntime() = default;
    [[nodiscard]] virtual std::string ReadStringAttribute(
        std::uint32_t node, std::string_view key,
        std::string_view fallback) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveWorldPath(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t GridParameter(
        std::uint32_t grid_address) = 0;
    virtual void ConfigurePath(BehaviorPathEndpointState& helper,
                               std::uint32_t helper_address,
                               std::uint32_t grid_parameter,
                               std::uint32_t path_address) = 0;
    [[nodiscard]] virtual std::int32_t ChildCount(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Child(
        std::uint32_t node, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string ChildName(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::int32_t ReadIntAttribute(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloatAttribute(
        std::uint32_t node, std::string_view key,
        float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadMemoryInt(
        std::uint32_t memory, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadMemoryFloat(
        std::uint32_t memory, std::string_view key,
        float fallback) = 0;
    virtual void SetHelperScalar(std::uint32_t helper_address,
                                 std::uint32_t slot, float value) = 0;
    [[nodiscard]] virtual MonsterTransformRows ActorTransform(
        std::uint32_t actor_component) = 0;
    virtual void SetHelperVector(std::uint32_t helper_address,
                                 std::uint32_t slot,
                                 const BehaviorVector3& value) = 0;
    virtual void AddGridElement(std::uint32_t grid_address,
                                std::uint32_t helper_address) = 0;
    [[nodiscard]] virtual bool SelectAnimation(
        std::uint32_t animation_address, std::string_view name) = 0;
    virtual void SetAnimationFlag(std::uint32_t animation_address,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::int32_t ScriptVersion() = 0;
    virtual void SetAnimationTime(std::uint32_t animation_address,
                                  float time) = 0;
    [[nodiscard]] virtual std::uint32_t SetAnimationTimeAndReturn(
        std::uint32_t animation_address, float time) {
        SetAnimationTime(animation_address, time);
        return 0u;
    }
    [[nodiscard]] virtual std::uint32_t AnimationFlag(
        std::uint32_t animation_address) = 0;
};

class MonsterPathEndpointRestoreRuntimeStub
    : public IMonsterPathEndpointRestoreRuntime {
public:
    std::string ReadStringAttribute(std::uint32_t node,
        std::string_view key, std::string_view fallback) override;
    std::uint32_t ResolveWorldPath(std::string_view name) override;
    std::uint32_t GridParameter(std::uint32_t grid_address) override;
    void ConfigurePath(BehaviorPathEndpointState& helper,
        std::uint32_t helper_address,
        std::uint32_t grid_parameter, std::uint32_t path_address) override;
    std::int32_t ChildCount(std::uint32_t node) override;
    std::uint32_t Child(std::uint32_t node, std::uint32_t index) override;
    std::string ChildName(std::uint32_t node) override;
    std::int32_t ReadIntAttribute(std::uint32_t node,
        std::string_view key, std::int32_t fallback) override;
    float ReadFloatAttribute(std::uint32_t node,
        std::string_view key, float fallback) override;
    std::int32_t ReadMemoryInt(std::uint32_t memory,
        std::string_view key, std::int32_t fallback) override;
    float ReadMemoryFloat(std::uint32_t memory,
        std::string_view key, float fallback) override;
    void SetHelperScalar(std::uint32_t helper_address,
        std::uint32_t slot, float value) override;
    MonsterTransformRows ActorTransform(std::uint32_t actor_component) override;
    void SetHelperVector(std::uint32_t helper_address,
        std::uint32_t slot, const BehaviorVector3& value) override;
    void AddGridElement(std::uint32_t grid_address,
        std::uint32_t helper_address) override;
    bool SelectAnimation(std::uint32_t animation_address,
        std::string_view name) override;
    void SetAnimationFlag(std::uint32_t animation_address,
        std::uint32_t flag) override;
    std::int32_t ScriptVersion() override;
    void SetAnimationTime(std::uint32_t animation_address,
                          float time) override;
    std::uint32_t AnimationFlag(std::uint32_t animation_address) override;

private:
    BehaviorPathEndpointRuntimeStub endpoint_{};
};

class MonsterPathEndpointRestore final {
public:
    // IDA 0x435530: restore class-ID-zero monster path endpoint state.
    static void Accept(MonsterPathEndpointTargetState& target,
                       const MonsterBehaviorManagerState& manager,
                       BehaviorPathEndpointState& helper,
                       std::uint32_t helper_address,
                       IMonsterPathEndpointRestoreRuntime& runtime);
};

class MonsterPathEndpointC03Restore final {
public:
    // IDA 0x435f20: restore the class-ID-one path endpoint target.
    static void Accept(MonsterPathEndpointTargetState& target,
                       const MonsterBehaviorManagerState& manager,
                       BehaviorPathEndpointState& helper,
                       std::uint32_t helper_address,
                       IMonsterPathEndpointRestoreRuntime& runtime);
};

}  // namespace pal4
