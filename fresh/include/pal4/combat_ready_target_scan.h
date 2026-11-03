#pragma once

#include <cstdint>

namespace pal4 {

class ICombatReadyTargetRuntime {
public:
    virtual ~ICombatReadyTargetRuntime() = default;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t OpenIterator(
        std::uint32_t actor, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator,
                                 std::uint32_t flags) = 0;
};

class CombatReadyTargetScan final {
public:
    [[nodiscard]] static std::uint32_t FirstReadyOther(
        ICombatReadyTargetRuntime& runtime,
        std::uint32_t actor, std::uint32_t vtable_offset,
        std::uint32_t excluded);
};

}  // namespace pal4
