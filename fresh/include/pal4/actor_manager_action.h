#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerChildRuntime {
public:
    virtual ~IActorManagerChildRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IActorManagerAnimationQueryRuntime : public IActorManagerChildRuntime {
public:
    [[nodiscard]] virtual std::uint32_t GetAnimationFlag(std::uint32_t state) = 0;
    [[nodiscard]] virtual std::string_view GetAnimationName(std::uint32_t state) = 0;
};

class IActorManagerActionRuntime : public IActorManagerAnimationQueryRuntime {
public:
    [[nodiscard]] virtual bool FindAnimation(
        std::uint32_t child, std::string_view name) = 0;
    virtual void LoadMissingAnimation(std::uint32_t actor) = 0;
    virtual void SetAttachmentActive(std::uint32_t child,
                                     bool right_hand, std::uint8_t active,
                                     std::uint32_t effect_context) = 0;
    virtual void SetAnimationFlag(std::uint32_t state,
                                  std::uint8_t flag) = 0;
    virtual void SetAnimationName(std::uint32_t state,
                                  std::string_view name) = 0;
    virtual void TouchSkillSystem() = 0;
    virtual void InvokeChildSlot4(std::uint32_t child,
                                  std::uint32_t first,
                                  std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeChildSlot8(
        std::uint32_t child, std::uint32_t first,
        std::uint32_t second) = 0;
};

class ActorManagerAction final {
public:
    static constexpr std::uint32_t kEmptyTransform = 0x8b5c58u;
    static constexpr std::uint32_t kEmptyBoundingBox = 0x8f60b0u;

    static void Play(std::uint32_t actor, std::string_view action,
                     std::uint8_t flag, IActorManagerActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Position(
        std::uint32_t actor, IActorManagerActionRuntime& runtime);
    [[nodiscard]] static bool InvokeRotation(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, IActorManagerActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t InvokeScale(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, IActorManagerActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Matrix(
        std::uint32_t actor, IActorManagerActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t BoundingBox(
        std::uint32_t actor, IActorManagerActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clump(
        std::uint32_t actor, IActorManagerChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t AnimationFlag(
        std::uint32_t actor, IActorManagerAnimationQueryRuntime& runtime);
    [[nodiscard]] static std::string_view AnimationName(
        std::uint32_t actor, IActorManagerAnimationQueryRuntime& runtime);
};

}  // namespace pal4
