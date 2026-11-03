#pragma once

#include "pal4/combat_event_data_processor.h"

#include <cstdint>

namespace pal4 {

class ICombatEventStateUpdateRuntime {
public:
    virtual ~ICombatEventStateUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t SetSlot(
        std::uint32_t actor, std::uint32_t slot,
        std::int32_t value) = 0;
    [[nodiscard]] virtual std::int32_t QuerySlot(
        std::uint32_t actor, std::uint32_t slot) = 0;
    virtual void RestoreSlot(std::uint32_t actor, std::uint32_t slot,
                             std::int32_t value) = 0;
    virtual void Notify(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t BaseStat(
        std::uint32_t actor, std::uint32_t slot) = 0;
};

class CombatEventStateUpdater final {
public:
    // IDA 0x581690.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t actor, std::int32_t value,
        std::uint32_t key, ICombatEventStateUpdateRuntime& runtime,
        ICombatEventDataRuntime& event_runtime);
};

}  // namespace pal4
