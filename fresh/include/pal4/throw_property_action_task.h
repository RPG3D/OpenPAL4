#pragma once

#include "pal4/event_subscription_set.h"
#include "pal4/property_action_task.h"
#include "pal4/throw_property_task.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatPropertyActionCreationRuntime;

struct ThrowPropertyActionHandlerBinding {
    std::uint32_t key_address;
    std::uint32_t callback;
};

class ThrowPropertyActionHandlers final {
public:
    static constexpr std::array<ThrowPropertyActionHandlerBinding, 2>
        kBindings{{
            {0x8b8b64u, 0x55e7b0u},
            {0x8b5940u, 0x55e620u},
        }};

    // IDA 0x55e170/0x55e1d0.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task,
                      IEventSubscriptionRuntime& runtime);
};

class IThrowPropertyActionRuntime {
public:
    virtual ~IThrowPropertyActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void ResetEffect(std::uint32_t address) = 0;
    virtual void DestroyEmbeddedProperty(
        std::uint32_t address) = 0;
    virtual void DestroyBase(std::uint32_t task) = 0;
    virtual void StartAction(std::uint32_t task) = 0;
    [[nodiscard]] virtual bool IsActionPending(
        std::uint32_t task) = 0;
    virtual void ActorSetState(std::uint32_t actor,
                               std::uint32_t state) = 0;
    virtual void AccumulateElapsed(float delta) = 0;
    [[nodiscard]] virtual bool ThrowComplete(
        std::uint32_t embedded) = 0;
    virtual void ProcessAnimationCommand(
        std::uint32_t task, std::string_view original_name) = 0;
    [[nodiscard]] virtual int CleanupCommand(
        std::string_view lowercase_name) = 0;
};

class ThrowPropertyActionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845814u;
    static constexpr std::uint32_t kSize = 412u;

    // IDA 0x56ae80.
    [[nodiscard]] static std::uint32_t Create(
        ICombatPropertyActionCreationRuntime& runtime);

    // IDA 0x55e3c0-0x55e8a0.
    static void Destroy(std::uint32_t task,
                        IThrowPropertyActionRuntime& runtime,
                        IEventSubscriptionRuntime& events,
                        IThrowPropertyTaskRuntime& throw_runtime);
    [[nodiscard]] static int ProcessCommand(
        std::uint32_t task, std::string_view message_name,
        IThrowPropertyActionRuntime& runtime,
        IThrowPropertyTaskRuntime& throw_runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t task, float delta,
        IThrowPropertyActionRuntime& runtime,
        IThrowPropertyTaskRuntime& throw_runtime,
        IUsePropertyTaskRuntime& property_runtime);
    [[nodiscard]] static bool CheckStatus(
        std::uint32_t task, IThrowPropertyActionRuntime& runtime,
        IUsePropertyTaskRuntime& property_runtime);
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, IThrowPropertyActionRuntime& runtime,
        IEventSubscriptionRuntime& events);
    [[nodiscard]] static std::uint32_t HandleEffectEvent(
        std::uint32_t task, std::uint32_t event,
        IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleCompletionEvent(
        std::uint32_t task, std::uint32_t event,
        IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static int HandleAnimationEvent(
        std::uint32_t task, PropertyTaskEffectHandle effect,
        std::string_view event_name,
        IPropertyActionEventRuntime& runtime);
};

}  // namespace pal4
