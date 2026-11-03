#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerSceneRuntime {
public:
    virtual ~IActorManagerPlayerSceneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveAnimationTransform(
        std::uint32_t actor, std::uint32_t mode) = 0;
    virtual void SetAnimationTransform(std::uint32_t animation,
                                       std::uint32_t transform) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t MoveChild(
        std::uint32_t actor, const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
    virtual std::int32_t QueryCurrentStat(std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SecondaryState(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t PrimaryState(std::uint32_t actor) = 0;
    virtual void SelectAnimation(std::uint32_t actor,
                                 std::uint32_t descriptor,
                                 std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t CreateActorIterator(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IteratorDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceIterator(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::string_view MatchAnimationName() = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t animation) = 0;
    virtual void ActivateActor(std::uint32_t actor,
                               std::uint32_t active) = 0;
};

class ActorManagerPlayerSceneTransition final {
public:
    static constexpr std::uint32_t kPrimaryAnimation = 0x8fa34cu;
    static constexpr std::uint32_t kSecondaryAnimation = 0x8fa33cu;
    static constexpr std::uint32_t kDefaultAnimation = 0x8fa3a0u;

    static void Update(std::uint32_t actor, std::uint32_t flags,
                       IActorManagerPlayerSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t Begin(
        std::uint32_t actor, IActorManagerPlayerSceneRuntime& runtime);
};

}  // namespace pal4
