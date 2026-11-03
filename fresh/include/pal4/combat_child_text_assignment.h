#pragma once

#include <cstdint>

namespace pal4 {

class ICombatChildTextRuntime {
public:
    virtual ~ICombatChildTextRuntime() = default;
    // Native callers project borrowed text to modern ownership at this boundary.
    [[nodiscard]] virtual bool TryAssignHostText(std::uint32_t,std::uint32_t) { return false; }
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetTextErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t header, std::uint32_t length,
        std::uint8_t flag) = 0;
    virtual void CopyText(std::uint32_t header,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void DetachText(std::uint32_t header) = 0;
    virtual void MoveTextBytes(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t length) = 0;
    virtual void SetTextSize(std::uint32_t header,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t EraseText(
        std::uint32_t header, std::uint32_t start,
        std::uint32_t count) = 0;
};

class CombatChildTextAssignment final {
public:
    // Shared legacy text assignment in IDA 0x565a00 and 0x5699d0.
    [[nodiscard]] static std::uint8_t Assign(
        std::uint32_t destination_header,
        std::uint32_t source_header,
        ICombatChildTextRuntime& runtime);
};

}  // namespace pal4
