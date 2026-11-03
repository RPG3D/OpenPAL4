#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

struct ActorManagerVec3 {
    float x{};
    float y{};
    float z{};
};

class IActorManagerCombatMotionRuntime {
public:
    virtual ~IActorManagerCombatMotionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot444(
        std::uint32_t actor, std::uint32_t argument) = 0;
    virtual void ChildSlot4(std::uint32_t child, std::uint32_t value) = 0;
    virtual void Normalize(ActorManagerVec3& vector) = 0;
    [[nodiscard]] virtual ActorManagerVec3 ChildPosition(std::uint32_t actor) = 0;
    [[nodiscard]] virtual double Acos(float cosine) = 0;
    [[nodiscard]] virtual std::uint32_t ChildSlot8(
        std::uint32_t actor, const ActorManagerVec3& angle,
        std::uint32_t mode) = 0;
    virtual void ActorSlot564(std::uint32_t actor,
                              std::uint32_t global, std::uint32_t mode) = 0;
    virtual void ActorSlot396(std::uint32_t actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot464(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorActor(std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DeleteIterator(std::uint32_t iterator, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::optional<std::string_view>
    GlobalAnimationName() = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t child_animation) = 0;
    [[nodiscard]] virtual bool EqualIgnoreCase(
        std::string_view first, std::string_view second) = 0;
};

class ActorManagerCombatActorMotion final {
public:
    static constexpr std::uint32_t kGlobalMotion = 0x8fa3a0u;

    static void Update(std::uint32_t actor, std::uint32_t flag,
                       IActorManagerCombatMotionRuntime& runtime);
    [[nodiscard]] static std::uint32_t StartAnimation(
        std::uint32_t actor, IActorManagerCombatMotionRuntime& runtime);
};

}  // namespace pal4
