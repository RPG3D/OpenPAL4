#pragma once

#include "pal4/event_subscription_set.h"

#include <cstdint>

namespace pal4 {

// Message delivery does not need the legacy string/container destruction ABI.
class ICombatProcessMessageDispatchRuntime {
public:
    virtual ~ICombatProcessMessageDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetDialogData(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchAnimationMessage(
        std::uint32_t task, std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchEffectMessage(
        std::uint32_t task, std::uint32_t first,
        std::uint32_t second, std::uint32_t payload) = 0;
};

class ICombatProcessMessageTaskRuntime : public IEventSubscriptionRuntime,
                                       public ICombatProcessMessageDispatchRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ResetEffect(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t GetDialogData(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchAnimationMessage(
        std::uint32_t task, std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchEffectMessage(
        std::uint32_t task, std::uint32_t first,
        std::uint32_t second, std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t ResetTemporaryEffect(
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third) = 0;
};

class ICombatProcessMessageFlowRuntime {
public:
    virtual ~ICombatProcessMessageFlowRuntime() = default;
    virtual void RegisterSubscriptions(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessAction(std::uint32_t task) = 0;
    virtual void ClearSubscriptions(std::uint32_t task) = 0;
    virtual void HandleEffectEvent(std::uint32_t slot,
                                   std::uint32_t context, bool active) = 0;
    [[nodiscard]] virtual bool IsAnimationComplete(std::uint32_t task) = 0;
    [[nodiscard]] virtual bool EffectSlotActive(std::uint32_t slot) = 0;
    virtual void CallActorCommand(std::uint32_t actor, std::uint32_t first,
                                  std::uint32_t second,
                                  std::uint32_t fourth) = 0;
};

class CombatProcessMessageTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8456c4u;

    // IDA 0x556a20-0x556b80.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t task, ICombatProcessMessageTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetTemporaryEffect(
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third, ICombatProcessMessageTaskRuntime& runtime);
    static void DestroyEffectSlot(std::uint32_t slot,
                                  ICombatProcessMessageTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleAnimationMessage(
        std::uint32_t task, std::uint32_t event,
        ICombatProcessMessageDispatchRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEffectMessage(
        std::uint32_t task, std::uint32_t event,
        ICombatProcessMessageDispatchRuntime& runtime);

    // IDA 0x5570d0-0x557150.
    [[nodiscard]] static std::uint8_t StartAction(
        std::uint32_t task, ICombatProcessMessageFlowRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopAction(
        std::uint32_t task, std::uint32_t context,
        ICombatProcessMessageFlowRuntime& runtime);
    [[nodiscard]] static bool IsActionComplete(
        std::uint32_t task, ICombatProcessMessageFlowRuntime& runtime);
    [[nodiscard]] static std::uint32_t CallGameObjectCommand(
        std::uint32_t first, std::uint32_t second, std::uint32_t actor,
        std::uint32_t fourth, ICombatProcessMessageFlowRuntime& runtime);
};

}  // namespace pal4
