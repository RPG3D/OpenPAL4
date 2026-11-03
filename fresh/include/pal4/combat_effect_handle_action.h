#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using CombatEffectHandle = std::array<std::uint32_t, 2>;

class ICombatEffectHandleActionRuntime {
public:
    virtual ~ICombatEffectHandleActionRuntime() = default;
    virtual void BuildHandle(std::uint32_t target, CombatEffectHandle& handle,
                             std::uint32_t first_source,
                             std::uint32_t second_source) = 0;
    virtual void ResetHandle(CombatEffectHandle& handle) = 0;
};

class CombatEffectHandleAction final {
public:
    // IDA 0x581250; the second and third native arguments are unused.
    static void Process(std::uint32_t action, std::uint32_t target,
                        ICombatEffectHandleActionRuntime& runtime);
};

}  // namespace pal4
