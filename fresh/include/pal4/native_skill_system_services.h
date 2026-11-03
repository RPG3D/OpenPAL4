#pragma once
#include <cstdint>
namespace pal4 {
class NativePlayerCombatEvents;
class RandomNumberGenerator;
// Borrowed service boundary keeps actor collection code independent of the
// SkillSystem owning package while requiring a real initialized instance.
class INativeSkillSystemServices {
public:
    virtual ~INativeSkillSystemServices() = default;
    [[nodiscard]] virtual std::uint32_t Address() const noexcept = 0;
    [[nodiscard]] virtual NativePlayerCombatEvents& PropertyEvents() noexcept = 0;
    [[nodiscard]] virtual RandomNumberGenerator& Random() noexcept = 0;
};
}
