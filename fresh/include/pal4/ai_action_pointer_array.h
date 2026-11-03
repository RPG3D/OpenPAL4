#pragma once

#include <cstdint>

namespace pal4 {

class IAiActionPointerArrayRuntime {
public:
    virtual ~IAiActionPointerArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class AiActionPointerArray final {
public:
    // IDA 0x562360.
    [[nodiscard]] static std::uint32_t Capacity(
        std::uint32_t array, IAiActionPointerArrayRuntime& runtime);
    // IDA 0x562380.
    [[nodiscard]] static std::uint32_t InsertOne(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IAiActionPointerArrayRuntime& runtime);
};

}  // namespace pal4
