#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace pal4 {

class PackageTea final {
public:
    static constexpr const char* kKey =
        "Vampire.C.J at Softstar Technology (ShangHai) Co., Ltd";
    static constexpr std::size_t kBlockBytes = 4096u;

    // IDA 0x794260: corrected Block TEA over little-endian 32-bit words.
    [[nodiscard]] static int DecryptWords(
        std::span<std::uint32_t> words,
        const std::array<std::uint32_t, 4>& key) noexcept;
    // Decrypt complete little-endian words in place; trailing bytes are untouched.
    [[nodiscard]] static int DecryptBytes(
        std::span<std::uint8_t> bytes,
        const std::array<std::uint32_t, 4>& key) noexcept;
    // IDA 0x794390: decrypt first and trailing 4096-byte table regions.
    [[nodiscard]] static bool DecryptArchiveTable(
        std::span<std::uint8_t> table);
    [[nodiscard]] static std::array<std::uint32_t, 4> PackageKeyWords();
};

}  // namespace pal4
