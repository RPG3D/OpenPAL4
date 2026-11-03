#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class AtlNilStringManagerSlot final {
public:
    // IDA 0x5ceba0.
    [[nodiscard]] std::uint32_t SetManager(std::uint32_t address) noexcept {
        return manager_ = address;
    }
    [[nodiscard]] std::uint32_t Manager() const noexcept { return manager_; }

private:
    std::uint32_t manager_ = 0;
};

class DialogBitset256 final {
public:
    using Words = std::array<std::uint32_t, 8>;

    // IDA 0x5cebb0.
    [[nodiscard]] static Words& SetBit(Words& words, std::uint32_t position,
                                       bool value);
    // IDA 0x5cec10. The native loop's final index is -1.
    [[nodiscard]] static int Fill(Words& words, std::uint32_t value) noexcept;
};

}  // namespace pal4
