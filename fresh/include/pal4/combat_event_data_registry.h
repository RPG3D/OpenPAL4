#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventDataRegistryRuntime {
public:
    virtual ~ICombatEventDataRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class CombatEventDataRegistry final {
public:
    static constexpr std::uint32_t kBase = 0x8fa8d0u;
    static constexpr std::uint32_t kCount = 79u;

    // IDA 0x581560.
    [[nodiscard]] static std::uint32_t Lookup(
        std::uint32_t index, ICombatEventDataRegistryRuntime& runtime);
};

}  // namespace pal4
