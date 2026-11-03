#pragma once

#include <cstdint>

namespace pal4 {

class IRwGeometryFlagsRuntime {
public:
    virtual ~IRwGeometryFlagsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadGeometryWord(
        std::uint32_t address) = 0;
    virtual void WriteGeometryWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllGeometryFlagAtomics(
        std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwGeometryFlagsRuntime&),
        std::uint32_t context) = 0;
};

class RwGeometryFlags final {
public:
    // IDA 0x667b40 and 0x667b50.
    [[nodiscard]] static std::uint32_t SetAtomicFlags45(
        std::uint32_t atomic, std::uint32_t context,
        IRwGeometryFlagsRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetClumpFlags45(
        std::uint32_t clump, IRwGeometryFlagsRuntime& runtime);
};

}  // namespace pal4
