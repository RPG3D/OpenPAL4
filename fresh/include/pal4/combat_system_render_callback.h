#pragma once

#include <cstdint>

namespace pal4 {

class ICombatSystemRenderCallbackRuntime {
public:
    virtual ~ICombatSystemRenderCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetCombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEvent61(
        std::uint32_t combat_system, std::uint32_t argument) = 0;
};

class ICombatSystemRenderEventRuntime {
public:
    virtual ~ICombatSystemRenderEventRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ForwardEvent61(
        std::uint32_t combat_system, std::uint32_t argument) = 0;
};

class CombatSystemRenderCallback final {
public:
    // IDA 0x574df0.
    [[nodiscard]] static std::uint32_t Invoke(
        std::uint32_t argument,
        ICombatSystemRenderCallbackRuntime& runtime);
    // IDA 0x576190.
    [[nodiscard]] static std::uint32_t ProcessEvent61(
        std::uint32_t combat_system, std::uint32_t argument,
        ICombatSystemRenderEventRuntime& runtime);
};

}  // namespace pal4
