#pragma once

#include <cstdint>

namespace pal4 {

class IWordVectorStorageRuntime {
public:
    virtual ~IWordVectorStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class WordVectorStorage final {
public:
    // IDA 0x573f70. Retains the allocated buffer and returns the old end.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t vector, IWordVectorStorageRuntime& runtime);
    // IDA 0x573fa0. Inserts one 32-bit element before position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t vector, std::uint32_t position,
        std::uint32_t source, IWordVectorStorageRuntime& runtime);
};

}  // namespace pal4
