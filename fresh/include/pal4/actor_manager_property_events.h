#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IActorManagerPropertyEventDispatchRuntime {
public:
    virtual ~IActorManagerPropertyEventDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::string_view event, std::span<const std::uint32_t> payload,
        std::uint32_t mode) = 0;
};
class IActorManagerPropertyEventRuntime : public IActorManagerPropertyEventDispatchRuntime {
public:
    virtual ~IActorManagerPropertyEventRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void AddSkillNative(std::uint32_t actor,
                                std::uint32_t context,
                                std::uint32_t skill) = 0;
    virtual void RemoveSkillNative(std::uint32_t actor,
                                   std::uint32_t context,
                                   std::uint32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::string_view event, std::span<const std::uint32_t> payload,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CombatWounded(
        std::uint32_t actor, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterActorEvent(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeProperty(
        std::uint32_t property, std::uint32_t slot,
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third) = 0;
};

class ActorManagerPropertyEvents final {
public:
    [[nodiscard]] static std::uint32_t AddSkill(
        std::uint32_t actor, std::uint32_t context,
        std::uint32_t extra, std::uint32_t skill,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveSkill(
        std::uint32_t actor, std::uint32_t context,
        std::uint32_t skill,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ImmunityStateA(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ImmunityStateB(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ImmunitySkill(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ImmunityNormal(
        std::uint32_t actor, IActorManagerPropertyEventDispatchRuntime& runtime);
    [[nodiscard]] static std::uint32_t Wounded(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot96(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot100(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot104(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot108(
        std::uint32_t actor, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerPropertyEventRuntime& runtime);
};

}  // namespace pal4
