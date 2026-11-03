#pragma once

#include "pal4/cast_magic_task.h"
#include "pal4/combat_effect_slot.h"
#include "pal4/event_subscription_set.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct CombatActionHandlerBinding {
    std::uint32_t key_address;
    std::uint32_t callback;
};

class CombatActionHandlers final {
public:
    static constexpr std::array<CombatActionHandlerBinding, 5> kBindings{{
        {0x8b8b64u, 0x559fd0u},
        {0x8b6310u, 0x55cab0u},
        {0x8b5fbcu, 0x55cab0u},
        {0x8b631cu, 0x55a100u},
        {0x8b5fc8u, 0x55a100u},
    }};

    // IDA 0x559120 and 0x5591f0.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task, IEventSubscriptionRuntime& runtime);
};

class IMagicActionTaskRuntime {
public:
    virtual ~IMagicActionTaskRuntime() = default;
    virtual void StartAction(std::uint32_t task) = 0;
    virtual void StopAction(std::uint32_t task,
                            std::uint32_t frame_context) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DestroyCastTask(std::uint32_t task) = 0;
    virtual void DestroyBase(std::uint32_t task) = 0;
    virtual void ProcessAttack(std::uint32_t task) = 0;
    virtual void ProcessAnimationCommand(
        std::uint32_t task, std::string_view original_name) = 0;
    [[nodiscard]] virtual int CleanupCommand(
        std::string_view lowercase_name) = 0;
};

class IMagicActionCreationRuntime : public ICombatEffectSlotRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyCombatBase(std::uint32_t object) = 0;
    virtual void DestroyMessageBase(std::uint32_t object) = 0;
    virtual void DestroyEmbeddedBase(std::uint32_t embedded) = 0;
    virtual void DestroyEffectSlot(std::uint32_t slot) = 0;
    virtual void SetTextErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
};

class IMagicActionProcessRuntime {
public:
    virtual ~IMagicActionProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view MagicName(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t MagicMode(
        std::uint32_t magic) = 0;
    [[nodiscard]] virtual bool ActorReady(std::uint32_t actor) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyEffect() = 0;
    [[nodiscard]] virtual std::uint32_t ExecuteEffect(
        std::uint32_t command, std::uint32_t target,
        std::uint32_t context) = 0;
    [[nodiscard]] virtual bool NeedsUpdate(std::uint32_t effect) = 0;
    virtual void BindEffect(std::uint32_t effect, std::string_view key,
                            std::uint32_t value) = 0;
    virtual void ReleaseEffect(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t ConfigValue(
        std::uint32_t effect, std::string_view key) = 0;
    [[nodiscard]] virtual int ResetAnimationEffect(
        std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t EventActor(
        std::uint32_t event) = 0;
    virtual void PrepareImmediateCast() = 0;
    [[nodiscard]] virtual std::uint32_t TargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorEmpty(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual int ReleaseIterator(std::uint32_t iterator) = 0;
};

class MagicActionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845708u;
    static constexpr std::uint32_t kSize = 336u;

    // IDA 0x566040: factory behind CastMagic child wrapper 0x845a5c.
    [[nodiscard]] static std::uint32_t Create(
        IMagicActionCreationRuntime& runtime);

    // IDA 0x5593e0-0x5594a0.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IMagicActionTaskRuntime& runtime,
        IEventSubscriptionRuntime& events, ICastMagicTaskRuntime& cast);
    [[nodiscard]] static std::uint8_t Stop(
        std::uint32_t task, std::uint32_t frame_context,
        IMagicActionTaskRuntime& runtime, IEventSubscriptionRuntime& events);
    static void Destroy(std::uint32_t task, IMagicActionTaskRuntime& runtime,
                        IEventSubscriptionRuntime& events);
    [[nodiscard]] static int ProcessCommand(
        std::uint32_t task, std::string_view message_name,
        IMagicActionTaskRuntime& runtime);
    static void ProcessInternal(std::uint32_t task,
                                IMagicActionProcessRuntime& runtime,
                                ICastMagicTaskRuntime& cast);
    [[nodiscard]] static int HandleAnimationEvent(
        std::uint32_t task, std::uint32_t effect,
        std::string_view event_name, IMagicActionProcessRuntime& runtime,
        ICastMagicTaskRuntime& cast);
    static void HandleCompletionEvent(std::uint32_t task,
                                      std::uint32_t effect,
                                      IMagicActionProcessRuntime& runtime);
    [[nodiscard]] static int HandleCombatEvent(
        std::uint32_t task, std::uint32_t event,
        IMagicActionProcessRuntime& runtime, ICastMagicTaskRuntime& cast);
};

}  // namespace pal4
