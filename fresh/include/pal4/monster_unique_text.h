#pragma once

#include "pal4/cegui_text_record_array.h"

#include <cstdint>

namespace pal4 {

struct MonsterBehaviorManagerState;

class IMonsterUniqueTextRuntime : public ICeguiTextRecordArrayRuntime {
public:
    [[nodiscard]] virtual std::uint32_t TemporaryRecordAddress() = 0;
    virtual void FreeTextHeader(std::uint32_t address) = 0;
};

// Linkable boundary for the original 32-bit allocator and address space.
class MonsterUniqueTextRuntimeStub final : public IMonsterUniqueTextRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t EmptyTextAddress() override;
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override;
    bool AllocateText(std::uint32_t record, std::uint32_t size,
                      std::uint8_t mode) override;
    void DetachAndReset(std::uint32_t record) override;
    void SetTextSize(std::uint32_t record, std::uint32_t size) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override;
    [[noreturn]] void ThrowRange() override;
    std::uint32_t AllocateRecords(std::uint32_t bytes) override;
    void FreeRecords(std::uint32_t address) override;
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override;
    std::uint32_t TemporaryRecordAddress() override;
    void FreeTextHeader(std::uint32_t address) override;

private:
    std::uint32_t next_record_address_{0x10000u};
};

class MonsterUniqueText final {
public:
    // IDA 0x434cf0: append a C string to the manager's unique text array.
    static void Add(MonsterBehaviorManagerState& state,
                    std::uint32_t source_address,
                    IMonsterUniqueTextRuntime& runtime);
};

}  // namespace pal4
