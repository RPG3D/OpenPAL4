#pragma once

#include "pal4/event_subscription_set.h"
#include "pal4/use_property_task.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatPropertyActionCreationRuntime;

struct PropertyActionHandlerBinding {
    std::uint32_t key_address;
    std::uint32_t callback;
};

class PropertyActionHandlers final {
public:
    static constexpr std::array<PropertyActionHandlerBinding, 2> kBindings{{
        {0x8b8b64u, 0x55da30u},
        {0x8b5940u, 0x55d880u},
    }};

    // IDA 0x55d410; cleanup is the shared event-list operation.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task,
                      IEventSubscriptionRuntime& runtime);
};

class IPropertyActionTaskRuntime {
public:
    virtual ~IPropertyActionTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void ResetEffect(std::uint32_t address) = 0;
    virtual void DestroyEmbedded(std::uint32_t address) = 0;
    virtual void DestroyBase(std::uint32_t task) = 0;
    virtual void StartAction(std::uint32_t task) = 0;
    virtual void StopAction(std::uint32_t task,
                            std::uint32_t frame_context) = 0;
    [[nodiscard]] virtual bool IsActionPending(std::uint32_t task) = 0;
    virtual void ResetEmbedded(std::uint32_t address) = 0;
    virtual void ProcessAnimationCommand(
        std::uint32_t task, std::string_view original_name) = 0;
    [[nodiscard]] virtual int CleanupCommand(
        std::string_view lowercase_name) = 0;
    [[nodiscard]] virtual std::uint32_t TargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorReady(std::uint32_t actor) = 0;
    virtual void ActorSetState(std::uint32_t actor,
                               std::uint32_t state) = 0;
};

class IPropertyActionEventRuntime {
public:
    virtual ~IPropertyActionEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual PropertyTaskEffectHandle CopyEffectPair(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view EffectName(
        const PropertyTaskEffectHandle& effect) = 0;
    [[nodiscard]] virtual int ResetEffectPair(
        PropertyTaskEffectHandle& effect) = 0;
    [[nodiscard]] virtual std::uint32_t EffectOwner(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t PrepareEffectCompletion(
        const PropertyTaskEffectHandle& effect) = 0;
};

class PropertyActionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8457ccu;
    static constexpr std::uint32_t kSize = 324u;

    // IDA 0x56afa0.
    [[nodiscard]] static std::uint32_t Create(
        ICombatPropertyActionCreationRuntime& runtime);

    // IDA 0x55d5d0-0x55d850.
    static void Destroy(std::uint32_t task,
                        IPropertyActionTaskRuntime& runtime,
                        IEventSubscriptionRuntime& events);
    static void CleanupEmbedded(std::uint32_t embedded,
                                IPropertyActionTaskRuntime& runtime);
    [[nodiscard]] static int ProcessCommand(
        std::uint32_t task, std::string_view message_name,
        IPropertyActionTaskRuntime& runtime,
        IUsePropertyTaskRuntime& property);
    [[nodiscard]] static bool CheckStatus(
        std::uint32_t task, IPropertyActionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IPropertyActionTaskRuntime& runtime,
        IEventSubscriptionRuntime& events);
    [[nodiscard]] static std::uint8_t Stop(
        std::uint32_t task, std::uint32_t frame_context,
        IPropertyActionTaskRuntime& runtime,
        IEventSubscriptionRuntime& events);
    // IDA 0x55d880-0x55db20.
    [[nodiscard]] static std::uint32_t HandleEffectEvent(
        std::uint32_t task, std::uint32_t event,
        IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static PropertyTaskEffectHandle CopyEventEffect(
        std::uint32_t event, IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleCompletionEvent(
        std::uint32_t task, std::uint32_t event,
        IPropertyActionEventRuntime& runtime);
    [[nodiscard]] static int HandleAnimationEvent(
        std::uint32_t task, PropertyTaskEffectHandle effect,
        std::string_view event_name,
        IPropertyActionEventRuntime& runtime);
};

}  // namespace pal4
