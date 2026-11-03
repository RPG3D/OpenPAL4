#include "pal4/game_data_upgrade_table.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

std::string InvalidRow(const std::string& detail) {
    return "ERROR <dbRecordBindTable< Record >::addRow >" + detail;
}

template <typename Runtime>
void ConvertRow(std::uint32_t table, std::uint32_t row,
                std::uint32_t count, GameDataUpgradeTableRecord& record,
                Runtime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + index * 4);
        runtime.ConvertField(field, record, runtime.PropertyAt(row, index));
    }
}

}  // namespace

bool GameDataUpgradeTable::HasValidRoleId(
    const GameDataUpgradeTableRecord& record) {
    return record[37] - 1001u < 4u;
}

std::uint8_t GameDataUpgradeTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataUpgradeRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(table + 24);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 28) - fields) / 60
                                 : 0;
    const auto bound_count = runtime.CollectionSize(table + 124);
    if (bound_count != field_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.InitializeStructures(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }

    GameDataUpgradeTableRecord record{};
    runtime.InitializeRecord(record);
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    ConvertRow(table, row, runtime.CollectionSize(table + 124), record, runtime);
    if (!HasValidRoleId(record))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));

    const auto key = record[36];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position == key_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto records = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = records ? (end - records) / 160 : 0;
        runtime.InsertRecord(table + 92, end, record);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
                                 runtime.ReadWord(position + 4) * 160;
        runtime.CopyRecord(destination, record);
    }
    return 1;
}

std::uint32_t GameDataUpgradeTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataUpgradeAppendRuntime& runtime) {
    GameDataUpgradeTableRecord record{};
    runtime.InitializeRecord(record);
    const auto pointers = runtime.ReadWord(table + 128);
    const auto pointer_count = pointers
                                   ? (runtime.ReadWord(table + 132) -
                                      pointers) / 4
                                   : 0;
    if (runtime.RowSize(row) != pointer_count)
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    ConvertRow(table, row, runtime.CollectionSize(table + 124), record, runtime);
    if (!HasValidRoleId(record))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    return runtime.AppendRecord(table + 92,
                                runtime.ReadWord(table + 100), record);
}

std::uint32_t GameDataUpgradeTable::MarkAndRefresh(
    std::uint32_t table, IGameDataUpgradeRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
