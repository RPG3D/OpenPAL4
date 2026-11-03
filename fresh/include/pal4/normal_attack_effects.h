#pragma once

#include <cstdint>

namespace pal4 {

enum class NormalAttackDamageKind : std::uint8_t {
    Physical,
    Magic,
};

struct NormalAttackDamage {
    float target = 0.0f;
    float attacker = 0.0f;
};

class INormalAttackEffectRuntime {
public:
    virtual ~INormalAttackEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual NormalAttackDamage CalculateDamage(
        std::uint32_t attacker, std::uint32_t target, std::uint8_t mode,
        NormalAttackDamageKind kind) = 0;
    virtual void NotifyAttacker(std::uint32_t attacker,
                                std::uint32_t target,
                                NormalAttackDamageKind kind) = 0;
    virtual void NotifyTarget(std::uint32_t target,
                              std::uint32_t attacker) = 0;
    [[nodiscard]] virtual std::int32_t Value7(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool HasProperty12(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    virtual void AddValue7(std::uint32_t actor, std::int32_t value) = 0;
    [[nodiscard]] virtual bool BlocksReflectedDamage(
        std::uint32_t attacker) = 0;
    virtual void TransferScaledMetric(std::uint32_t attacker,
                                      std::uint32_t target, float scale) = 0;
    virtual void NotifyCombatState(std::uint32_t actor,
                                   std::uint32_t code,
                                   std::uint32_t other) = 0;
    [[nodiscard]] virtual std::uint8_t CombatWounded(
        std::uint32_t target, std::uint32_t attacker) = 0;
    [[nodiscard]] virtual std::uint8_t UnchangedTarget(
        std::uint32_t target, std::uint32_t attacker) = 0;
    [[nodiscard]] virtual std::uint32_t TargetState(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint8_t RegisterFendOff(
        std::uint32_t attacker, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint8_t RegisterMiss(
        std::uint32_t attacker, std::uint32_t target) = 0;
};

class NormalAttackEffects final {
public:
    // IDA 0x557bd0, 0x557d20, 0x557e70 and 0x557ed0.
    [[nodiscard]] static std::uint8_t Attack(
        std::uint32_t task, std::uint32_t target,
        INormalAttackEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Magic(
        std::uint32_t task, std::uint32_t target,
        INormalAttackEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Item(
        std::uint32_t task, std::uint32_t target,
        INormalAttackEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Support(
        std::uint32_t task, std::uint32_t target,
        INormalAttackEffectRuntime& runtime);
};

}  // namespace pal4
