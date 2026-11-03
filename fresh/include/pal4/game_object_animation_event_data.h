#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace pal4 {

using AnimationEventData = std::array<std::byte, 0x14c>;

class IGameObjectAnimationEventStringRuntime {
public:
    virtual ~IGameObjectAnimationEventStringRuntime() = default;
    virtual void CopyStringField(
        std::span<std::byte, 64> destination,
        std::span<const std::byte, 64> source) = 0;
};

class IGameObjectAnimationEventMemory {
public:
    virtual ~IGameObjectAnimationEventMemory() = default;
    [[nodiscard]] virtual AnimationEventData ReadEvent(
        std::uint32_t address) = 0;
    virtual void WriteEvent(
        std::uint32_t address, const AnimationEventData& event) = 0;
};

class GameObjectAnimationEventData final {
public:
    [[nodiscard]] static AnimationEventData& Reset(
        AnimationEventData& event) noexcept;
    [[nodiscard]] static std::uint32_t NormalizeTime(
        AnimationEventData& event) noexcept;
    [[nodiscard]] static AnimationEventData& Copy(
        AnimationEventData& destination, AnimationEventData& scratch,
        const AnimationEventData& source,
        IGameObjectAnimationEventStringRuntime& runtime);
};

}  // namespace pal4
