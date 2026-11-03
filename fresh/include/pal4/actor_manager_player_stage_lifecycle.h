#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerStageRuntime {
public:
    virtual ~IActorManagerPlayerStageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void PrepareStage(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveAnimationTransform(
        std::uint32_t actor, std::uint32_t mode,
        std::uint32_t context) = 0;
    virtual void SetAnimationTransform(std::uint32_t animation,
                                       std::uint32_t transform) = 0;
    [[nodiscard]] virtual std::uint32_t SendEvent(
        std::string_view name, std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    virtual void ReturnAnimationToPool(std::uint32_t collection,
                                       std::uint32_t limit,
                                       std::uint32_t animation) = 0;
};

class ActorManagerPlayerStageLifecycle final {
public:
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t actor, std::uint32_t context,
        IActorManagerPlayerStageRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t actor, IActorManagerPlayerStageRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveAndReturnAnimations(
        std::uint32_t actor, IActorManagerPlayerStageRuntime& runtime);
};

}  // namespace pal4
