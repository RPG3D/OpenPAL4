#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActionConsoleWindowFactoryRuntime {
public:
    virtual ~ICombatActionConsoleWindowFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructFrame(
        std::uint32_t object, std::uint32_t name) = 0;
};

class CombatActionConsoleWindowFactory final {
public:
    static constexpr std::uint32_t kFrameSize = 0x1e8u;

    // IDA 0x540f70.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name,
        ICombatActionConsoleWindowFactoryRuntime& runtime);
};

}  // namespace pal4
