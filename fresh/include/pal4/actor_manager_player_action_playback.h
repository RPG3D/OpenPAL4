#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerActionRuntime {
public:
    virtual ~IActorManagerPlayerActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t animation) = 0;
    [[nodiscard]] virtual bool HasActionAnimation(
        std::uint32_t animation, std::string_view filename) = 0;
    virtual void PlayAction(std::uint32_t actor, std::string_view action,
                            std::uint8_t flags) = 0;
    virtual void TouchSkillSystem() = 0;
    virtual void TouchGlobalSystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindOrLoad(
        std::uint32_t actor, std::string_view animation_name) = 0;
    virtual void GetFrame(std::uint32_t animation) = 0;
    virtual void CleanupAnimation(std::uint32_t animation,
                                  std::uint32_t actor) = 0;
    virtual void SetTransformFromActor(std::uint32_t animation,
                                       std::uint32_t transform,
                                       std::uint32_t flags) = 0;
    virtual void SetTransformFromPosition(std::uint32_t animation,
                                          std::uint32_t position,
                                          std::uint32_t flags) = 0;
    virtual void SetLeftHandActive(std::uint32_t animation, bool active) = 0;
    virtual void SetRightHandActive(std::uint32_t animation, bool active) = 0;
    virtual void UpdateAttachments(std::uint32_t actor,
                                   std::uint32_t animation) = 0;
    virtual void DispatchRejoinScene(std::uint32_t actor) = 0;
};

class ActorManagerPlayerActionPlayback final {
public:
    static constexpr std::uint32_t kEmptyName = 0x841964u;

    [[nodiscard]] static std::uint8_t Play(
        std::uint32_t actor, std::string_view action, std::uint8_t flags,
        IActorManagerPlayerActionRuntime& runtime);
};

}  // namespace pal4
