#pragma once

#include <cstdint>

namespace pal4 {

class ICombatLogBufferLifecycleRuntime {
public:
    virtual ~ICombatLogBufferLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t DestroyFacet(
        std::uint32_t facet, std::uint32_t flags) = 0;
};

class CombatLogBufferLifecycle final {
public:
    // IDA 0x57eec0: outer stream teardown.
    [[nodiscard]] static std::uint32_t CleanupStream(
        std::uint32_t ios, ICombatLogBufferLifecycleRuntime& runtime);
    // IDA 0x57ef90: embedded string buffer teardown.
    [[nodiscard]] static std::int32_t CleanupBuffer(
        std::uint32_t buffer, ICombatLogBufferLifecycleRuntime& runtime);
};

}  // namespace pal4
