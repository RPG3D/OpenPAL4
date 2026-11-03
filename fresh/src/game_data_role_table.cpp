#include "pal4/game_data_role_table.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

template <typename Runtime>
class PreparedRoleRecord final {
public:
    explicit PreparedRoleRecord(Runtime& runtime) : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedRoleRecord() { runtime_.DestroyTemporaryRecord(value); }
    GameDataRoleTableRecord value{};

private:
    Runtime& runtime_;
};

std::string InvalidRow(const std::string& detail) {
    return "ERROR <dbRecordBindTable< Record >::addRow >" + detail;
}

}  // namespace

bool GameDataRoleTable::NormalizeAndValidate(
    GameDataRoleTableRecord& record) {
    const auto index = record[57] - 1001u;
    record[56] = index;
    return index < 4u && record[0] <= 6u;
}

std::uint8_t GameDataRoleTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataRoleRowRuntime& runtime) {
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

    PreparedRoleRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value) ||
        !NormalizeAndValidate(record.value))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));

    const auto key = record.value[57];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position == key_end ||
        static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        const auto begin = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = begin ? (end - begin) / 320 : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    } else {
        const auto destination = runtime.ReadWord(table + 96) +
                                 runtime.ReadWord(position + 4) * 320;
        runtime.CopyRecord(destination, record.value);
    }
    return 1;
}

void GameDataRoleTable::Append(std::uint32_t table, std::uint32_t row,
                               IGameDataRoleAppendRuntime& runtime) {
    PreparedRoleRecord record(runtime);
    const auto count = runtime.CollectionSize(table + 124);
    if (runtime.RowSize(row) != count)
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + index * 4);
        runtime.ConvertField(field, record.value,
                             runtime.PropertyAt(row, index));
    }
    if (!NormalizeAndValidate(record.value))
        throw std::runtime_error(InvalidRow(runtime.DumpRow(table, row)));
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
}

std::uint32_t GameDataRoleTable::MarkAndRefresh(
    std::uint32_t table, IGameDataRoleRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

}  // namespace pal4
