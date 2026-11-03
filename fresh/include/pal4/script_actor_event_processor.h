#pragma once

#include <cstdint>

namespace pal4 {

class IScriptActorEventRuntime {
public:
    virtual ~IScriptActorEventRuntime() = default;
    [[nodiscard]] virtual std::int32_t ActorState(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool SourceBlocksEvent(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t OpenTargetIterator(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool CandidateStateNotThree(
        std::uint32_t candidate) = 0;
    [[nodiscard]] virtual bool CandidateCanAffect(
        std::uint32_t candidate, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t GenerateRandomNumber() = 0;
    virtual void DeleteIterator(std::uint32_t iterator,
                                std::uint32_t flags) = 0;
};

class ScriptActorEventProcessor final {
public:
    // IDA 0x563a90: select one eligible actor from the target's iterator.
    [[nodiscard]] static std::uint32_t SelectTarget(
        std::uint32_t source, std::uint32_t target,
        IScriptActorEventRuntime& runtime);
};

}  // namespace pal4
