#pragma once

#include "pal4/game_object_embedded_animation.h"

#include <cstdint>

namespace pal4 {

class IGameObjectAnimationEventDispatchRuntime {
public:
    virtual ~IGameObjectAnimationEventDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EventBegin(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t EventEnd(std::uint32_t record) = 0;
    [[nodiscard]] virtual float TriggerTime(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t FiredFlag(std::uint32_t event) = 0;
    virtual void SetFiredFlag(std::uint32_t event, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ListenerAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool MatchesMessage(
        std::uint32_t listener, std::uint32_t message_name) = 0;
    virtual void Dispatch(std::uint32_t listener,
                          GameObjectEmbeddedAnimationState& state,
                          std::uint32_t event) = 0;
};

class GameObjectAnimationEventDispatcher final {
public:
    static constexpr std::uint32_t kEventStride = 0x14cu;

    [[nodiscard]] static bool UpdateTime(
        GameObjectEmbeddedAnimationState& state, float time,
        IGameObjectAnimationEventDispatchRuntime& runtime);
};

}  // namespace pal4
