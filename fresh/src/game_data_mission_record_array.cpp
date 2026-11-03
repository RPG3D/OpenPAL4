#include "pal4/game_data_mission_record_array.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 68;

}  // namespace

std::uint32_t GameDataMissionRecordArray::Insert(
    std::uint32_t array, std::uint32_t at, std::uint32_t record,
    IGameDataMissionRecordArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto slot = (at - begin) / kRecordBytes;
    const auto capacity_end = runtime.ReadWord(array + 12);
    if ((capacity_end - end) / kRecordBytes != 0) {
        if ((end - at) / kRecordBytes != 0) {
            runtime.CopyConstruct(end, end - kRecordBytes);
            (void)ShiftBackward(at, end - kRecordBytes, end,
                                runtime);
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
    auto destination = replacement;
    for (auto source = begin; source != at;
         source += kRecordBytes, destination += kRecordBytes)
        runtime.CopyConstruct(destination, source);
    runtime.CopyConstruct(destination, record);
    destination += kRecordBytes;
    destination = CopyRange(at, end, destination, runtime);
    for (auto source = begin; source != end; source += kRecordBytes)
        runtime.Destroy(source);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + new_capacity * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + 1) * kRecordBytes);
    return replacement + slot * kRecordBytes;
}

std::uint32_t GameDataMissionRecordArray::CopyRange(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataMissionRecordArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += kRecordBytes, destination += kRecordBytes)
        runtime.CopyConstruct(destination, source);
    return destination;
}

void GameDataMissionRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t at, std::uint32_t count,
    std::uint32_t record, IGameDataMissionRecordArrayRuntime& runtime) {
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
                (void)ShiftBackward(at, end - count * kRecordBytes,
                                    end, runtime);
                (void)AssignCopies(at, at + count * kRecordBytes,
                                   record, runtime);
                runtime.WriteWord(array + 8,
                                  end + count * kRecordBytes);
            }
        } else {
            (void)CopyRange(at, end, at + count * kRecordBytes,
                            runtime);
            FillConstruct(end, count - tail, record, runtime);
            (void)AssignCopies(at, end, record, runtime);
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
        runtime.Destroy(source);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + (used + growth) * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (used + count) * kRecordBytes);
}

void GameDataMissionRecordArray::FillConstruct(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t record, IGameDataMissionRecordArrayRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.CopyConstruct(destination + i * kRecordBytes, record);
}

std::uint32_t GameDataMissionRecordArray::AssignCopies(
    std::uint32_t begin, std::uint32_t end, std::uint32_t record,
    IGameDataMissionRecordArrayRuntime& runtime) {
    for (auto destination = begin; destination != end;
         destination += kRecordBytes)
        runtime.Assign(destination, record);
    return end;
}

std::uint32_t GameDataMissionRecordArray::ShiftBackward(
    std::uint32_t begin, std::uint32_t end,
    std::uint32_t destination_end,
    IGameDataMissionRecordArrayRuntime& runtime) {
    while (end != begin) {
        end -= kRecordBytes;
        destination_end -= kRecordBytes;
        runtime.Assign(destination_end, end);
    }
    return destination_end;
}

}  // namespace pal4
