#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventRecordRuntime {
public:
    virtual ~ICombatEventRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    virtual void SetErrorFlag(std::uint32_t record, std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             bool copy_mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void DetachAndReset(std::uint32_t record) = 0;
    virtual void SetTextSize(std::uint32_t record,
                             std::uint32_t length) = 0;
};

class CombatEventRecord final {
public:
    // IDA 0x57fa20: the trailing word comes from a separate pointer.
    [[nodiscard]] static std::uint32_t CopyData(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t metadata, ICombatEventRecordRuntime& runtime);
    // IDA 0x57fb60: null destination is ignored; metadata is source + 16.
    static void CopyExtended(std::uint32_t destination,
                             std::uint32_t source,
                             ICombatEventRecordRuntime& runtime);
};

}  // namespace pal4
