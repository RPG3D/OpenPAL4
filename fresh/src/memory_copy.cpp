#include "pal4/memory_copy.h"

namespace pal4 {

void MemoryCopy::Copy(void* destination, const void* source,
                      const std::uint32_t bytes) {
    auto* target = static_cast<std::uint8_t*>(destination);
    const auto* input = static_cast<const std::uint8_t*>(source);
    std::uint32_t offset = 0u;
    for (; bytes - offset >= 4u; offset += 4u) {
        const std::uint8_t word[4]{input[offset], input[offset + 1u],
                                   input[offset + 2u], input[offset + 3u]};
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            target[offset + index] = word[index];
        }
    }
    for (; offset < bytes; ++offset) {
        target[offset] = input[offset];
    }
}

void* MemoryCopy::CopyAndReturn(void* destination, const void* source,
                                std::uint32_t bytes) {
    Copy(destination, source, bytes);
    return destination;
}

std::uint32_t* MemoryCopy::CopyWordIfDestinationNonNull(
    std::uint32_t* destination, const std::uint32_t* source) {
    if (destination) *destination = *source;
    return destination;
}

const std::uint8_t* MemoryCopy::CopyByteAndReturnSource(
    std::uint8_t* destination, const std::uint8_t* source) {
    *destination = *source;
    return source;
}

}  // namespace pal4
