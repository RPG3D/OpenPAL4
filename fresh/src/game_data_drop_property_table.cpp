#include "pal4/game_data_drop_property_table.h"

#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

[[noreturn]] void ThrowBadRow(std::uint32_t table, std::uint32_t row,
                              IGameDataDropPropertyTableRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

std::uint32_t ConvertRow(std::uint32_t table, std::uint32_t row,
                         IGameDataDropPropertyTableRuntime& runtime) {
    std::uint32_t value = 0;
    const auto count = runtime.CollectionSize(table + 124);
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + 4 * i);
        runtime.ConvertField(field, value, runtime.PropertyAt(row, i));
    }
    return value;
}

}  // namespace

std::uint8_t GameDataDropPropertyTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataDropPropertyTableRuntime& runtime) {
    const auto field_begin = runtime.ReadWord(table + 24);
    const auto field_count = field_begin
                                 ? (runtime.ReadWord(table + 28) - field_begin) / 60
                                 : 0;
    if (runtime.CollectionSize(table + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.InitializeStructures(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }

    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        ThrowBadRow(table, row, runtime);

    const auto value = ConvertRow(table, row, runtime);
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, value);
    if (position != key_end &&
        static_cast<std::int32_t>(value) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto index = runtime.ReadWord(position + 4);
        runtime.WriteWord(runtime.ReadWord(table + 96) + index * 4, value);
    } else {
        const auto values = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = values ? (end - values) / 4 : 0;
        runtime.InsertValue(table + 92, end, value);
        runtime.InsertKey(table + 4, position, value, index);
    }
    return 1;
}

std::uint32_t GameDataDropPropertyTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataDropPropertyTableRuntime& runtime) {
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        ThrowBadRow(table, row, runtime);
    const auto value = ConvertRow(table, row, runtime);
    return runtime.AppendValue(table + 92, runtime.ReadWord(table + 100),
                               value);
}

std::uint32_t GameDataDropPropertyTable::MarkAndRefresh(
    std::uint32_t table, IGameDataDropPropertyTableRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

void GameDataDropPropertyStorage::Sort(
    std::uint32_t table, IGameDataDropPropertyStorageRuntime& runtime) {
    const auto values = runtime.ReadWord(table + 96);
    const auto end = runtime.ReadWord(table + 100);
    const auto count = values ? (end - values) / 4 : 0;
    std::vector<GameDataAiKeyEntry> keys;
    keys.reserve(count);
    for (std::uint32_t source = 0; source < count; ++source) {
        const auto value = runtime.ReadWord(values + source * 4);
        if (!keys.empty() && keys.back().key == value) continue;
        const auto destination = static_cast<std::uint32_t>(keys.size());
        if (destination != source)
            runtime.WriteWord(values + destination * 4, value);
        keys.push_back({value, destination});
    }
    runtime.WriteWord(table + 100, values +
                                       static_cast<std::uint32_t>(keys.size()) * 4);
    runtime.WriteWord(table + 12, runtime.ReadWord(table + 8));
    for (const auto& entry : keys) runtime.AppendKey(table + 4, entry);
    GameDataAiKeyAlgorithms::Sort(keys);
    const auto key_begin = runtime.ReadWord(table + 8);
    for (std::uint32_t i = 0; i < keys.size(); ++i)
        runtime.WriteKey(key_begin + i * 8, keys[i]);
}

std::uint32_t GameDataDropPropertyStorage::CompactAdjacent(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    const ValueEqual& equal, IGameDataDropPropertyStorageRuntime& runtime) {
    if (begin == end) return destination;
    runtime.WriteWord(destination, runtime.ReadWord(begin));
    auto last_kept = begin;
    auto output = destination + 4;
    for (auto current = begin + 4; current != end; current += 4) {
        if (!equal(runtime.ReadWord(last_kept), runtime.ReadWord(current))) {
            last_kept = current;
            runtime.WriteWord(output, runtime.ReadWord(current));
            output += 4;
        }
    }
    return output;
}

std::uint32_t GameDataDropPropertyStorage::Reserve(
    std::uint32_t table, std::uint32_t requested,
    IGameDataDropPropertyStorageRuntime& runtime) {
    const auto old_values = runtime.ReadWord(table + 96);
    const auto value_capacity = old_values
                                    ? (runtime.ReadWord(table + 104) - old_values) / 4
                                    : 0;
    if (value_capacity < requested) {
        const auto allocation_count =
            static_cast<std::int32_t>(requested) < 0 ? 0u : requested;
        const auto replacement = runtime.Allocate(allocation_count * 4);
        const auto old_end = runtime.ReadWord(table + 100);
        const auto value_count = old_values ? (old_end - old_values) / 4 : 0;
        for (std::uint32_t i = 0; i < value_count; ++i) {
            const auto value = runtime.ReadWord(old_values + i * 4);
            if (replacement) runtime.WriteWord(replacement + i * 4, value);
        }
        runtime.Free(old_values);
        runtime.WriteWord(table + 104, replacement + requested * 4);
        runtime.WriteWord(table + 96, replacement);
        runtime.WriteWord(table + 100, replacement + value_count * 4);
    }

    const auto old_keys = runtime.ReadWord(table + 8);
    const auto key_capacity = old_keys
                                  ? (runtime.ReadWord(table + 16) - old_keys) / 8
                                  : 0;
    if (key_capacity >= requested) return key_capacity;

    const auto allocation_count =
        static_cast<std::int32_t>(requested) < 0 ? 0u : requested;
    const auto replacement = runtime.Allocate(allocation_count * 8);
    const auto old_end = runtime.ReadWord(table + 12);
    const auto key_count = old_keys ? (old_end - old_keys) / 8 : 0;
    for (std::uint32_t i = 0; i < key_count; ++i) {
        const auto entry = runtime.ReadKey(old_keys + i * 8);
        if (replacement) runtime.WriteKey(replacement + i * 8, entry);
    }
    runtime.Free(old_keys);
    runtime.WriteWord(table + 16, replacement + requested * 8);
    runtime.WriteWord(table + 8, replacement);
    runtime.WriteWord(table + 12, replacement + key_count * 8);
    return key_count;
}

}  // namespace pal4
