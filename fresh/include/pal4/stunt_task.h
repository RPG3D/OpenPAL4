#pragma once

#include <cstdint>

namespace pal4 {

class IStuntTaskRuntime {
public:
    virtual ~IStuntTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void SetStuntTarget(std::uint32_t source,
                                std::uint32_t stunt,
                                std::uint32_t target) = 0;
    virtual void AdjustStuntCost(std::uint32_t source,
                                 std::uint32_t type,
                                 std::int32_t amount) = 0;
    virtual void AdjustActorValue(std::uint32_t source,
                                  std::int32_t amount) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyActorEffect(
        std::uint32_t source, std::uint32_t type,
        std::int32_t amount) = 0;
    [[nodiscard]] virtual std::uint32_t StuntTargetKind(
        std::uint32_t stunt) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual float FloatArrayElement(
        std::uint32_t array, std::uint32_t index) = 0;
    [[nodiscard]] virtual float GroupDamageScale() {
        return FloatArrayElement(GameDatabase()+184u,26u);
    }
    [[nodiscard]] virtual std::uint32_t TargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool CandidateEligible(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorProperty(
        std::uint32_t actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ActorReady(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t StuntIncludesSelf(
        std::uint32_t stunt) = 0;
    virtual void ApplyTargetHit(std::uint32_t target,
                                std::uint32_t source,
                                std::uint32_t stunt) = 0;
    virtual void NotifySource(std::uint32_t source,
                              std::uint32_t value,
                              std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t SetTargetState12(
        std::uint32_t target, std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t SetTargetStateOne(
        std::uint32_t target) = 0;
};

class StuntTask final {
public:
    // IDA 0x55b980-0x55bb90.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IStuntTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetTarget(
        std::uint32_t task, IStuntTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleActor(
        std::uint32_t task, IStuntTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleActorInternal(
        std::uint32_t task, std::uint32_t target,
        IStuntTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyEffects(
        std::uint32_t task, IStuntTaskRuntime& runtime);
};

}  // namespace pal4
