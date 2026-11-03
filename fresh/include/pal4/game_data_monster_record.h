#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataMonsterRecordRuntime {
public:
    virtual ~IGameDataMonsterRecordRuntime() = default;
    virtual void ConstructText(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IGameDataMonsterRecordDestroyRuntime {
public:
    virtual ~IGameDataMonsterRecordDestroyRuntime() = default;
    virtual void DestroyText(std::uint32_t address) = 0;
};

class GameDataMonsterRecord final {
public:
    // IDA 0x4de4f0: construct 6 + 5 text records, then reset scalar fields.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t base, IGameDataMonsterRecordRuntime& runtime);
    // IDA 0x4de570: clear the record's scalar and array-pointer fields.
    static void ResetScalarAndArrayFields(
        std::uint32_t base, IGameDataMonsterRecordRuntime& runtime);
    // IDA 0x4dee20: destroy 5 + 6 text records in reverse order.
    static void DestroySubrecord(
        std::uint32_t base, IGameDataMonsterRecordDestroyRuntime& runtime);
    // IDA 0x4df800: destroy the text subobject at record +336.
    static void DestroyFullRecord(
        std::uint32_t record,
        IGameDataMonsterRecordDestroyRuntime& runtime);
};

}  // namespace pal4
