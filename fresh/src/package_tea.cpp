#include "pal4/package_tea.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kDelta = 0x9e3779b9u;

[[nodiscard]] std::uint32_t Mix(std::uint32_t previous,
                                 std::uint32_t next,
                                 std::uint32_t sum,
                                 std::uint32_t index,
                                 std::uint32_t selector,
                                 const std::array<std::uint32_t, 4>& key) {
    return (((previous >> 5u) ^ (next << 2u)) +
            ((next >> 3u) ^ (previous << 4u))) ^
           ((sum ^ next) + (key[(index & 3u) ^ selector] ^ previous));
}

[[nodiscard]] std::array<std::uint32_t, PackageTea::kBlockBytes / 4u>
ReadBlock(const std::uint8_t* bytes) {
    std::array<std::uint32_t, PackageTea::kBlockBytes / 4u> words{};
    for (std::size_t i = 0; i < words.size(); ++i) {
        const std::size_t offset = i * 4u;
        words[i] = std::uint32_t(bytes[offset]) |
                   (std::uint32_t(bytes[offset + 1u]) << 8u) |
                   (std::uint32_t(bytes[offset + 2u]) << 16u) |
                   (std::uint32_t(bytes[offset + 3u]) << 24u);
    }
    return words;
}

void WriteBlock(std::uint8_t* bytes,
                const std::array<std::uint32_t,
                                 PackageTea::kBlockBytes / 4u>& words) {
    for (std::size_t i = 0; i < words.size(); ++i) {
        const std::size_t offset = i * 4u;
        const std::uint32_t word = words[i];
        bytes[offset] = static_cast<std::uint8_t>(word);
        bytes[offset + 1u] = static_cast<std::uint8_t>(word >> 8u);
        bytes[offset + 2u] = static_cast<std::uint8_t>(word >> 16u);
        bytes[offset + 3u] = static_cast<std::uint8_t>(word >> 24u);
    }
}

}  // namespace

int PackageTea::DecryptBytes(std::span<std::uint8_t> bytes,
                             const std::array<std::uint32_t, 4>& key) noexcept {
    const std::size_t count = bytes.size() / 4u;
    if (count < 2u) return -1;
    const auto read = [&](std::size_t i) {
        const auto p = i * 4u;
        return std::uint32_t(bytes[p]) | (std::uint32_t(bytes[p + 1]) << 8u) |
               (std::uint32_t(bytes[p + 2]) << 16u) | (std::uint32_t(bytes[p + 3]) << 24u);
    };
    const auto write = [&](std::size_t i, std::uint32_t value) {
        for (unsigned b = 0; b < 4; ++b) bytes[i * 4u + b] = static_cast<std::uint8_t>(value >> (8u * b));
    };
    std::uint32_t rounds = 6u + static_cast<std::uint32_t>(52u / count);
    std::uint32_t sum = rounds * kDelta, next = read(0);
    while (rounds-- != 0u) {
        const std::uint32_t selector = (sum >> 2u) & 3u;
        for (std::size_t index = count - 1u; index > 0u; --index) {
            next = read(index) - Mix(read(index - 1u), next, sum, static_cast<std::uint32_t>(index), selector, key);
            write(index, next);
        }
        next = read(0) - Mix(read(count - 1u), next, sum, 0u, selector, key);
        write(0, next);
        sum -= kDelta;
    }
    return 0;
}

int PackageTea::DecryptWords(
    std::span<std::uint32_t> words,
    const std::array<std::uint32_t, 4>& key) noexcept {
    const std::size_t count = words.size();
    if (count < 2u) return -1;
    std::uint32_t rounds = 6u + static_cast<std::uint32_t>(52u / count);
    std::uint32_t sum = rounds * kDelta;
    std::uint32_t next = words[0];
    while (rounds-- != 0u) {
        const std::uint32_t selector = (sum >> 2u) & 3u;
        for (std::size_t index = count - 1u; index > 0u; --index) {
            const std::uint32_t previous = words[index - 1u];
            next = words[index] -= Mix(previous, next, sum,
                                       static_cast<std::uint32_t>(index),
                                       selector, key);
        }
        const std::uint32_t previous = words[count - 1u];
        next = words[0] -= Mix(previous, next, sum, 0u, selector, key);
        sum -= kDelta;
    }
    return 0;
}

std::array<std::uint32_t, 4> PackageTea::PackageKeyWords() {
    std::array<std::uint32_t, 4> words{};
    for (std::size_t i = 0; i < 16u; ++i)
        words[i / 4u] |= std::uint32_t(
            static_cast<std::uint8_t>(kKey[i])) << (8u * (i % 4u));
    return words;
}

bool PackageTea::DecryptArchiveTable(std::span<std::uint8_t> table) {
    if (table.size() <= 0x2000u) return false;
    auto front = ReadBlock(table.data());
    auto back = ReadBlock(table.data() + table.size() - kBlockBytes - 1u);
    const auto key = PackageKeyWords();
    (void)DecryptWords(front, key);
    (void)DecryptWords(back, key);
    WriteBlock(table.data(), front);
    WriteBlock(table.data() + table.size() - kBlockBytes - 1u, back);
    return true;
}

}  // namespace pal4
