#pragma once

#include <cstdint>

namespace pal4 {

class IMonsterManagerTreeRemoveRuntime {
public:
    virtual ~IMonsterManagerTreeRemoveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) const = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

// IDA 0x438520. Returns the successor captured before removal.
class MonsterManagerTreeRemove final {
public:
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t tree, std::uint32_t node,
        IMonsterManagerTreeRemoveRuntime& runtime);
};

}  // namespace pal4
