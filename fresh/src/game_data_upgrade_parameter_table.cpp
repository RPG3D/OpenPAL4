#include "pal4/game_data_upgrade_parameter_table.h"

#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 92;
constexpr std::uint32_t kFieldBytes = 60;

[[noreturn]] void ThrowBadRow(std::uint32_t table, std::uint32_t row,
                              IGameDataUpgradeParameterTableRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

GameDataUpgradeParameterRecord ConvertRow(
    std::uint32_t table, std::uint32_t row,
    IGameDataUpgradeParameterTableRuntime& runtime) {
    GameDataUpgradeParameterRecord record{};
    const auto count = runtime.CollectionSize(table + 124);
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + 4 * i);
        runtime.ConvertField(field, record, runtime.PropertyAt(row, i));
    }
    return record;
}

}  // namespace

std::uint8_t GameDataUpgradeParameterTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataUpgradeParameterTableRuntime& runtime) {
    const auto field_begin = runtime.ReadWord(table + 24);
    const auto field_count = field_begin
                                 ? (runtime.ReadWord(table + 28) - field_begin) /
                                       kFieldBytes
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

    const auto record = ConvertRow(table, row, runtime);
    const auto key = record[22];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position != key_end &&
        static_cast<std::int32_t>(key) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto index = runtime.ReadWord(position + 4);
        const auto values = runtime.ReadWord(table + 96);
        runtime.CopyRecord(values + index * kRecordBytes, record);
    } else {
        const auto values = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = values ? (end - values) / kRecordBytes : 0;
        runtime.InsertRecord(table + 92, end, record);
        runtime.InsertKey(table + 4, position, key, index);
    }
    return 1;
}

std::uint32_t GameDataUpgradeParameterTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataUpgradeParameterTableRuntime& runtime) {
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        ThrowBadRow(table, row, runtime);
    const auto record = ConvertRow(table, row, runtime);
    return runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                                record);
}

std::uint32_t GameDataUpgradeParameterTable::MarkAndRefresh(
    std::uint32_t table, IGameDataUpgradeParameterTableRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

void GameDataUpgradeParameterStorage::Sort(
    std::uint32_t table, IGameDataUpgradeParameterStorageRuntime& runtime) {
    const auto records = runtime.ReadWord(table + 96);
    const auto old_end = runtime.ReadWord(table + 100);
    const auto old_count = records ? (old_end - records) / kRecordBytes : 0;
    std::vector<GameDataAiKeyEntry> keys;
    keys.reserve(old_count);

    for (std::uint32_t source = 0; source < old_count; ++source) {
        const auto record = runtime.ReadRecord(records + source * kRecordBytes);
        if (!keys.empty() && keys.back().key == record[22]) continue;
        const auto destination = static_cast<std::uint32_t>(keys.size());
        if (destination != source)
            runtime.WriteRecord(records + destination * kRecordBytes, record);
        keys.push_back({record[22], destination});
    }
    runtime.WriteWord(table + 100,
                      records + static_cast<std::uint32_t>(keys.size()) *
                                    kRecordBytes);

    runtime.WriteWord(table + 12, runtime.ReadWord(table + 8));
    for (const auto& entry : keys) runtime.AppendKey(table + 4, entry);

    GameDataAiKeyAlgorithms::Sort(keys);
    const auto key_begin = runtime.ReadWord(table + 8);
    for (std::uint32_t i = 0; i < keys.size(); ++i)
        runtime.WriteKey(key_begin + i * 8, keys[i]);
}

std::uint32_t GameDataUpgradeParameterStorage::Reserve(
    std::uint32_t table, std::uint32_t requested,
    IGameDataUpgradeParameterStorageRuntime& runtime) {
    const auto old_records = runtime.ReadWord(table + 96);
    const auto record_capacity = old_records
                                     ? (runtime.ReadWord(table + 104) -
                                        old_records) / kRecordBytes
                                     : 0;
    if (record_capacity < requested) {
        const auto allocation_count =
            static_cast<std::int32_t>(requested) < 0 ? 0u : requested;
        const auto replacement = runtime.Allocate(allocation_count * kRecordBytes);
        const auto old_end = runtime.ReadWord(table + 100);
        const auto record_count = old_records
                                      ? (old_end - old_records) / kRecordBytes
                                      : 0;
        for (std::uint32_t i = 0; i < record_count; ++i) {
            const auto record =
                runtime.ReadRecord(old_records + i * kRecordBytes);
            if (replacement)
                runtime.WriteRecord(replacement + i * kRecordBytes, record);
        }
        runtime.Free(old_records);
        runtime.WriteWord(table + 104,
                          replacement + requested * kRecordBytes);
        runtime.WriteWord(table + 96, replacement);
        runtime.WriteWord(table + 100,
                          replacement + record_count * kRecordBytes);
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
    const auto new_end = replacement + key_count * 8;
    runtime.WriteWord(table + 12, new_end);
    return new_end;
}

bool GameDataUpgradeParameterStorage::EqualTailKeys(
    const GameDataUpgradeParameterRecord& left,
    const GameDataUpgradeParameterRecord& right) {
    return left[22] == right[22];
}

}  // namespace pal4
