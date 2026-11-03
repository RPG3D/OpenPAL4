#pragma once

#include <cstdint>

namespace pal4 {

class IStripResourceBlockRuntime {
public:
    virtual ~IStripResourceBlockRuntime() = default;
    [[nodiscard]] virtual std::uint8_t StackByte() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class StripResourceBlock final {
public:
    static constexpr std::uint32_t kSize = 0x88u;

    // IDA 0x63cc80: initialize a strip block and its 36-byte list head.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t block, IStripResourceBlockRuntime& runtime);
};

}  // namespace pal4
