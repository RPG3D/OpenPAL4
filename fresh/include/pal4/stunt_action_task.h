#pragma once

#include "pal4/event_subscription_set.h"
#include "pal4/combat_effect_slot.h"
#include "pal4/stunt_task.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct StuntActionHandlerBinding {
    std::uint32_t key_address;
    std::uint32_t callback;
};

class StuntActionHandlers final {
public:
    static constexpr std::array<StuntActionHandlerBinding, 5> kBindings{{
        {0x8b8b64u, 0x55cb00u},
        {0x8b6310u, 0x55cab0u},
        {0x8b5fbcu, 0x55cab0u},
        {0x8b631cu, 0x55cc30u},
        {0x8b5fc8u, 0x55cc30u},
    }};

    // IDA 0x55bbe0; the cleanup call is shared.
    [[nodiscard]] static std::uint32_t Subscribe(
        std::uint32_t task, IEventSubscriptionRuntime& runtime);
    static void Clear(std::uint32_t task,
                      IEventSubscriptionRuntime& runtime);
};

class IStuntActionActorEventRuntime {
public:
    virtual ~IStuntActionActorEventRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t EventActor(std::uint32_t)=0;
    virtual std::uint32_t CallActor(std::uint32_t,std::uint32_t,std::uint32_t)=0;
};
class IStuntActionProcessRuntime : public IStuntActionActorEventRuntime {
public:
    virtual ~IStuntActionProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::string_view StuntName(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t StuntTargetKind(
        std::uint32_t stunt) = 0;
    [[nodiscard]] virtual bool ActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyEffect() = 0;
    [[nodiscard]] virtual std::uint32_t ExecuteEffect(
        std::uint32_t command, std::uint32_t target,
        std::uint32_t context) = 0;
    [[nodiscard]] virtual bool NeedsUpdate(std::uint32_t effect) = 0;
    virtual void BindEffect(std::uint32_t effect,
                            std::string_view key,
                            std::uint32_t value) = 0;
    virtual void ReleaseEffect(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t ConfigValue(
        std::uint32_t effect, std::string_view key) = 0;
    [[nodiscard]] virtual int ResetAnimationEffect(
        std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t EventActor(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t CallActor(
        std::uint32_t actor, std::uint32_t slot,
        std::uint32_t argument) = 0;
    virtual void PrepareImmediateStunt() = 0;
    [[nodiscard]] virtual std::uint32_t TargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorEmpty(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual int ReleaseIterator(
        std::uint32_t iterator) = 0;
};

class ICombatActionTargetRuntime {
public:
    virtual ~ICombatActionTargetRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t) = 0;
};

class IStuntActionFrameRuntime : public ICombatActionTargetRuntime {
public:
    virtual ~IStuntActionFrameRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint8_t ReadByte(std::uint32_t)=0;
    virtual std::string_view TaskAnimationName(std::uint32_t,std::uint32_t)=0;
    virtual std::string_view ActorAnimationName(std::uint32_t)=0;
    virtual bool ActorAnimationFlag(std::uint32_t)=0;
    virtual void ActorSetState(std::uint32_t,std::uint32_t)=0;
    virtual bool IsActionPending(std::uint32_t)=0;
};
class IStuntActionTaskRuntime : public IStuntActionFrameRuntime {
public:
    virtual ~IStuntActionTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view TaskAnimationName(
        std::uint32_t task, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::string_view ActorAnimationName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorAnimationFlag(
        std::uint32_t actor) = 0;
    virtual void ActorSetState(std::uint32_t actor,
                               std::uint32_t state) = 0;
    virtual void ActorReset(std::uint32_t actor) = 0;
    virtual void StartAction(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint8_t StopAction(
        std::uint32_t task, std::uint32_t frame_context) = 0;
    [[nodiscard]] virtual bool IsActionPending(std::uint32_t task) = 0;
    virtual void DestroyEmbedded(std::uint32_t task) = 0;
    virtual void DestroyBase(std::uint32_t task) = 0;
    virtual void ProcessAnimationCommand(
        std::uint32_t task, std::string_view original_name) = 0;
    [[nodiscard]] virtual int CleanupCommand(
        std::string_view lowercase_name) = 0;
};

class IStuntActionCreationRuntime : public ICombatEffectSlotRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyGenericBase(std::uint32_t object) = 0;
    virtual void DestroyCombatBase(std::uint32_t object) = 0;
    virtual void DestroyMessageBase(std::uint32_t object) = 0;
    virtual void DestroyEmbeddedBase(std::uint32_t embedded) = 0;
    virtual void DestroyEffectSlot(std::uint32_t slot) = 0;
    virtual void SetTextErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
};

class IStuntActionEmbeddedCreationRuntime {
public:
    virtual ~IStuntActionEmbeddedCreationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class StuntActionEmbeddedTask final {
public:
    static constexpr std::uint32_t kSize = 76u;
    static constexpr std::uint32_t kVtable = 0x845d98u;

    // IDA 0x56a690.
    [[nodiscard]] static std::uint32_t Create(
        IStuntActionEmbeddedCreationRuntime& runtime);
};

class StuntActionTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8457a4u;
    static constexpr std::uint32_t kSize = 336u;

    // IDA 0x56a5a0.
    [[nodiscard]] static std::uint32_t Create(
        IStuntActionCreationRuntime& runtime);

    // IDA 0x55be10-0x55cc30.
    static void Destroy(std::uint32_t task,
                        IStuntActionTaskRuntime& runtime,
                        IEventSubscriptionRuntime& events);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IStuntActionTaskRuntime& runtime,
        IEventSubscriptionRuntime& events,
        IStuntTaskRuntime& stunt);
    [[nodiscard]] static std::uint8_t Stop(
        std::uint32_t task, std::uint32_t frame_context,
        IStuntActionTaskRuntime& runtime,
        IEventSubscriptionRuntime& events);
    [[nodiscard]] static bool CheckStatus(
        std::uint32_t task, IStuntActionFrameRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateActorState(
        std::uint32_t task, IStuntActionFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetActorProperty(
        std::uint32_t task, ICombatActionTargetRuntime& runtime);
    [[nodiscard]] static int ProcessCommand(
        std::uint32_t task, std::string_view message_name,
        IStuntActionTaskRuntime& runtime,
        IStuntActionProcessRuntime& process,
        IStuntTaskRuntime& stunt);
    static void ProcessInternal(std::uint32_t task,
                                IStuntActionProcessRuntime& runtime,
                                IStuntTaskRuntime& stunt);
    [[nodiscard]] static int HandleAnimationEvent(
        std::uint32_t task, std::uint32_t effect,
        std::string_view event_name,
        IStuntActionProcessRuntime& runtime,
        IStuntTaskRuntime& stunt);
    [[nodiscard]] static std::uint32_t HandleActorEvent(
        std::uint32_t task, std::uint32_t event,
        IStuntActionActorEventRuntime& runtime);
    static void HandleCompletionEvent(
        std::uint32_t task, std::uint32_t effect,
        IStuntActionProcessRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleCombatEvent(
        std::uint32_t task, std::uint32_t event,
        IStuntActionProcessRuntime& runtime,
        IStuntTaskRuntime& stunt);
};

}  // namespace pal4
