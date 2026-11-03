#pragma once

#include <cstdint>

namespace pal4 {

class ICombatWorldClumpRuntime {
public:
    virtual ~ICombatWorldClumpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::int32_t AddClump(
        std::uint32_t world, std::uint32_t clump) = 0;
};

class CombatWorldClumpGate final {
public:
    [[nodiscard]] static bool Add(std::uint32_t clump,
                                  ICombatWorldClumpRuntime& runtime);
};

}  // namespace pal4
