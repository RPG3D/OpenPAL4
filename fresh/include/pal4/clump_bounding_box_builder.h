#pragma once

#include <cstdint>

namespace pal4 {

class IClumpBoundingBoxRuntime {
public:
    virtual ~IClumpBoundingBoxRuntime() = default;
    // Original shared secondary mode word at 0x92f560; retained after traversal.
    virtual void SetDirectTransformMode(std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomic(
        std::uint32_t clump, std::uint32_t box) = 0;
};

class ClumpBoundingBoxBuilder final {
public:
    // IDA 0x5fa1e0; traversal callback is IDA 0x5fa200.
    [[nodiscard]] static std::uint32_t Build(
        std::uint32_t clump, std::uint32_t direct_mode,
        std::uint32_t box, IClumpBoundingBoxRuntime& runtime);
};

}  // namespace pal4
