#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataEquipRecordCopyRuntime {
public:
    virtual ~IGameDataEquipRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CopyConstructText(std::uint32_t destination,
                                   std::uint32_t source) = 0;
    virtual void AssignText(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class GameDataEquipRecordCopy final {
public:
    // IDA 0x4d7df0: copy-construct 400 bytes, including 13 text records.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t destination, std::uint32_t source,
        IGameDataEquipRecordCopyRuntime& runtime);
    // IDA 0x4d8030: assign 400 bytes with string-aware text assignment.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameDataEquipRecordCopyRuntime& runtime);
};

}  // namespace pal4
