#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

class IActorManagerAttackEventRuntime {
public:
    virtual ~IActorManagerAttackEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void Dispatch(std::string_view event,
                          std::span<const std::uint32_t> payload,
                          std::uint32_t mode) = 0;
    virtual void InvokeProperty(std::uint32_t property,
                                std::uint32_t virtual_slot,
                                std::uint32_t actor,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
};

class ActorManagerAttackEvents final {
public:
    static void PhysicalAttack(std::uint32_t actor, std::uint32_t target,
                               IActorManagerAttackEventRuntime& runtime);
    static void CriticalAttack(std::uint32_t actor, std::uint32_t target,
                               IActorManagerAttackEventRuntime& runtime);
    static void FendOffed(std::uint32_t actor, std::uint32_t target,
                          IActorManagerAttackEventRuntime& runtime);
    static void AttackMissed(std::uint32_t actor, std::uint32_t target,
                             IActorManagerAttackEventRuntime& runtime);
    static void CastMagic(std::uint32_t actor, std::uint32_t target,
                          std::uint32_t magic,
                          IActorManagerAttackEventRuntime& runtime);
    static void Stunt(std::uint32_t actor, std::uint32_t target,
                      std::uint32_t stunt,
                      IActorManagerAttackEventRuntime& runtime);
    static void UseProperty(std::uint32_t actor, std::uint32_t target,
                            std::uint32_t property,
                            IActorManagerAttackEventRuntime& runtime);
};

}  // namespace pal4
