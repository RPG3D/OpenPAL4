#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataPropertyRecordArrayRuntime {
public:
    virtual ~IGameDataPropertyRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void DestroyTextRecord(std::uint32_t address) = 0;
};

class IGameDataPropertyCapacityRuntime :
    public virtual IGameDataPropertyRecordArrayRuntime {
public:
    virtual void CopyIndex(std::uint32_t destination,
                           std::uint32_t source) = 0;
};

class IGameDataPropertyInsertRuntime :
    public virtual IGameDataPropertyRecordArrayRuntime {
public:
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
};

class GameDataPropertyCapacity final {
public:
    // IDA 0x4e6200: reserve record and key/index arrays independently.
    [[nodiscard]] static std::uint32_t Reserve(
        std::uint32_t table, std::int32_t requested,
        IGameDataPropertyCapacityRuntime& runtime);
};

class GameDataPropertyRecordArray final {
public:
    // IDA 0x4e63b0: insert one 168-byte record at an array position.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t source, IGameDataPropertyInsertRuntime& runtime);
    // IDA 0x4e6640: insert several copies of a 168-byte record.
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IGameDataPropertyInsertRuntime& runtime);
    // IDA 0x4e65d0: live record count from begin/end pointers.
    [[nodiscard]] static std::uint32_t Size(
        std::uint32_t array, IGameDataPropertyRecordArrayRuntime& runtime);
    // IDA 0x4e6600: copy-construct a half-open record range.
    [[nodiscard]] static std::uint32_t CopyConstructRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        IGameDataPropertyRecordArrayRuntime& runtime);
    // IDA 0x4e6a10: copy-construct the same record several times.
    static void CopyConstructCopies(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source,
        IGameDataPropertyRecordArrayRuntime& runtime);
};

}  // namespace pal4
