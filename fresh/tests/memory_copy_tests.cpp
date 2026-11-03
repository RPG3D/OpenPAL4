#include "pal4/memory_copy.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>

int main() {
    for (std::uint32_t length = 0u; length <= 9u; ++length) {
        std::array<std::uint8_t, 12> source{};
        std::array<std::uint8_t, 12> destination{};
        destination.fill(0xaau);
        for (std::uint32_t index = 0u; index < source.size(); ++index) {
            source[index] = static_cast<std::uint8_t>(index + 1u);
        }
        pal4::MemoryCopy::Copy(destination.data(), source.data(), length);
        for (std::uint32_t index = 0u; index < destination.size(); ++index) {
            const auto expected = index < length ? source[index] : 0xaau;
            if (destination[index] != expected) {
                std::cerr << "copy length boundary failed\n";
                return EXIT_FAILURE;
            }
        }
    }
    std::array<std::uint8_t, 16> unaligned_source{};
    std::array<std::uint8_t, 16> unaligned_actual{};
    std::array<std::uint8_t, 16> unaligned_host{};
    for (std::uint32_t index = 0u; index < unaligned_source.size(); ++index) {
        unaligned_source[index] = static_cast<std::uint8_t>(index + 10u);
    }
    unaligned_actual.fill(0xaau);
    unaligned_host.fill(0xaau);
    pal4::MemoryCopy::Copy(unaligned_actual.data() + 1u,
                          unaligned_source.data() + 2u, 9u);
    std::memcpy(unaligned_host.data() + 1u,
                unaligned_source.data() + 2u, 9u);
    if (unaligned_actual != unaligned_host) {
        std::cerr << "unaligned nonoverlap copy differs from host memcpy\n";
        return EXIT_FAILURE;
    }
    pal4::MemoryCopy::Copy(nullptr, nullptr, 0u);
    if (pal4::MemoryCopy::CopyAndReturn(nullptr, nullptr, 0u) != nullptr) {
        std::cerr << "zero-length copy must return original destination\n";
        return EXIT_FAILURE;
    }
    std::array<std::uint8_t, 5> returned_destination{};
    const std::array<std::uint8_t, 5> returned_source{9, 8, 7, 6, 5};
    if (pal4::MemoryCopy::CopyAndReturn(
            returned_destination.data(), returned_source.data(), 5u) !=
            returned_destination.data() ||
        returned_destination != returned_source) {
        std::cerr << "copy result and complete byte contents failed\n";
        return EXIT_FAILURE;
    }
    std::array<std::uint8_t, 8> overlap{1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
    pal4::MemoryCopy::Copy(overlap.data() + 1u, overlap.data(), 6u);
    if (overlap != std::array<std::uint8_t, 8>{1u, 1u, 2u, 3u,
                                                 4u, 4u, 4u, 8u}) {
        std::cerr << "forward DWORD and byte copy order failed\n";
        return EXIT_FAILURE;
    }
    std::uint32_t source_word = 0x12345678u;
    std::uint32_t target_word = 0u;
    if (pal4::MemoryCopy::CopyWordIfDestinationNonNull(
            &target_word, &source_word) != &target_word ||
        target_word != source_word ||
        pal4::MemoryCopy::CopyWordIfDestinationNonNull(
            nullptr, nullptr) != nullptr) {
        std::cerr << "conditional DWORD copy or return failed\n";
        return EXIT_FAILURE;
    }
    std::uint8_t source_byte = 0x9au;
    std::uint8_t target_byte = 0u;
    if (pal4::MemoryCopy::CopyByteAndReturnSource(
            &target_byte, &source_byte) != &source_byte ||
        target_byte != source_byte) {
        std::cerr << "single byte copy or source return failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "memory copy boundary tests passed\n";
}
