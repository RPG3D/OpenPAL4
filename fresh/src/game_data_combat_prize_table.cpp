#include "pal4/game_data_combat_prize_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

template <typename Runtime>
void ConvertRow(std::uint32_t table, std::uint32_t row,
                std::uint32_t count, GameDataCombatPrizeRecord& record,
                Runtime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + index * 4);
        runtime.ConvertField(field, record, runtime.PropertyAt(row, index));
    }
}

template <typename Runtime>
void RequireMatchingRow(std::uint32_t table, std::uint32_t row,
                        Runtime& runtime) {
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));
}

}  // namespace

std::uint8_t GameDataCombatPrizeTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataCombatPrizeRowRuntime& runtime) {
    const auto fields = runtime.ReadWord(table + 24);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 28) - fields) / 60
                                 : 0;
    if (runtime.CollectionSize(table + 124) != field_count) {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.InitializeStructures(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    }
    GameDataCombatPrizeRecord record{};
    RequireMatchingRow(table, row, runtime);
    ConvertRow(table, row, runtime.CollectionSize(table + 124), record,
               runtime);
    const auto key = record[0];
    const auto begin = runtime.ReadWord(table + 8);
    const auto end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(begin, end, key);
    if (position == end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto records = runtime.ReadWord(table + 96);
        const auto record_end = runtime.ReadWord(table + 100);
        const auto index = records ? (record_end - records) / 128 : 0;
        runtime.InsertRecord(table + 92, record_end, record);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
                                 runtime.ReadWord(position + 4) * 128;
        runtime.CopyRecord(destination, record);
    }
    return 1;
}

std::uint32_t GameDataCombatPrizeTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataCombatPrizeAppendRuntime& runtime) {
    GameDataCombatPrizeRecord record{};
    RequireMatchingRow(table, row, runtime);
    ConvertRow(table, row, runtime.CollectionSize(table + 124), record,
               runtime);
    return runtime.AppendRecord(table + 92,
                                runtime.ReadWord(table + 100), record);
}

std::uint32_t GameDataCombatPrizeTable::MarkAndRefresh(
    std::uint32_t table, IGameDataCombatPrizeRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
