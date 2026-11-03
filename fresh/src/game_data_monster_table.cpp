#include "pal4/game_data_monster_table.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr std::string_view kTableName = "Monster";
constexpr std::string_view kMissingTable =
    "ERROR [ palDBMonsterManager::verifyDataTable < the table Monster  "
    "doesn't exist>]";

class PreparedMonsterRecord final {
public:
    explicit PreparedMonsterRecord(IGameDataMonsterRowRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedMonsterRecord() { runtime_.DestroyTemporaryRecordTexts(value); }
    PreparedMonsterRecord(const PreparedMonsterRecord&) = delete;
    PreparedMonsterRecord& operator=(const PreparedMonsterRecord&) = delete;
    GameDataMonsterRecord value{};

private:
    IGameDataMonsterRowRuntime& runtime_;
};

class PreparedAppendRecord final {
public:
    explicit PreparedAppendRecord(IGameDataMonsterAppendRuntime& runtime)
        : runtime_(runtime) {
        runtime_.InitializeRecord(value);
    }
    ~PreparedAppendRecord() { runtime_.DestroyTemporaryRecordTexts(value); }
    PreparedAppendRecord(const PreparedAppendRecord&) = delete;
    PreparedAppendRecord& operator=(const PreparedAppendRecord&) = delete;
    GameDataMonsterRecord value{};

private:
    IGameDataMonsterAppendRuntime& runtime_;
};

}  // namespace

std::uint32_t GameDataMonsterTable::FindRecord(
    std::uint32_t manager, std::int32_t key,
    IGameDataMonsterTableRuntime& runtime) {
    const auto table = runtime.ReadWord(manager + 4);
    auto begin = runtime.ReadWord(table + 8);
    const auto end = runtime.ReadWord(table + 12);
    auto count = static_cast<std::int32_t>(end - begin) / 8;
    while (count > 0) {
        const auto half = count / 2;
        const auto middle = begin + static_cast<std::uint32_t>(half) * 8;
        if (static_cast<std::int32_t>(runtime.ReadWord(middle)) >= key) {
            count = half;
        } else {
            begin = middle + 8;
            count -= half + 1;
        }
    }
    if (begin == end ||
        static_cast<std::int32_t>(runtime.ReadWord(begin)) != key)
        return 0;
    return runtime.ReadWord(table + 96) +
           runtime.ReadWord(begin + 4) * 596;
}

std::uint8_t GameDataMonsterTable::Preload(
    std::uint32_t manager, IGameDataMonsterTableRuntime& runtime) {
    return runtime.PreloadTable(runtime.ReadWord(manager), kTableName,
                                0x4debd0);
}

std::uint8_t GameDataMonsterTable::Verify(
    std::uint32_t manager, IGameDataMonsterTableRuntime& runtime) {
    const auto table = runtime.FindTable(runtime.ReadWord(manager), kTableName);
    runtime.WriteWord(manager + 4, table);
    if (!table) {
        runtime.LogError(kMissingTable);
        throw std::runtime_error(std::string(kMissingTable));
    }
    return 1;
}

std::uint8_t GameDataMonsterTable::AddOrReplace(
    std::uint32_t table, std::uint32_t row,
    IGameDataMonsterRowRuntime& runtime) {
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

    PreparedMonsterRecord record(runtime);
    if (!runtime.ParseBoundRow(table + 108, row, record.value))
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));

    const auto key = record.value[100];
    const auto key_begin = runtime.ReadWord(table + 8);
    const auto key_end = runtime.ReadWord(table + 12);
    const auto position = runtime.FindKey(key_begin, key_end, key);
    if (position != key_end &&
        static_cast<std::int32_t>(key) >=
            static_cast<std::int32_t>(runtime.ReadWord(position))) {
        runtime.CopyRecord(runtime.ReadWord(table + 96) +
                               runtime.ReadWord(position + 4) * 596,
                           record.value);
    } else {
        const auto records = runtime.ReadWord(table + 96);
        const auto end = runtime.ReadWord(table + 100);
        const auto index = records ? (end - records) / 596 : 0;
        runtime.InsertRecord(table + 92, end, record.value);
        runtime.InsertKey(table + 4, position, key, index);
    }
    return 1;
}

void GameDataMonsterTable::Append(
    std::uint32_t table, std::uint32_t row,
    IGameDataMonsterAppendRuntime& runtime) {
    PreparedAppendRecord record(runtime);
    if (runtime.RowSize(row) != runtime.CollectionSize(table + 124))
        throw std::runtime_error(
            "ERROR <dbRecordBindTable< Record >::addRow >" +
            runtime.DumpRow(table, row));
    for (std::uint32_t i = 0; i < runtime.CollectionSize(table + 124); ++i) {
        const auto fields = runtime.ReadWord(table + 128);
        const auto field = runtime.ReadWord(fields + i * 4);
        runtime.ConvertField(field, record.value, runtime.PropertyAt(row, i));
    }
    runtime.AppendRecord(table + 92, runtime.ReadWord(table + 100),
                         record.value);
}

std::uint32_t GameDataMonsterTable::MarkAndRefresh(
    std::uint32_t table, IGameDataMonsterRefreshRuntime& runtime) {
    runtime.WriteByte(table + 88, 1);
    return runtime.Refresh(table);
}

void GameDataMonsterTable::RefreshBinding(
    std::uint32_t table, IGameDataMonsterBindingRuntime& runtime) {
    const auto pointers = runtime.ReadWord(table + 128);
    const auto pointer_count = pointers
                                   ? (runtime.ReadWord(table + 132) -
                                      pointers) / 4
                                   : 0;
    const auto fields = runtime.ReadWord(table + 24);
    const auto field_count = fields
                                 ? (runtime.ReadWord(table + 28) - fields) /
                                       60
                                 : 0;
    if (pointer_count == field_count) return;
    runtime.LogBind("beginBindField, table:", table + 52);
    runtime.AssignEffects(table + 108, table + 20);
    runtime.InitializeStructures(table + 108);
    runtime.BindEffects(table + 108);
    runtime.LogBind("endBindField, table:", table + 52);
}

}  // namespace pal4
