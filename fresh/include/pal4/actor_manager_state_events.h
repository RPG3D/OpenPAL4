#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IActorManagerPairEventRuntime {
public:
    virtual ~IActorManagerPairEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::string_view event, std::span<const std::uint32_t> payload,
        std::uint32_t mode) = 0;
};

class IActorManagerStateEventRuntime : public IActorManagerPairEventRuntime {
public:
    virtual ~IActorManagerStateEventRuntime() = default;
    virtual void TouchSkillSystem(std::uint32_t actor,
                                  std::uint32_t state) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeProperty(
        std::uint32_t property, std::uint32_t slot,
        std::uint32_t actor) = 0;
    virtual void ResetObjectProperties(std::uint32_t actor) = 0;
    virtual void InvokeActorSlot(std::uint32_t actor,
                                 std::uint32_t slot) = 0;
};

class ActorManagerStateEvents final {
public:
    [[nodiscard]] static std::uint8_t SetState(
        std::uint32_t actor, std::uint32_t state, bool notify,
        IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t BeginFlee(
        std::uint32_t actor, IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t Fled(
        std::uint32_t actor, IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t FleeFailed(
        std::uint32_t actor, IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t Defend(
        std::uint32_t actor, IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t BecomeDead(
        std::uint32_t actor, std::uint32_t cause,
        IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t BecomeAlive(
        std::uint32_t actor, IActorManagerStateEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProtectAction(
        std::uint32_t actor, std::uint32_t protected_actor,
        std::uint32_t other,
        IActorManagerPairEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t JointAttack(
        std::uint32_t actor, std::uint32_t other,
        IActorManagerPairEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReactAttack(
        std::uint32_t actor, std::uint32_t other,
        IActorManagerPairEventRuntime& runtime);
};

}  // namespace pal4
