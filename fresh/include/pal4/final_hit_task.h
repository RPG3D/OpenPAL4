#pragma once

#include "pal4/dead_effect_task.h"
#include "pal4/event_subscription_set.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class FinalHitTaskEvents final {
public:
    static constexpr std::uint32_t kEventKey = 0x8b8b64u;
    static constexpr std::uint32_t kCallback = 0x55b640u;

    // IDA 0x55b1e0 and 0x55b210.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task,
                      IEventSubscriptionRuntime& runtime);
};

class IFinalHitTaskRuntime {
public:
    virtual ~IFinalHitTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadText(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t CombatCamera(
        std::uint32_t combat) = 0;
    [[nodiscard]] virtual DeadEffectHandle CreateEffect(
        std::uint32_t camera, std::string_view name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) = 0;
    virtual void ResetTemporaryEffect(DeadEffectHandle& effect) = 0;
    virtual void ResetOwnedEffect(std::uint32_t task_handle) = 0;
    [[nodiscard]] virtual std::uint32_t CreateText(
        std::string_view text) = 0;
    virtual void ReleaseText(std::uint32_t text) = 0;
    virtual void SetProperties(std::uint32_t task_handle,
                               std::uint32_t text,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ChildPosition(
        std::uint32_t actor) = 0;
    virtual void UpdatePosition(std::uint32_t task_handle,
                                std::uint32_t position,
                                std::uint8_t mode) = 0;
    virtual void FinalizeEffect(std::uint32_t task_handle) = 0;
    [[nodiscard]] virtual bool NeedsUpdate(
        std::uint32_t task_handle) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t task) = 0;
};

class FinalHitTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845788u;
    static constexpr std::uint32_t kEffectPosition = 0x84b1dcu;

    // IDA 0x55b3f0-0x55b640.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, IFinalHitTaskRuntime& runtime,
        IEventSubscriptionRuntime& events);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IFinalHitTaskRuntime& runtime,
        IEventSubscriptionRuntime& events);
    // The native router owns subscriptions separately; the effect sequence is
    // shared with the original subscription adapter.
    [[nodiscard]] static std::uint8_t ProcessEffect(
        std::uint32_t task, IFinalHitTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(
        std::uint32_t task, IEventSubscriptionRuntime& events);
    [[nodiscard]] static std::uint8_t CheckStatus(
        std::uint32_t task, IFinalHitTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEvent(
        std::uint32_t task, std::uint32_t event,
        IFinalHitTaskRuntime& runtime);
};

}  // namespace pal4
