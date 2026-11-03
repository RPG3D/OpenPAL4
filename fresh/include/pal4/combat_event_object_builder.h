#pragma once

#include "pal4/combat_event_data_registry.h"

#include <cstdint>

namespace pal4 {

class ICombatEventObjectBuildRuntime : public ICombatEventDataRegistryRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual double ReadEventValue(
        std::uint32_t event, std::uint32_t source_address) = 0;
};

class CombatEventObjectBuilder final {
public:
    // IDA 0x581580.
    [[nodiscard]] static std::uint32_t Build(
        std::uint32_t record, std::uint32_t actor,
        std::uint32_t index, float scale, std::uint32_t mode,
        ICombatEventObjectBuildRuntime& runtime);
};

}  // namespace pal4
