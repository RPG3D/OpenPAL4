#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ICombatDialogPropertyCollectionRuntime {
public:
    virtual ~ICombatDialogPropertyCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreatePropertyIterator(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t PropertySlot(
        std::uint32_t property) = 0;
};

class CombatDialogPropertyCollector final {
public:
    // IDA 0x57bc40.
    [[nodiscard]] static std::uint32_t Collect(
        std::array<std::uint32_t, 6>& properties,
        std::uint32_t actor,
        ICombatDialogPropertyCollectionRuntime& runtime);
};

}  // namespace pal4
