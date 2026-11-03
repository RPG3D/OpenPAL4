#pragma once

#include "pal4/actor_facet_event_record.h"

#include <cstdint>

namespace pal4 {

class IActorDamageEventRuntime : public IActorFacetEventRecordRuntime {
public:
    [[nodiscard]] virtual std::int32_t ActorIntVirtual(
        std::uint32_t actor, std::uint32_t offset,
        std::uint32_t index) = 0;
    [[nodiscard]] virtual double ActorFloatVirtual(
        std::uint32_t actor, std::uint32_t offset,
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ActorStateVirtual(
        std::uint32_t actor, std::uint32_t offset) = 0;
    virtual void ActorVoidVirtual(std::uint32_t actor,
                                  std::uint32_t offset) = 0;
    virtual void SetValidatedProperty(std::uint32_t property,
                                      std::uint32_t index,
                                      std::int32_t value) = 0;
    virtual void UpdateHealthStatus(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryRecord() = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    virtual void DispatchEvent(std::uint32_t router,
                               std::uint32_t key,
                               std::uint32_t record,
                               std::uint32_t mode) = 0;
};

class ActorDamageEventSetters final {
public:
    // IDA 0x56c320/0x56c3a0/0x56c420/0x56c4a0.
    [[nodiscard]] static std::int32_t SetWord344(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat376(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetWord464(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetWord392(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);

    // IDA 0x56c520/0x56c5b0/0x56c630.
    [[nodiscard]] static double SetFloat416FromInt(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat440(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState632(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    // IDA 0x56c690-0x56c950.
    [[nodiscard]] static std::int32_t SetState636(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState640(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState644(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState648(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState652(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState656(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState660(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState664(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);

    // IDA 0x56c9b0-0x56cd10.
    [[nodiscard]] static std::int32_t SetExtendedWord(
        std::uint32_t actor, std::int32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat584(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat604(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double GetFloat484(
        std::uint32_t actor, std::int32_t index,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat484(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double GetFloat508(
        std::uint32_t actor, std::int32_t index,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat508(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double GetFloat536(
        std::uint32_t actor, std::int32_t index,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetFloat536(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);

    // IDA 0x56ce40-0x56d0d0: count-property event keys.
    [[nodiscard]] static std::int32_t SetCountWord344(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat376(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountWord464(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountWord392(
        std::uint32_t actor, std::uint32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat416FromInt(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat440(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);

    // IDA 0x56d160-0x56d560: count-property state and word events.
    [[nodiscard]] static std::int32_t SetCountState632(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState636(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState640(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState644(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState648(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState652(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState656(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState660(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountState664(
        std::uint32_t actor, std::uint32_t value,
        IActorDamageEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCountExtendedWord(
        std::uint32_t actor, std::int32_t index,
        std::int32_t value, IActorDamageEventRuntime& runtime);

    // IDA 0x56d610-0x56d8b0: remaining count-property float events.
    [[nodiscard]] static double SetCountFloat584(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat604(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat484(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat508(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
    [[nodiscard]] static double SetCountFloat536(
        std::uint32_t actor, std::int32_t index,
        float value, IActorDamageEventRuntime& runtime);
};

}  // namespace pal4
