#pragma once

#include <cstdint>

namespace pal4 {

class IActorPropertySlotRuntime {
public:
    virtual ~IActorPropertySlotRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t InvokePropertySlot(std::uint32_t property,std::uint32_t slot,
        std::uint32_t actor,std::uint32_t argument)=0;
};

class IActorReadyCandidateRuntime : public IActorPropertySlotRuntime {
public:
    virtual ~IActorReadyCandidateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ObjectCheckState3(std::uint32_t object) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t InvokePropertySlot(
        std::uint32_t property, std::uint32_t slot,
        std::uint32_t actor, std::uint32_t argument) = 0;
};

class IActorMetricRuntime {
public:
    virtual ~IActorMetricRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ActorMetric(std::uint32_t actor, std::uint32_t selector) = 0;
};
class IActorManagerDataEventRuntime : public IActorReadyCandidateRuntime, public IActorMetricRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
};

class ActorManagerDataEvents final {
public:
    [[nodiscard]] static bool Process(std::uint32_t actor,
                                      IActorReadyCandidateRuntime& runtime);
    [[nodiscard]] static std::uint32_t CallPropertySlot8(
        std::uint32_t actor, std::uint32_t argument,
        IActorPropertySlotRuntime& runtime);
    [[nodiscard]] static std::uint32_t CallPropertySlot12(
        std::uint32_t actor, std::uint32_t argument,
        IActorPropertySlotRuntime& runtime);
    [[nodiscard]] static bool CheckMetric8(
        std::uint32_t actor, std::uint32_t candidate,
        IActorMetricRuntime& runtime);
    [[nodiscard]] static bool CheckMetric9(
        std::uint32_t actor, std::uint32_t candidate,
        IActorMetricRuntime& runtime);
    [[nodiscard]] static bool SharesOwner(
        std::uint32_t actor, std::uint32_t candidate,
        IActorManagerDataEventRuntime& runtime);
};

}  // namespace pal4
