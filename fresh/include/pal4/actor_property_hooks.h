#pragma once

#include "pal4/actor_facet_event_record.h"

#include <cstdint>

namespace pal4 {

class IActorPropertyHookRuntime : public IActorFacetEventRecordRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t ActorIntVirtual(
        std::uint32_t actor, std::uint32_t virtual_offset,
        std::uint32_t index) = 0;
    virtual void PendingAdjustVirtual(std::uint32_t pending,
                                      std::uint32_t virtual_offset,
                                      std::uint32_t index,
                                      std::int32_t delta) = 0;
    [[nodiscard]] virtual std::int32_t OwnerSetVirtual(
        std::uint32_t owner, std::uint32_t virtual_offset,
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t OwnerFinishVirtual(
        std::uint32_t owner, std::uint32_t virtual_offset,
        std::uint32_t actor, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryRecord() = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::int32_t DispatchEvent(
        std::uint32_t router, std::uint32_t key,
        std::uint32_t record, std::uint32_t mode) = 0;
};

class ActorPropertyHooks final {
public:
    // IDA 0x56dab0-0x56dc90, in entry order.
    [[nodiscard]] static std::uint32_t SetPendingPointer(
        std::uint32_t object, std::uint32_t value,
        IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::int32_t ForwardWithPendingAdjustment(
        std::uint32_t object, std::uint32_t actor,
        std::uint32_t index, std::int32_t value,
        IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructPendingPointer(
        std::uint32_t object, IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::int32_t DispatchThenSetPending(
        std::uint32_t object, std::uint32_t actor,
        std::uint32_t value, IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructPendingGate(
        std::uint32_t object, IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::int32_t ForwardUnlessPending(
        std::uint32_t object, std::uint32_t actor,
        std::uint32_t index, std::int32_t value,
        IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructReactiveGate(
        std::uint32_t object, IActorPropertyHookRuntime& runtime);
    static void NotifyReactiveGate(
        std::uint32_t object, std::uint32_t event_id,
        IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::int32_t ForwardWithReactiveAdjustment(
        std::uint32_t object, std::uint32_t actor,
        std::uint32_t index, std::int32_t value,
        IActorPropertyHookRuntime& runtime);
    [[nodiscard]] static std::int32_t DispatchThenFinish(
        std::uint32_t object, std::uint32_t actor,
        IActorPropertyHookRuntime& runtime);
};

}  // namespace pal4
