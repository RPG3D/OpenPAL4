#include "pal4/game_data_ai_record_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 56;
constexpr std::uint32_t kFieldBytes = 60;
constexpr std::uint32_t kKeyBytes = 8;

[[noreturn]] void ThrowBadRow(std::uint32_t table, std::uint32_t row,
                              IGameDataAiRecordTableRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

GameDataAiRecord ConvertRow(std::uint32_t table, std::uint32_t row,
                            IGameDataAiRecordTableRuntime& runtime) {
    GameDataAiRecord record{};
    const auto count = runtime.CollectionSize(table + 124);
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + 4 * i);
        const auto property = runtime.PropertyAt(row, i);
        runtime.ConvertField(field, record, property);
    }
    return record;
}

}  // namespace

std::uint8_t GameDataAiRecordTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataAiRecordTableRuntime& runtime) {
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

    auto record = ConvertRow(table, row, runtime);
    if (!runtime.RecordValid(record)) ThrowBadRow(table, row, runtime);

    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, record[0]);
    if (position != key_end &&
        static_cast<std::int32_t>(record[0]) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto index = runtime.ReadWord(position + 4);
        const auto values = runtime.ReadWord(table + 96);
        runtime.CopyRecord(values + index * kRecordBytes, record);
    } else {
        const auto values = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = values ? (end - values) / kRecordBytes : 0;
        runtime.InsertRecord(table + 92, end, record);
        runtime.InsertKey(table + 4, position, record[0], index);
    }
    return 1;
}

std::uint32_t GameDataAiRecordTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataAiRecordTableRuntime& runtime) {
    const auto fields = runtime.ReadWord(table + 128);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 132) - fields) / 4
                                 : 0;
    if (runtime.RowSize(row) != field_count)
        ThrowBadRow(table, row, runtime);

    auto record = ConvertRow(table, row, runtime);
    if (!runtime.RecordValid(record)) ThrowBadRow(table, row, runtime);
    return runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                                record);
}

std::uint32_t GameDataAiRecordTable::MarkAndRefresh(
    std::uint32_t table, IGameDataAiRecordTableRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

std::uint32_t GameDataAiRecordTable::ClearAndDispatch(
    std::uint32_t table, IGameDataAiRecordTableRuntime& runtime) {
    runtime.WriteByte(table + 88, 0);
    return runtime.DispatchVirtual24(table);
}

}  // namespace pal4
