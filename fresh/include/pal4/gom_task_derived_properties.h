#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

class GomTaskDerivedProperties final {
public:
    // IDA 0x61ddb0/0x61ddc0/0x61ddd0; shared by task-derived vtables.
    [[nodiscard]] static std::uint8_t GetFlag60(
        std::span<const std::uint32_t> words);
    [[nodiscard]] static std::uint8_t GetFlag61(
        std::span<const std::uint32_t> words);
    [[nodiscard]] static std::uint32_t SetWord68(
        std::span<std::uint32_t> words, std::uint32_t value);
    // IDA 0x61e3e0: shared by multiple task vtables.
    [[nodiscard]] static bool IsNonZero(std::uint32_t value);
};

}  // namespace pal4
