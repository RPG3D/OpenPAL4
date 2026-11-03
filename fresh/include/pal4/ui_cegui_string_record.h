#pragma once

#include <cstdint>

namespace pal4 {

class IUiCeguiStringRecordRuntime {
public:
    virtual ~IUiCeguiStringRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void DestroyString(std::uint32_t record) = 0;
};

class UiCeguiStringRecord final {
public:
    // IDA 0x4c1820.
    static void Free(std::uint32_t record,
                     IUiCeguiStringRecordRuntime& runtime);
};

}  // namespace pal4
