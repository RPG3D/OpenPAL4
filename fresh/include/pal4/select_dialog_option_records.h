#pragma once

#include <cstdint>

namespace pal4 {

class ISelectDialogRecordRuntime {
public:
    virtual ~ISelectDialogRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
};

class ISelectDialogReplaceRuntime {
public:
    virtual ~ISelectDialogReplaceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringPointer() = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool AllocateBuffer(
        std::uint32_t record, std::uint32_t length,
        bool copy_mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void DetachAndReset(std::uint32_t record) = 0;
    virtual void SetSize(std::uint32_t record,
                         std::uint32_t size) = 0;
    virtual void ThrowRangeError() = 0;
};

class SelectDialogOptionRecords final {
public:
    // IDA 0x4baad0.
    static void Cleanup(std::uint32_t begin,
                        std::uint32_t end,
                        ISelectDialogRecordRuntime& runtime);

    // IDA 0x4bab20.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        ISelectDialogRecordRuntime& runtime);

    // IDA 0x4bab60.
    static void CopyRepeated(std::uint32_t destination,
                             std::uint32_t count,
                             std::uint32_t source,
                             ISelectDialogRecordRuntime& runtime);

    // IDA 0x4bab90.
    [[nodiscard]] static std::uint32_t ReplaceRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source,
        ISelectDialogReplaceRuntime& runtime);

    // IDA 0x4bace0.
    [[nodiscard]] static std::uint32_t ReplaceRangeBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        ISelectDialogReplaceRuntime& runtime);
};

class ISelectDialogOptionArrayRuntime : public ISelectDialogRecordRuntime,
                                        public ISelectDialogReplaceRuntime {
public:
    virtual ~ISelectDialogOptionArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) override = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) override = 0;
    virtual void Free(std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
};

class SelectDialogOptionArray final {
public:
    // IDA 0x54e2d0: insert one 16-byte option record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, ISelectDialogOptionArrayRuntime& runtime);
    // IDA 0x54e450: record count from the begin/end fields.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, ISelectDialogOptionArrayRuntime& runtime);
};

}  // namespace pal4
