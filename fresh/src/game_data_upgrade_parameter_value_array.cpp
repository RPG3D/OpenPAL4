#include "pal4/game_data_upgrade_parameter_value_array.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 92;

}  // namespace

std::uint32_t GameDataUpgradeParameterValueArray::CopyRecords(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    for (auto source = begin; source != end; source += kRecordBytes) {
        if (destination)
            runtime.WriteRecord(destination, runtime.ReadRecord(source));
        destination += kRecordBytes;
    }
    return destination;
}

std::uint32_t GameDataUpgradeParameterValueArray::Size(
    std::uint32_t array, IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / kRecordBytes : 0;
}

std::uint32_t GameDataUpgradeParameterValueArray::Insert(
    std::uint32_t array, std::uint32_t position,
    const GameDataUpgradeParameterRecord& record,
    IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto insertion_index = (position - begin) / kRecordBytes;

    if ((capacity_end - end) / kRecordBytes != 0) {
        for (auto source = end; source != position; source -= kRecordBytes)
            runtime.WriteRecord(source,
                                runtime.ReadRecord(source - kRecordBytes));
        runtime.WriteRecord(position, record);
        runtime.WriteWord(array + 8, end + kRecordBytes);
        return begin + insertion_index * kRecordBytes;
    }

    const auto old_count = begin ? (end - begin) / kRecordBytes : 0;
    const auto growth = old_count > 1 ? old_count : 1;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * kRecordBytes);
    auto destination = CopyRecords(begin, position, replacement, runtime);
    if (destination) runtime.WriteRecord(destination, record);
    destination += kRecordBytes;
    (void)CopyRecords(position, end, destination, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (old_count + 1) * kRecordBytes);
    return replacement + insertion_index * kRecordBytes;
}

std::uint32_t GameDataUpgradeParameterValueArray::InsertCopies(
    std::uint32_t array, std::uint32_t position, std::uint32_t count,
    const GameDataUpgradeParameterRecord& record,
    IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto free_count = (capacity_end - end) / kRecordBytes;
    const auto suffix_count = (end - position) / kRecordBytes;
    if (free_count >= count) {
        if (count == 0) return 0;
        for (auto source = end; source != position; source -= kRecordBytes) {
            const auto destination = source + (count - 1) * kRecordBytes;
            if (destination)
                runtime.WriteRecord(destination,
                                    runtime.ReadRecord(source - kRecordBytes));
        }
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto destination = position + i * kRecordBytes;
            if (destination) runtime.WriteRecord(destination, record);
        }
        const auto new_end = end + count * kRecordBytes;
        runtime.WriteWord(array + 8, new_end);
        return suffix_count >= count ? position + count * kRecordBytes
                                     : new_end;
    }

    const auto old_count = begin ? (end - begin) / kRecordBytes : 0;
    const auto growth = old_count >= count ? old_count : count;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * kRecordBytes);
    auto destination = CopyRecords(begin, position, replacement, runtime);
    for (std::uint32_t i = 0; i < count; ++i) {
        if (destination) runtime.WriteRecord(destination, record);
        destination += kRecordBytes;
    }
    (void)CopyRecords(position, end, destination, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (old_count + count) * kRecordBytes);
    return begin;
}

std::uint32_t GameDataUpgradeParameterValueArray::FillRecords(
    std::uint32_t destination, std::uint32_t count,
    const GameDataUpgradeParameterRecord& record,
    IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    if (count == 0) return 0;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (destination) runtime.WriteRecord(destination, record);
        destination += kRecordBytes;
    }
    return destination;
}

void GameDataUpgradeParameterValueArray::CopyRecordIfPresent(
    std::uint32_t destination, std::uint32_t source,
    IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    if (destination)
        runtime.WriteRecord(destination, runtime.ReadRecord(source));
}

std::uint32_t GameDataUpgradeParameterValueArray::CompactAdjacent(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    const RecordEqual& equal,
    IGameDataUpgradeParameterValueArrayRuntime& runtime) {
    if (begin == end) return destination;
    runtime.WriteRecord(destination, runtime.ReadRecord(begin));
    auto last_kept = begin;
    auto output = destination + kRecordBytes;
    for (auto current = begin + kRecordBytes; current != end;
         current += kRecordBytes) {
        if (!equal(runtime.ReadRecord(last_kept), runtime.ReadRecord(current))) {
            last_kept = current;
            runtime.WriteRecord(output, runtime.ReadRecord(current));
            output += kRecordBytes;
        }
    }
    return output;
}

}  // namespace pal4
