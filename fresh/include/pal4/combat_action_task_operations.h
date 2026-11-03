#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatActionTaskRuntime {
public:
    virtual ~ICombatActionTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SkillSystem() = 0;
    [[nodiscard]] virtual std::uint32_t GlobalSystem() = 0;
    // Legacy ABI retains getter order. Native bindings need only the observable
    // prerequisites; the original Global_System getter/nullsub have no effects.
    virtual void PrepareActionServices() { (void)SkillSystem(); (void)GlobalSystem(); }
    virtual void SetActorTarget(std::uint32_t actor,
                                std::uint32_t target, std::uint8_t mode) = 0;
    virtual void StartActorAction(std::uint32_t actor,
                                  std::uint32_t target, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::string GetAnimationName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t GetAnimationFlag(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual int CompareInsensitive(
        std::string_view left, std::string_view right) = 0;
};

class CombatActionTaskOperations final {
public:
    // IDA 0x5562d0, 0x556450 and 0x5564d0.
    [[nodiscard]] static std::uint8_t SetTarget(
        std::uint32_t task, ICombatActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t ProcessAction(
        std::uint32_t task, ICombatActionTaskRuntime& runtime);
    [[nodiscard]] static bool IsAnimationComplete(
        std::uint32_t task, ICombatActionTaskRuntime& runtime);
};

class CombatProcessMessageSubscriptions final {
public:
    static constexpr std::uint32_t kAnimationKey = 0x8b595cu;
    static constexpr std::uint32_t kEffectKey = 0x8b8b60u;
    static constexpr std::uint32_t kAnimationCallback = 0x556b50u;
    static constexpr std::uint32_t kEffectCallback = 0x556b80u;

    // IDA 0x5567e0 and 0x556830.
    [[nodiscard]] static std::uint32_t Register(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task,
                      IEventSubscriptionRuntime& runtime);
};

}  // namespace pal4
