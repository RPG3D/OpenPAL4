#pragma once

#include <cstdint>

namespace pal4 {

class IReverseSubstringRuntime {
public:
    virtual ~IReverseSubstringRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
};

class ReverseSubstringRuntimeStub final : public IReverseSubstringRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
};

class ReverseSubstringSearch final {
public:
    // IDA 0x60bca0. The native owner stores data at +4 and length at +8.
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t data, std::uint32_t length,
        std::uint32_t needle, std::uint32_t needle_length,
        std::uint32_t start, IReverseSubstringRuntime& runtime);
};

}  // namespace pal4
