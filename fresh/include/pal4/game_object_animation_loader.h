#pragma once

#include "pal4/game_object_embedded_animation.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameObjectAnimationLoadRuntime
    : public IGameObjectEmbeddedAnimationRuntime {
public:
    virtual ~IGameObjectAnimationLoadRuntime() = default;
    [[nodiscard]] virtual bool LoadScript(
        GameObjectEmbeddedAnimationState& state,
        std::string_view path) = 0;
    [[nodiscard]] virtual std::int32_t AnimationCount(
        GameObjectEmbeddedAnimationState& state) = 0;
    [[nodiscard]] virtual std::string_view AnimationResourceSuffix(
        GameObjectEmbeddedAnimationState& state,
        std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateRecords(
        std::uint32_t bytes) = 0;
    virtual void ConstructRecords(
        std::uint32_t first_record, std::uint32_t count,
        std::uint32_t stride) = 0;
    virtual void SetRecordName(
        std::uint32_t record, std::string_view name) = 0;
    virtual void SetRecordBasePath(
        std::uint32_t record, std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t ReadAnimation(
        std::string_view path) = 0;
    virtual void SetRecordAnimation(
        std::uint32_t record, std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::uint32_t RecordAnimation(
        std::uint32_t record) = 0;
    virtual void LoadAnimationEvents(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t record) = 0;
    [[nodiscard]] virtual bool HasHierarchy(
        std::uint32_t clump) = 0;
    virtual void SetHierarchyFlags(
        std::uint32_t hierarchy, std::uint32_t flags) = 0;
    virtual void AttachHierarchy(std::uint32_t hierarchy) = 0;
    virtual void AttachSkinAtomics(
        std::uint32_t clump, std::uint32_t hierarchy) = 0;
    virtual void InitializeHierarchyAnimationData(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t hierarchy) = 0;
    [[nodiscard]] virtual std::uint32_t HierarchyInterpolator(
        std::uint32_t hierarchy) = 0;
    virtual void SetCurrentAnimation(
        std::uint32_t interpolator, std::uint32_t animation) = 0;
    virtual void SetCurrentTime(
        std::uint32_t interpolator, float time) = 0;
    virtual void UpdateMatrices(std::uint32_t hierarchy) = 0;
    [[nodiscard]] virtual std::uint32_t HierarchyFrameCount(
        std::uint32_t hierarchy) = 0;
    [[nodiscard]] virtual std::uint32_t InterpolatorFrameData(
        std::uint32_t interpolator) = 0;
    [[nodiscard]] virtual std::uint32_t CloneBlendSnapshot(
        std::uint32_t frame_count,
        std::uint32_t frame_data) = 0;
};

class GameObjectAnimationLoader final {
public:
    static constexpr std::uint32_t kRecordStride = 0x44u;

    [[nodiscard]] static bool LoadData(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t clump, std::string_view base_path,
        std::string_view table_suffix,
        IGameObjectAnimationLoadRuntime& runtime);
};

}  // namespace pal4
