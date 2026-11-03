#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IActorManagerSkillEventRuntime {
public:
    virtual ~IActorManagerSkillEventRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::string_view event, std::span<const std::uint32_t> payload,
        std::uint32_t mode) = 0;
};

class ActorManagerSkillEvents final {
public:
    [[nodiscard]] static std::uint32_t TriggerSkill(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth, IActorManagerSkillEventRuntime& runtime);
    static void EmptyFourArgumentCallback(std::uint32_t, std::uint32_t,
                                          std::uint32_t, std::uint32_t);
    [[nodiscard]] static std::uint32_t CallFunctionId(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerSkillEventRuntime& runtime);
    static void EmptyThreeArgumentCallback(std::uint32_t, std::uint32_t,
                                           std::uint32_t);
    [[nodiscard]] static std::uint32_t RemoveMutexSkill(
        std::uint32_t actor, std::uint32_t skill,
        IActorManagerSkillEventRuntime& runtime);
};

}  // namespace pal4
