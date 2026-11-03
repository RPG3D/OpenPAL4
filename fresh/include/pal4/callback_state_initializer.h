#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

class CallbackState final {
public:
    static constexpr std::size_t kSize = 0x20;
    using Storage = std::array<std::byte, kSize>;

    [[nodiscard]] static Storage Initialize(Storage initial = {}) noexcept;
    [[nodiscard]] static std::uint32_t SetDword0C(
        Storage& storage, std::uint32_t value) noexcept;
    [[nodiscard]] static std::uint8_t SetByte1A(
        Storage& storage, std::uint8_t value) noexcept;
    [[nodiscard]] static std::uint32_t ReadDword(
        const Storage& storage, std::size_t offset) noexcept;
    static void WriteDword(Storage& storage, std::size_t offset,
                           std::uint32_t value) noexcept;
    [[nodiscard]] static bool IsPropertyEmpty(
        const Storage& storage) noexcept;
};

}  // namespace pal4
