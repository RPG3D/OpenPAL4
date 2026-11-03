#pragma once

#include "pal4/game_object_embedded_animation.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameObjectAnimationSwitchRuntime {
public:
    virtual ~IGameObjectAnimationSwitchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindHierarchy(
        std::uint32_t clump) = 0;
    virtual void SetHierarchyFlags(
        std::uint32_t hierarchy, std::uint32_t flags) = 0;
    virtual void AttachHierarchy(std::uint32_t hierarchy) = 0;
    virtual void AttachSkinAtomics(
        std::uint32_t clump, std::uint32_t hierarchy) = 0;
    [[nodiscard]] virtual std::int32_t AnimationCount(
        GameObjectEmbeddedAnimationState& state) = 0;
    [[nodiscard]] virtual std::string_view ActionName(
        GameObjectEmbeddedAnimationState& state,
        std::int32_t index) = 0;
    [[nodiscard]] virtual int CompareActionNames(
        std::string_view stored, std::string_view requested) = 0;
    virtual void CleanupCurrentRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t RecordAnimation(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t HierarchyInterpolator(
        std::uint32_t hierarchy) = 0;
    virtual void SetCurrentAnimation(
        std::uint32_t interpolator,
        std::uint32_t animation) = 0;
    virtual void SetCurrentTime(
        std::uint32_t interpolator, float time) = 0;
    virtual void UpdateMatrices(std::uint32_t hierarchy) = 0;
};

class GameObjectAnimationSwitch final {
public:
    static constexpr std::uint32_t kRecordStride = 0x44u;

    [[nodiscard]] static bool AttachHierarchyToClump(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t target_clump,
        IGameObjectAnimationSwitchRuntime& runtime);
    [[nodiscard]] static bool SetCurrentAnimation(
        GameObjectEmbeddedAnimationState& state,
        std::string_view name,
        IGameObjectAnimationSwitchRuntime& runtime);
    [[nodiscard]] static bool SetCurrentAnimationBlended(
        GameObjectEmbeddedAnimationState& state,
        std::string_view name, float blend_time,
        IGameObjectAnimationSwitchRuntime& runtime);
};

}  // namespace pal4
