#pragma once

#include "pal4/game_object_embedded_animation.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class IGameObjectAnimationQueryRuntime {
public:
    virtual ~IGameObjectAnimationQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SelectedObjectName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyErrorString() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyLogMessage() = 0;
    [[nodiscard]] virtual std::uint32_t RecordName(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::int32_t AnimationCount(
        GameObjectEmbeddedAnimationState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AnimationEntry(
        GameObjectEmbeddedAnimationState& state,
        std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t EventBegin(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t EventEnd(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual int CompareEventName(
        std::uint32_t stored_name,
        std::uint32_t requested_name) = 0;
};

using AnimationVisit = std::function<bool(
    std::uint32_t entry, std::int32_t index,
    std::uint32_t context)>;

class GameObjectAnimationQuery final {
public:
    static constexpr std::uint32_t kEventStride = 0x14cu;

    [[nodiscard]] static std::uint32_t CurrentAnimationData(
        const GameObjectEmbeddedAnimationState& state) noexcept;
    [[nodiscard]] static std::uint32_t SelectedAnimationName(
        const GameObjectEmbeddedAnimationState& state,
        IGameObjectAnimationQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CurrentRecordName(
        const GameObjectEmbeddedAnimationState& state,
        IGameObjectAnimationQueryRuntime& runtime);
    [[nodiscard]] static bool ProcessAnimations(
        GameObjectEmbeddedAnimationState& state,
        const AnimationVisit& callback, std::uint32_t context,
        IGameObjectAnimationQueryRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindEvent(
        GameObjectEmbeddedAnimationState& state,
        std::uint32_t requested_name,
        IGameObjectAnimationQueryRuntime& runtime);
};

}  // namespace pal4
