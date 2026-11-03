#pragma once

#include <cstdint>

namespace pal4 {

class IMonsterManagerTreeTextRuntime {
public:
    virtual ~IMonsterManagerTreeTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) const = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextData() const = 0;
    virtual void SetErrorFlag(std::uint32_t record, std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination, std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void DetachAndReset(std::uint32_t record) = 0;
};

// IDA 0x438e90: copies a 20-byte record at a 32-bit process address.
class MonsterManagerTreeTextAssignment final {
public:
    static void Assign(std::uint32_t destination, std::uint32_t source,
                       IMonsterManagerTreeTextRuntime& runtime);
};

}  // namespace pal4
