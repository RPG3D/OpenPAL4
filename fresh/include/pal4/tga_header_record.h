#pragma once

#include <cstdint>

namespace pal4 {

class ITgaHeaderRecordMemory {
public:
    virtual ~ITgaHeaderRecordMemory() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class TgaHeaderRecord final {
public:
    // IDA 0x5f9930.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t record, ITgaHeaderRecordMemory& memory);
};

}  // namespace pal4
