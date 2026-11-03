#include "pal4/game_data_monster_record_array.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 596;

void AssignCopies(std::uint32_t begin, std::uint32_t end,
                  std::uint32_t record,
                  IGameDataMonsterRecordArrayRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        runtime.Assign(destination, record);
}

void ShiftBackward(std::uint32_t begin, std::uint32_t end,
                   std::uint32_t destination_end,
                   IGameDataMonsterRecordArrayRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        runtime.Assign(destination_end, end);
    }
}

}  // namespace

std::uint32_t GameDataMonsterRecordArray::Insert(
    std::uint32_t array, std::uint32_t at, std::uint32_t record,
    IGameDataMonsterRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto slot = (at - begin) / kRecordBytes;
    const auto capacity_end = runtime.ReadWord(array + 12);
    if ((capacity_end - end) / kRecordBytes != 0) {
        if ((end - at) / kRecordBytes != 0) {
            runtime.CopyConstruct(end, end - kRecordBytes);
            auto source_end = end - kRecordBytes;
            auto destination_end = end;
            while (source_end != at) {
                source_end -= kRecordBytes;
                destination_end -= kRecordBytes;
                runtime.Assign(destination_end, source_end);
            }
            runtime.Assign(at, record);
        } else {
            runtime.CopyConstruct(end, record);
        }
        runtime.WriteWord(array + 8, end + kRecordBytes);
        return begin + slot * kRecordBytes;
    }

    const auto used = begin ? (end - begin) / kRecordBytes : 0;
    const auto growth = used > 1 ? used : 1;
    const auto new_capacity = used + growth;
    const auto replacement = runtime.Allocate(new_capacity * kRecordBytes);
    auto destination = CopyRange(begin, at, replacement, runtime);
    runtime.CopyConstruct(destination, record);
    destination += kRecordBytes;
    (void)CopyRange(at, end, destination, runtime);
    for (auto source = begin; source != end; source += kRecordBytes)
        GameDataMonsterRecord::DestroyFullRecord(source, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + new_capacity * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + 1) * kRecordBytes);
    return replacement + slot * kRecordBytes;
}

std::uint32_t GameDataMonsterRecordArray::Size(
    std::uint32_t array, IGameDataMonsterRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / kRecordBytes : 0;
}

std::uint32_t GameDataMonsterRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination,
    IGameDataMonsterRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += kRecordBytes, destination += kRecordBytes)
        runtime.CopyConstruct(destination, source);
    return destination;
}

void GameDataMonsterRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t at, std::uint32_t count,
    std::uint32_t record, IGameDataMonsterRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto spare = (capacity_end - end) / kRecordBytes;
    if (spare >= count) {
        const auto tail = (end - at) / kRecordBytes;
        if (tail >= count) {
            if (count != 0) {
                (void)CopyRange(end - count * kRecordBytes, end,
                                end, runtime);
                ShiftBackward(at, end - count * kRecordBytes,
                              end, runtime);
                AssignCopies(at, at + count * kRecordBytes,
                             record, runtime);
                runtime.WriteWord(array + 8,
                                  end + count * kRecordBytes);
            }
        } else {
            (void)CopyRange(at, end, at + count * kRecordBytes,
                            runtime);
            FillConstruct(end, count - tail, record, runtime);
            AssignCopies(at, end, record, runtime);
            runtime.WriteWord(array + 8, end + count * kRecordBytes);
        }
        return;
    }

    const auto used = begin ? (end - begin) / kRecordBytes : 0;
    const auto growth = used > count ? used : count;
    const auto replacement = runtime.Allocate((used + growth) *
                                               kRecordBytes);
    auto destination = CopyRange(begin, at, replacement, runtime);
    FillConstruct(destination, count, record, runtime);
    destination += count * kRecordBytes;
    (void)CopyRange(at, end, destination, runtime);
    for (auto source = begin; source != end; source += kRecordBytes)
        GameDataMonsterRecord::DestroyFullRecord(source, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + (used + growth) * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + count) * kRecordBytes);
}

void GameDataMonsterRecordArray::FillConstruct(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t record,
    IGameDataMonsterRecordArrayRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.CopyConstruct(destination + i * kRecordBytes, record);
}

}  // namespace pal4
