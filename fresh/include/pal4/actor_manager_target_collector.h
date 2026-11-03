#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerTargetCollectorRuntime {
public:
    virtual ~IActorManagerTargetCollectorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SelectionMode(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool AtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t Current(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t TargetType(
        std::uint32_t target) = 0;
    virtual void InsertDirect(std::uint32_t collection,
                              std::uint32_t target) = 0;
    virtual void InsertEvent(std::uint32_t collection,
                             std::uint32_t target) = 0;
    virtual void Advance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyIterator(
        std::uint32_t iterator) = 0;
};

class ActorManagerTargetCollector final {
public:
    [[nodiscard]] static std::uint32_t Collect(
        std::uint32_t actor, std::uint32_t collection,
        IActorManagerTargetCollectorRuntime& runtime);
    [[nodiscard]] static std::uint32_t CollectAll(
        std::uint32_t actor, std::uint32_t collection,
        IActorManagerTargetCollectorRuntime& runtime);
};

}  // namespace pal4
