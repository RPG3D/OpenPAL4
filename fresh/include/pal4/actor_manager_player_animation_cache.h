#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerAnimationRuntime {
public:
    virtual ~IActorManagerPlayerAnimationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t LoadAnimation(
        std::uint32_t collection, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePointerArray(
        std::uint32_t bytes) = 0;
    virtual void FreePointerArray(std::uint32_t address) = 0;
    virtual void GetFrame(std::uint32_t animation) = 0;
    virtual void RenderCamera(std::uint32_t animation,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    virtual void CleanupAnimation(std::uint32_t animation,
                                  std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessAnimationEvent(
        std::uint32_t actor, std::uint32_t animation) = 0;
    virtual void SetTransformFromActor(std::uint32_t animation,
                                       std::uint32_t old_actor_transform,
                                       std::uint32_t flags) = 0;
    virtual void SetTransformFromPosition(std::uint32_t animation,
                                          std::uint32_t old_position,
                                          std::uint32_t flags) = 0;
    virtual void SetLeftHandActive(std::uint32_t animation, bool active) = 0;
    virtual void SetRightHandActive(std::uint32_t animation, bool active) = 0;
    virtual void DispatchRejoinScene(std::uint32_t actor) = 0;
};

class ActorManagerPlayerAnimationCache final {
public:
    static constexpr std::uint32_t kEmptyName = 0x841964u;

    [[nodiscard]] static std::uint32_t Activate(
        std::uint32_t actor, IActorManagerPlayerAnimationRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindOrLoad(
        std::uint32_t actor, std::uint32_t name,
        IActorManagerPlayerAnimationRuntime& runtime);
    static void SwitchToRequestedAnimation(
        std::uint32_t actor, IActorManagerPlayerAnimationRuntime& runtime);
};

}  // namespace pal4
