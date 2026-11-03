#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class LegacyCrc32 final {
public:
    // IDA 0x401370 and 0x668640: identical CRC routines with separate globals.
    [[nodiscard]] std::uint32_t ComputeAndStore(
        const std::uint8_t* data, std::uint32_t length);
    [[nodiscard]] bool Initialized() const noexcept { return initialized_; }
    [[nodiscard]] std::uint32_t LastResult() const noexcept {
        return last_result_;
    }

private:
    std::array<std::uint32_t, 256> table_{};
    bool initialized_ = false;
    std::uint32_t last_result_ = 0;
};

class GameDataRowHashGate final {
public:
    // IDA 0x4eeaa0: update the row hash and check a state in [0, 4).
    [[nodiscard]] static bool Update(
        std::uint32_t& row_hash, const std::uint8_t* data,
        std::uint32_t length, std::int32_t state, LegacyCrc32& crc);
    // IDA 0x4eee40: unsigned state at record offset +4 is at most two.
    [[nodiscard]] static bool IsStateAtMostTwo(std::uint32_t state) noexcept;
};

}  // namespace pal4
