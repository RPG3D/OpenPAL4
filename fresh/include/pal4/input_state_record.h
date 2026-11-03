#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class InputStateRecord final {
public:
    static constexpr std::uint32_t kBytes = 0x1878u;

    [[nodiscard]] InputStateRecord& Initialize();
    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() { return bytes_; }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes() const {
        return bytes_;
    }

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

}  // namespace pal4
