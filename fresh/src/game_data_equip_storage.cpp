#include "pal4/game_data_equip_storage.h"

#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 400;
constexpr std::uint32_t kKeyBytes = 8;

}  // namespace

void GameDataEquipStorage::Sort(
    std::uint32_t table, IGameDataEquipStorageRuntime& runtime) {
    const auto records = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    const auto old_count = records ? (old_end - records) / kRecordBytes : 0;
    std::vector<GameDataAiKeyEntry> keys;
    keys.reserve(old_count);
    for (std::uint32_t source = 0; source < old_count; ++source) {
        const auto source_address = records + source * kRecordBytes;
        const auto record = runtime.ReadRecord(source_address);
        if (!keys.empty() && keys.back().key == record[36]) continue;
        const auto destination = static_cast<std::uint32_t>(keys.size());
        if (destination != source)
            runtime.AssignRecord(records + destination * kRecordBytes,
                                 source_address);
        keys.push_back({record[36], destination});
    }
    const auto new_end = records +
                         static_cast<std::uint32_t>(keys.size()) *
                             kRecordBytes;
    GameDataEquipRecordLifecycle::DestroyRange(new_end, old_end, runtime);
    runtime.WriteWord(table + 100, new_end);

    runtime.WriteWord(table + 12, runtime.ReadWord(table + 8));
    for (const auto& entry : keys) runtime.AppendKey(table + 4, entry);
    GameDataAiKeyAlgorithms::Sort(keys);
    const auto key_begin = runtime.ReadWord(table + 8);
    for (std::uint32_t i = 0; i < keys.size(); ++i)
        runtime.WriteKey(key_begin + i * kKeyBytes, keys[i]);
}

std::uint32_t GameDataEquipStorage::Reserve(
    std::uint32_t table, std::uint32_t requested,
    IGameDataEquipStorageRuntime& runtime) {
    const auto old_records = runtime.ReadWord(table + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(table + 104) -
                                        old_records) / kRecordBytes
                                     : 0;
    if (record_capacity < requested) {
        const auto count = static_cast<std::int32_t>(requested) < 0 ? 0u
                                                                : requested;
        const auto replacement = runtime.Allocate(count * kRecordBytes);
        const auto old_end = runtime.ReadWord(table + 100);
        const auto record_count = old_records
                                      ? (old_end - old_records) / kRecordBytes
                                      : 0;
        for (std::uint32_t i = 0; i < record_count; ++i) {
            if (replacement)
                runtime.CopyConstructRecord(
                    replacement + i * kRecordBytes,
                    old_records + i * kRecordBytes);
        }
        GameDataEquipRecordLifecycle::DestroyRange(old_records, old_end,
                                                    runtime);
        runtime.Free(old_records);
        runtime.WriteWord(table + 104,
                          replacement + requested * kRecordBytes);
        runtime.WriteWord(table + 96, replacement);
        runtime.WriteWord(table + 100,
                          replacement + record_count * kRecordBytes);
    }

    const auto old_keys = runtime.ReadWord(table + 8);
    const auto key_capacity = old_keys
                                  ? (runtime.ReadWord(table + 16) - old_keys) /
                                        kKeyBytes
                                  : 0;
    if (key_capacity >= requested) return key_capacity;

    const auto count = static_cast<std::int32_t>(requested) < 0 ? 0u
                                                            : requested;
    const auto replacement = runtime.Allocate(count * kKeyBytes);
    const auto old_end = runtime.ReadWord(table + 12);
    const auto key_count = old_keys ? (old_end - old_keys) / kKeyBytes : 0;
    for (std::uint32_t i = 0; i < key_count; ++i) {
        const auto entry = runtime.ReadKey(old_keys + i * kKeyBytes);
        if (replacement)
            runtime.WriteKey(replacement + i * kKeyBytes, entry);
    }
    runtime.Free(old_keys);
    runtime.WriteWord(table + 16,
                      replacement + requested * kKeyBytes);
    runtime.WriteWord(table + 8, replacement);
    const auto new_end = replacement + key_count * kKeyBytes;
    runtime.WriteWord(table + 12, new_end);
    return new_end;
}

std::uint32_t GameDataEquipStorage::Insert(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IGameDataEquipStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto index = begin ? (position - begin) / kRecordBytes : 0;
    const auto count = begin ? (end - begin) / kRecordBytes : 0;
    if (capacity_end != end) {
        if (index == count) {
            runtime.CopyConstructRecord(end, source);
        } else {
            runtime.CopyConstructRecord(end, end - kRecordBytes);
            for (std::uint32_t i = count - 1; i > index; --i)
                runtime.AssignRecord(begin + i * kRecordBytes,
                                     begin + (i - 1) * kRecordBytes);
            runtime.AssignRecord(begin + index * kRecordBytes, source);
        }
        runtime.WriteWord(array + 8, end + kRecordBytes);
        return begin + index * kRecordBytes;
    }

    const auto new_capacity = count + (count > 1 ? count : 1);
    const auto replacement = runtime.Allocate(new_capacity * kRecordBytes);
    if (replacement) {
        for (std::uint32_t i = 0; i < index; ++i)
            runtime.CopyConstructRecord(replacement + i * kRecordBytes,
                                        begin + i * kRecordBytes);
        runtime.CopyConstructRecord(replacement + index * kRecordBytes,
                                    source);
        for (std::uint32_t i = index; i < count; ++i)
            runtime.CopyConstructRecord(replacement + (i + 1) * kRecordBytes,
                                        begin + i * kRecordBytes);
    }
    GameDataEquipRecordLifecycle::DestroyRange(begin, end, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12,
                      replacement + new_capacity * kRecordBytes);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8,
                      replacement + (count + 1) * kRecordBytes);
    return replacement + index * kRecordBytes;
}

std::uint32_t GameDataEquipStorage::Size(
    std::uint32_t array, IGameDataEquipStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    return begin ? (runtime.ReadWord(array + 8) - begin) / kRecordBytes : 0;
}

std::uint32_t GameDataEquipStorage::CopyConstructRange(
    std::uint32_t first, std::uint32_t last,
    std::uint32_t destination, IGameDataEquipStorageRuntime& runtime) {
    for (std::uint32_t source = first; source != last;
         source += kRecordBytes, destination += kRecordBytes)
        runtime.CopyConstructRecord(destination, source);
    return destination;
}

std::uint32_t GameDataEquipStorage::CopyRepeated(
    std::uint32_t destination, std::uint32_t count,
    std::uint32_t source, IGameDataEquipStorageRuntime& runtime) {
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.CopyConstructRecord(destination + i * kRecordBytes, source);
    return count ? source : 0;
}

std::uint32_t GameDataEquipStorage::AssignRepeated(
    std::uint32_t first, std::uint32_t last,
    std::uint32_t source, IGameDataEquipStorageRuntime& runtime) {
    for (std::uint32_t destination = first; destination != last;
         destination += kRecordBytes)
        runtime.AssignRecord(destination, source);
    return last;
}

std::uint32_t GameDataEquipStorage::CopyConstructReverse(
    std::uint32_t first, std::uint32_t last,
    std::uint32_t destination_end,
    IGameDataEquipStorageRuntime& runtime) {
    while (first != last) {
        last -= kRecordBytes;
        destination_end -= kRecordBytes;
        runtime.CopyConstructRecord(destination_end, last);
    }
    return destination_end;
}

std::uint32_t GameDataEquipStorage::CompactAdjacent(
    std::uint32_t first, std::uint32_t last,
    std::uint32_t destination,
    bool (*equal)(const GameDataEquipRecord&, const GameDataEquipRecord&),
    IGameDataEquipStorageRuntime& runtime) {
    if (first == last) return destination;
    auto previous = runtime.ReadRecord(first);
    runtime.AssignRecord(destination, first);
    destination += kRecordBytes;
    for (std::uint32_t source = first + kRecordBytes; source != last;
         source += kRecordBytes) {
        const auto current = runtime.ReadRecord(source);
        if (equal(previous, current)) continue;
        runtime.AssignRecord(destination, source);
        destination += kRecordBytes;
        previous = current;
    }
    return destination;
}

std::uint32_t GameDataEquipStorage::InsertRepeated(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IGameDataEquipStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto spare = (capacity_end - end) / kRecordBytes;
    if (spare < count) {
        const auto old_count = begin ? (end - begin) / kRecordBytes : 0;
        const auto growth = old_count >= count ? old_count : count;
        const auto capacity = old_count + growth;
        const auto allocation_count =
            static_cast<std::int32_t>(capacity) < 0 ? 0u : capacity;
        const auto replacement =
            runtime.Allocate(allocation_count * kRecordBytes);
        if (replacement) {
            auto destination =
                CopyConstructRange(begin, position, replacement, runtime);
            (void)CopyRepeated(destination, count, source, runtime);
            destination += count * kRecordBytes;
            (void)CopyConstructRange(position, end, destination, runtime);
        }
        GameDataEquipRecordLifecycle::DestroyRange(begin, end, runtime);
        runtime.Free(begin);
        runtime.WriteWord(array + 12,
                          replacement + capacity * kRecordBytes);
        runtime.WriteWord(array + 4, replacement);
        runtime.WriteWord(array + 8,
                          replacement + (old_count + count) * kRecordBytes);
        return 5 * (old_count + count);
    }

    if (count == 0) return 0;
    const auto suffix = (end - position) / kRecordBytes;
    if (suffix < count) {
        (void)CopyConstructRange(position, end,
                                 position + count * kRecordBytes, runtime);
        (void)CopyRepeated(end, count - suffix, source, runtime);
        for (std::uint32_t i = 0; i < suffix; ++i)
            runtime.AssignRecord(position + i * kRecordBytes, source);
        const auto new_end = end + count * kRecordBytes;
        runtime.WriteWord(array + 8, new_end);
        return new_end;
    }

    (void)CopyConstructRange(end - count * kRecordBytes, end,
                             end, runtime);
    for (std::uint32_t i = suffix - count; i > 0; --i)
        runtime.AssignRecord(position + (i + count - 1) * kRecordBytes,
                             position + (i - 1) * kRecordBytes);
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.AssignRecord(position + i * kRecordBytes, source);
    runtime.WriteWord(array + 8, end + count * kRecordBytes);
    return count * kRecordBytes;
}

}  // namespace pal4
