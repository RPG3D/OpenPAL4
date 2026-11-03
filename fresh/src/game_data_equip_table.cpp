#include "pal4/game_data_equip_table.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 400;
constexpr std::uint32_t kFieldBytes = 60;
constexpr std::uint32_t kKeyWord = 36;

class PreparedRecord final {
public:
    explicit PreparedRecord(IGameDataEquipTableRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedRecord() { runtime_.DestroyRecord(value); }
    PreparedRecord(const PreparedRecord&) = delete;
    PreparedRecord& operator=(const PreparedRecord&) = delete;
    GameDataEquipRecord value{};

private:
    IGameDataEquipTableRuntime& runtime_;
};

[[noreturn]] void ThrowBadRow(std::uint32_t table, std::uint32_t row,
                              IGameDataEquipTableRuntime& runtime) {
    throw std::runtime_error(
        "ERROR <dbRecordBindTable< Record >::addRow >" +
        runtime.DumpRow(table, row));
}

void ConvertRow(std::uint32_t table, std::uint32_t row,
                GameDataEquipRecord& record,
                IGameDataEquipTableRuntime& runtime) {
    const auto count = runtime.CollectionSize(table + 124);
    const auto row_begin = runtime.ReadWord(row + 4);
    const auto row_count = row_begin
                               ? (runtime.ReadWord(row + 8) - row_begin) / 8
                               : 0;
    if (row_count != count) ThrowBadRow(table, row, runtime);
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        runtime.ConvertField(runtime.ReadWord(fields + 4 * i), record,
                             runtime.PropertyAt(row, i));
    }
    if (!runtime.CheckMemberRange(record)) ThrowBadRow(table, row, runtime);
}

}  // namespace

bool GameDataEquipTable::ParseBoundRow(
    std::uint32_t binding, std::uint32_t row,
    GameDataEquipRecord& record, IGameDataEquipTableRuntime& runtime) {
    const auto row_begin = runtime.ReadWord(row + 4);
    const auto row_count = row_begin
                               ? (runtime.ReadWord(row + 8) - row_begin) / 8
                               : 0;
    const auto fields = runtime.ReadWord(binding + 20);
    const auto field_count = fields
                                 ? (runtime.ReadWord(binding + 24) - fields) / 4
                                 : 0;
    if (row_count != field_count) return false;
    for (std::uint32_t i = 0; i < field_count; ++i) {
        const auto current_fields = runtime.ReadWord(binding + 20);
        runtime.ConvertField(runtime.ReadWord(current_fields + 4 * i), record,
                             runtime.PropertyAt(row, i));
    }
    return true;
}

std::uint8_t GameDataEquipTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataEquipTableRuntime& runtime) {
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

    PreparedRecord record(runtime);
    if (!ParseBoundRow(table + 108, row, record.value, runtime) ||
        !runtime.CheckMemberRange(record.value))
        ThrowBadRow(table, row, runtime);

    const auto key = record.value[kKeyWord];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position != key_end &&
        static_cast<std::int32_t>(key) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto index = runtime.ReadWord(position + 4);
        runtime.CopyRecord(runtime.ReadWord(table + 96) +
                               index * kRecordBytes,
                           record.value);
    } else {
        const auto values = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = values ? (end - values) / kRecordBytes : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    }
    return 1;
}

void GameDataEquipTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataEquipTableRuntime& runtime) {
    PreparedRecord record(runtime);
    ConvertRow(table, row, record.value, runtime);
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
}

std::uint32_t GameDataEquipTable::MarkAndRefresh(
    std::uint32_t table, IGameDataEquipTableRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
