#include "pal4/monster_unique_text.h"

#include "pal4/monster_behavior_manager.h"

#include <algorithm>
#include <stdexcept>

namespace pal4 {
namespace {

std::uint32_t Length(std::uint32_t address,
                     IMonsterUniqueTextRuntime& runtime) {
    std::uint32_t length = 0u;
    while (runtime.ReadByte(address + length) != 0u)
        ++length;
    return length;
}

struct TemporaryTextOwner {
    IMonsterUniqueTextRuntime& runtime;
    std::uint32_t record;

    ~TemporaryTextOwner() {
        const auto text = runtime.ReadDword(record + 4u);
        if (text == 0u)
            return;
        const auto header = text - 1u;
        const auto count = runtime.ReadByte(header);
        if (count == 0u || count == 0xffu)
            runtime.FreeTextHeader(header);
        else
            runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
    }
};

}  // namespace

std::uint8_t MonsterUniqueTextRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
std::uint32_t MonsterUniqueTextRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void MonsterUniqueTextRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void MonsterUniqueTextRuntimeStub::WriteDword(std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterUniqueTextRuntimeStub::EmptyTextAddress() {
    return CeguiTextRecord::kEmptyTextAddress;
}
void MonsterUniqueTextRuntimeStub::SetErrorFlag(std::uint32_t, std::uint8_t) {}
bool MonsterUniqueTextRuntimeStub::AllocateText(std::uint32_t,
                                               std::uint32_t,
                                               std::uint8_t) {
    return false;
}
void MonsterUniqueTextRuntimeStub::DetachAndReset(std::uint32_t) {}
void MonsterUniqueTextRuntimeStub::SetTextSize(std::uint32_t,
                                               std::uint32_t) {}
void MonsterUniqueTextRuntimeStub::CopyBytes(std::uint32_t,
                                             std::uint32_t,
                                             std::uint32_t) {}
void MonsterUniqueTextRuntimeStub::ThrowRange() {
    throw std::out_of_range("text record range");
}
std::uint32_t MonsterUniqueTextRuntimeStub::AllocateRecords(
    std::uint32_t bytes) {
    const auto address = next_record_address_;
    next_record_address_ += bytes;
    return address;
}
void MonsterUniqueTextRuntimeStub::FreeRecords(std::uint32_t) {}
void MonsterUniqueTextRuntimeStub::AssignRecord(std::uint32_t,
                                                std::uint32_t) {}
std::uint32_t MonsterUniqueTextRuntimeStub::TemporaryRecordAddress() {
    return 0x8000u;
}
void MonsterUniqueTextRuntimeStub::FreeTextHeader(std::uint32_t) {}

void MonsterUniqueText::Add(MonsterBehaviorManagerState& state,
                            std::uint32_t source_address,
                            IMonsterUniqueTextRuntime& runtime) {
    CeguiTextRecordArrayState array{
        state.words[0x720u / 4u], state.words[0x724u / 4u],
        state.words[0x728u / 4u], state.words[0x72cu / 4u]};
    for (auto record = array.begin; record != array.end; record += 16u) {
        const auto source_length = Length(source_address, runtime);
        const auto stored_length = runtime.ReadDword(record + 8u);
        bool equal = true;
        for (std::uint32_t index = 0u;
             index < std::min(stored_length, source_length); ++index) {
            if (runtime.ReadByte(runtime.ReadDword(record + 4u) + index) !=
                runtime.ReadByte(source_address + index)) {
                equal = false;
                break;
            }
        }
        if (equal && stored_length == source_length)
            return;
    }

    const auto length = Length(source_address, runtime);
    const auto temporary = runtime.TemporaryRecordAddress();
    runtime.WriteDword(temporary, source_address & 0xffu);
    runtime.WriteDword(temporary + 4u, 0u);
    runtime.WriteDword(temporary + 8u, 0u);
    runtime.WriteDword(temporary + 12u, 0u);
    TemporaryTextOwner owner{runtime, temporary};
    if (runtime.AllocateText(temporary, length, 1u)) {
        runtime.CopyBytes(runtime.ReadDword(temporary + 4u),
                          source_address, length);
        runtime.SetTextSize(temporary, length);
    }

    CeguiTextRecordArray::Insert(array, array.end, 1u, temporary, runtime);
    state.words[0x720u / 4u] = array.unused;
    state.words[0x724u / 4u] = array.begin;
    state.words[0x728u / 4u] = array.end;
    state.words[0x72cu / 4u] = array.capacity;
}

}  // namespace pal4
