#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRecordArrayRuntime {
public:
    virtual ~IGameDataRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t record_bytes) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source,
                        std::uint32_t record_bytes) = 0;
    virtual void DestroyRange(std::uint32_t begin, std::uint32_t end,
                              std::uint32_t record_bytes) = 0;
};

class SkillDataRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 132;
    // IDA 0x4f65a0: insert one 132-byte Skill record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f67a0: count active Skill records.
    [[nodiscard]] static std::int32_t Count(
        std::uint32_t array, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f67d0: copy-construct a half-open Skill record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f6cb0: insert repeated 132-byte Skill records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f7650: copy-construct repeated Skill records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IGameDataRecordArrayRuntime& runtime);
};

class StuntDataRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 208;
    // IDA 0x4f6810: insert one 208-byte Stunt record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f6a20: copy-construct a half-open Stunt record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f7050: insert repeated 208-byte Stunt records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f7680: copy-construct repeated Stunt records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IGameDataRecordArrayRuntime& runtime);
};

class MagicDataRecordArray final {
public:
    static constexpr std::uint32_t kRecordBytes = 152;
    // IDA 0x4f6a60: insert one 152-byte Magic record.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f6c70: copy-construct a half-open Magic record range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f7350: insert repeated 152-byte Magic records.
    static void InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataRecordArrayRuntime& runtime);
    // IDA 0x4f76b0: copy-construct repeated Magic records.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t source,
                     IGameDataRecordArrayRuntime& runtime);
};

}  // namespace pal4
