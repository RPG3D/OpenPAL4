#pragma once

#include "pal4/event_subscription_set.h"
#include "pal4/property_action_task.h"
#include "pal4/throw_property_action_task.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatPropertyActionCreationRuntime;

struct CastPropertyActionHandlerBinding {
    std::uint32_t key_address;
    std::uint32_t callback;
};

class CastPropertyActionHandlers final {
public:
    static constexpr std::array<CastPropertyActionHandlerBinding, 2>
        kBindings{{
            {0x8b8b64u, 0x55f050u},
            {0x8b5940u, 0x55eec0u},
        }};

    // IDA 0x55e9e0; cleanup uses the shared two-slot routine.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task,
                      IEventSubscriptionRuntime& runtime);
};

class ICastPropertyActionRuntime {
public:
    virtual ~ICastPropertyActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address,
                            float value) = 0;
    virtual void ResetEffect(std::uint32_t address) = 0;
    virtual void DestroyEmbedded(std::uint32_t address) = 0;
    virtual void DestroyBase(std::uint32_t task) = 0;
    virtual void StartAction(std::uint32_t task) = 0;
    virtual void StopAction(std::uint32_t task,
                            std::uint32_t frame_context) = 0;
    virtual void ResetEmbedded(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsActionPending(
        std::uint32_t task) = 0;
    virtual void ActorSetState(std::uint32_t actor,
                               std::uint32_t state) = 0;
    [[nodiscard]] virtual PropertyTaskEffectHandle CreateActorEffect(
        std::uint32_t actor, std::string_view effect_name,
        std::uint32_t hook_address) = 0;
    virtual void ResetTemporaryEffect(
        PropertyTaskEffectHandle& effect) = 0;
    virtual void ProcessAnimationCommand(
        std::uint32_t task, std::string_view original_name) = 0;
    [[nodiscard]] virtual int CleanupCommand(
        std::string_view lowercase_name) = 0;
};

class CastPropertyActionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84583cu;
    static constexpr std::uint32_t kSize = 328u;
    static constexpr std::uint32_t kEffectHook = 0x8fa284u;

    // IDA 0x56ad70.
    [[nodiscard]] static std::uint32_t Create(
        ICombatPropertyActionCreationRuntime& runtime);

    // IDA 0x55eba0-0x55f140.
    static void Destroy(std::uint32_t task,
                        ICastPropertyActionRuntime& runtime,
                        IEventSubscriptionRuntime& events);
    [[nodiscard]] static int ProcessCommand(
        std::uint32_t task, std::string_view message_name,
        ICastPropertyActionRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t task, float delta,
        ICastPropertyActionRuntime& runtime,
        IUsePropertyTaskRuntime& property_runtime);
    [[nodiscard]] static bool CheckStatus(
        std::uint32_t task, ICastPropertyActionRuntime& runtime,
        IUsePropertyTaskRuntime& property_runtime);
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, ICastPropertyActionRuntime& runtime,
        IEventSubscriptionRuntime& events);
    [[nodiscard]] static std::uint8_t Stop(
        std::uint32_t task, std::uint32_t frame_context,
        ICastPropertyActionRuntime& runtime,
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
